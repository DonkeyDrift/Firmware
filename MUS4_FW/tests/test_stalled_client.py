"""v1.10.17 停滞客户端写阻塞修复：源码结构 + 行为镜像锁定测试。

病灶（实车 v1.10.17 复测）：单个停滞 TCP 客户端（连上 80 端口发请求后不读响应）
把 wifi_web 任务串行阻塞 ~10s（实测 web_http_dt_max=10038），期间其他 HTTP 请求
全部超时；写超时后连接被杀才自动恢复。

根因（core 3.3.10 源码核实，见 WebConsoleServer.cpp 防护段注释）：
NetworkClient::write() = select(1s) × WIFI_CLIENT_MAX_WRITE_RETRY(10) 的内部
重试循环——send() 本身带 MSG_DONTWAIT 从不阻塞，阻塞全部来自 select 等待
socket 可写；对端停读（TCP 发送缓冲满）时单次 write 最长 ~10s。该重试次数与
select 超时均不可从 sketch 配置；SO_SNDTIMEO/setTimeout 对这条路径无效
（send 不阻塞，SO_SNDTIMEO 无从生效）；NetworkClient 没有 availableForWrite()
实现（继承基类默认恒 0）。WebServer 每个响应都带 Connection: close、一连接一
请求，因此阻塞点唯一：大响应体那一次 write()。

修复机制（本文件锁定）：
1. 大响应体不再交给 NetworkClient::write，改裸 lwip send(fd, buf, n,
   MSG_DONTWAIT) 分块直写——只写入发送缓冲当前装得下的字节数，立即返回，
   彻底绕开 core 的 1s×10 select 重试；
2. 「无进展超时」判定停滞：任何字节进展都重置 deadline，连续
   WIFI_WEB_STALL_TIMEOUT_MS 无进展即 client.stop() 放弃连接；
3. 响应头（~150B，经 WebServer::send 生成）发送前先做一次短超时可写探测，
   探不到可写即放弃，杜绝头部 write 进入 core 的 10s 重试；
4. 头部用 setContentLength(len)+send(code, type) 生成，与原 send()/send_P()
   字节等价（Content-Length 正确、此前 sendHeader 的自定义头全部保留），
   鉴权 / 503 中间件 / 路由语义不变；
5. OTA 上传不受影响：上传数据走 _parseForm 读路径，本修复只改响应写路径；
   503 中间件、鉴权失败、tiny 字面量文本响应保持 core 原路径。
"""

import pathlib
import re

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
SERVER = PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"


def body_of(source, signature_pattern):
    m = re.search(signature_pattern, source, re.DOTALL)
    assert m is not None, f"未找到函数体: {signature_pattern}"
    return m.group(1)


def uint_const(source, name):
    m = re.search(rf"static const \w+ {name} = (\d+);", source)
    assert m is not None, f"未找到常量定义: {name}"
    return int(m.group(1))


# ---------------------------------------------------------------------------
# 结构断言：机制存在且形态正确
# ---------------------------------------------------------------------------

def test_guard_constants_within_target():
    """停滞放弃阈值必须落在百毫秒级（任务目标），探测/分块常量同时存在。"""
    source = SERVER.read_text(encoding="utf-8")
    timeout_ms = uint_const(source, "WIFI_WEB_STALL_TIMEOUT_MS")
    probe_ms = uint_const(source, "WIFI_WEB_HEADER_PROBE_MS")
    chunk = uint_const(source, "WIFI_WEB_SEND_CHUNK")
    assert 500 <= timeout_ms <= 2000, f"WIFI_WEB_STALL_TIMEOUT_MS={timeout_ms} 不在 1s 级目标内"
    assert 10 <= probe_ms <= 250
    assert 256 <= chunk <= 1024


def test_guarded_body_writer_uses_nonblocking_send():
    """响应体写路径必须绕开 NetworkClient::write 的 1s×10 select 重试：
    裸 send(MSG_DONTWAIT)、按进展重置 deadline、停滞即 stop 放弃。"""
    source = SERVER.read_text(encoding="utf-8")
    nonblock = body_of(source, r"static int wifiWebSendNonblock\(int fd, const char\* data, size_t len\)\s*\{(.*?)\n\}")
    body = body_of(source, r"static bool wifiWebWriteBodyGuarded\(const char\* data, size_t len\)\s*\{(.*?)\n\}")

    assert "MSG_DONTWAIT" in nonblock, "必须用非阻塞 send 直写 socket"
    assert re.search(r"\bsend\(\s*fd\s*,", nonblock), "必须以 fd 直调 lwip send"
    assert "EWOULDBLOCK" in nonblock
    assert ".write(" not in body, "响应体不得再经 NetworkClient::write（内部 1s×10 select 重试）"
    assert body.count("deadline = millis() + WIFI_WEB_STALL_TIMEOUT_MS") >= 2, (
        "deadline 必须在初始化与每次进展后重置（无进展超时，而非总时长超时）"
    )
    assert "c.stop();" in body, "停滞判定成立必须 client.stop() 放弃连接"
    assert "s_wifiWebStallAborts++" in body, "放弃必须计数（STATUS 可观测）"
    assert "vTaskDelay(1)" in body, "等待可写期间必须让出 CPU（wifi_web 任务）"


def test_progmem_body_writer_chunks():
    """PROGMEM 大响应（首页/判分/漂移/更新页/favicon）分块拷贝到 RAM 再守卫写出。"""
    source = SERVER.read_text(encoding="utf-8")
    body = body_of(source, r"static bool wifiWebWriteBodyGuarded_P\(PGM_P data, size_t len\)\s*\{(.*?)\n\}")
    assert "memcpy_P" in body
    assert "WIFI_WEB_SEND_CHUNK" in body
    assert "wifiWebWriteBodyGuarded(" in body


def test_guarded_send_emits_headers_via_webserver():
    """头部仍由 WebServer 的 _prepareHeader 生成（状态行/自定义头/长度与原版
    一致），但写出必须走非阻塞守卫——core 的 send() 会在头部陷入 1s×10
    重试，正是实车残留 ~9s 阻塞的来源。发头前先做短超时可写探测。"""
    source = SERVER.read_text(encoding="utf-8")
    headers = body_of(source, r"static bool wifiWebSendHeadersGuarded\(int code, const char\* contentType, size_t len\)\s*\{(.*?)\n\}")

    assert "wifiWebFdWritable(fd, WIFI_WEB_HEADER_PROBE_MS)" in headers, "发头前必须做可写探测"
    assert "setContentLength(len)" in headers
    assert "prepareResponseHeader(header, code, contentType, len)" in headers
    assert "wifiWebWriteBodyGuarded(header.c_str(), header.length())" in headers
    assert ".send(code, contentType)" not in headers, "响应头不得再走 core 阻塞写"
    assert "setContentLength(CONTENT_LENGTH_NOT_SET)" in headers, "发完头必须复位，避免污染下一响应"
    assert "s_wifiWebStallAborts++" in headers, "探测失败即视为停滞放弃并计数"

    for pattern in (
        r"static void wifiWebSendGuardedString\(int code, const char\* contentType, const String& content\)\s*\{(.*?)\n\}",
        r"static void wifiWebSendGuarded_P\(int code, const char\* contentType, PGM_P content, size_t len\)\s*\{(.*?)\n\}",
    ):
        body = body_of(source, pattern)
        assert "wifiWebSendHeadersGuarded(" in body
    string_entry = body_of(source, r"static void wifiWebSendGuardedString\(int code, const char\* contentType, const String& content\)\s*\{(.*?)\n\}")
    assert "wifiWebWriteBodyGuarded(" in string_entry
    pgm_entry = body_of(source, r"static void wifiWebSendGuarded_P\(int code, const char\* contentType, PGM_P content, size_t len\)\s*\{(.*?)\n\}")
    assert "wifiWebWriteBodyGuarded_P(" in pgm_entry


def test_all_variable_body_responses_route_through_guard():
    """所有携带动态响应体的 send 调用必须改走守卫路径；仅允许字面量小响应
    （NACK/ACK/错误 JSON 常量等）保留 core 原路径。"""
    source = SERVER.read_text(encoding="utf-8")

    # 大 PROGMEM 页面全部走守卫
    assert "wifiWebServer.send_P(200" not in source, "大 PROGMEM 页面必须走 wifiWebSendGuarded_P"

    # 任何 wifiWebServer.send(...) 的第三个参数只允许是字符串字面量（或无第三参）
    for call in re.findall(r"wifiWebServer\.send\([^;]*\)", source):
        third = re.search(r"wifiWebServer\.send\(\s*\d+\s*,\s*(?:String\s*\()?\s*\"[^\"]*\"\s*\)?\s*,\s*(.+)\)$", call, re.DOTALL)
        if third is None:
            continue  # 两参形式（守卫内部只发头）或字面量第三参
        arg = third.group(1).strip().rstrip(")")
        assert arg.startswith('"'), (
            f"动态响应体必须走 wifiWebSendGuardedString，不得直接 send: {call[:80]}"
        )


def test_status_reports_stall_aborts():
    """STATUS / /api/status 暴露 web_stall_aborts 放弃计数，字段风格与现有一致。"""
    source = SERVER.read_text(encoding="utf-8")
    status_printf = body_of(source, r"void printWirelessStatus\(Print& out\)\s*\{(.*?)\n\}")
    assert "web_stall_aborts=%lu" in status_printf
    assert "s_wifiWebStallAborts" in status_printf


def test_dead_end_apis_not_relied_upon():
    """本 core 上无效的 API 不得被防护代码调用（注释说明除外）：
    availableForWrite（NetworkClient 无实现，恒 0）与 SO_SNDTIMEO
    （send 带 MSG_DONTWAIT，超时无从生效）。"""
    source = SERVER.read_text(encoding="utf-8")
    assert not re.search(r"\.\s*availableForWrite\s*\(", source), "NetworkClient 无 availableForWrite 实现（恒 0），不得依赖"
    assert "setSocketOption(SO_SNDTIMEO" not in source, "SO_SNDTIMEO 对 core 的 MSG_DONTWAIT 写路径无效，不得依赖"


def test_ota_upload_path_unaffected():
    """OTA 上传（/update 长传输）不受影响：
    - 上传走 _parseForm 读路径，客户端读超时提升（30s）保留；
    - Update 写入流程不变；
    - 503 中间件原样保留；
    - 上传成功 ACK 仍走 core 原路径（tiny 字面量）。"""
    source = SERVER.read_text(encoding="utf-8")

    upload_body = body_of(source, r"static void handleWifiWebUpdateUpload\(\)\s*\{(.*?)\n\}")
    assert "wifiWebServer.client().setTimeout(30000)" in upload_body
    assert "Update.write(upload.buf, upload.currentSize)" in upload_body

    setup_body = body_of(source, r"void setupWebConsoleServer\(\)\s*\{(.*?)\n\}")
    assert 'otaRuntime.inProgress && server.uri() != "/update"' in setup_body
    assert 'server.send(503, "text/plain", "OTA in progress\\n")' in setup_body

    post_body = body_of(source, r"static void handleWifiWebUpdatePost\(\)\s*\{(.*?)\n\}")
    assert 'wifiWebServer.send(200, "text/plain", "ACK:UPDATE_OK\\n")' in post_body


# ---------------------------------------------------------------------------
# 行为镜像：用 Python 复刻 core 写循环与守卫写循环，锁定量化目标
# ---------------------------------------------------------------------------

SND_BUF = 5744          # lwip TCP 发送缓冲（字节）
PEER_WINDOW = 2048      # 停滞客户端 SO_RCVBUF≈2KB 决定的对端接收窗口
SELECT_TIMEOUT_MS = 1000  # core NetworkClient::write 的 select 超时
WRITE_RETRY = 10          # core WIFI_CLIENT_MAX_WRITE_RETRY


class PeerLink:
    """ms 粒度的简化 TCP 链路模型：capacity 内可写，按 drain_bps_ms 释放；
    发送缓冲满后先停读 stall_ms（模拟慢客户端短暂停顿）再继续排空。"""

    def __init__(self, drain_bps_ms, stall_ms=0):
        self.capacity = SND_BUF + PEER_WINDOW
        self.free = self.capacity
        self.drain = drain_bps_ms
        self.stall_ms = stall_ms
        self._first_zero_t = None

    def tick(self, t):
        if self.free > 0:
            self._first_zero_t = None
            self.free = min(self.capacity, self.free + self.drain)
            return
        if self._first_zero_t is None:
            self._first_zero_t = t
        if t - self._first_zero_t >= self.stall_ms:
            self.free = min(self.capacity, self.free + self.drain)
            self._first_zero_t = None


def core_networkclient_write(peer, body):
    """镜像 core 3.3.10 NetworkClient::write(buf, size)：select(1s)×10 重试，
    select 可写立即返回、不可写等满 1s；部分成功重置 retry。返回 (耗时ms, 已发送)。"""
    sent = 0
    retry = WRITE_RETRY
    t = 0
    while retry > 0 and sent < len(body):
        retry -= 1
        if peer.free <= 0:
            t += SELECT_TIMEOUT_MS  # select 等满 1s（不可写）
            continue
        n = min(peer.free, len(body) - sent)
        sent += n
        peer.free -= n
        if sent < len(body):
            retry = WRITE_RETRY  # 部分成功重置重试（停滞场景的代价放大器）
    return t, sent


def guarded_body_write(peer, body, timeout_ms):
    """镜像 wifiWebWriteBodyGuarded：send(MSG_DONTWAIT) 只写装得下的字节，
    无进展每 ms 让出一次，无进展超阈值即放弃。返回 (耗时ms, 已发送, 是否放弃)。"""
    sent = 0
    t = 0
    deadline = timeout_ms
    while sent < len(body):
        if peer.free > 0:
            n = min(peer.free, len(body) - sent)
            sent += n
            peer.free -= n
            deadline = t + timeout_ms
            continue
        if t >= deadline:
            return t, sent, True
        t += 1  # vTaskDelay(1)
        peer.tick(t)
    return t, sent, False


def test_mirror_stalled_client_core_blocks_10s_guard_aborts_in_ms():
    """停滞客户端（不读）：core 写循环镜像复现实测 ~10s 阻塞；
    守卫写循环在 WIFI_WEB_STALL_TIMEOUT_MS 内放弃。"""
    source = SERVER.read_text(encoding="utf-8")
    timeout_ms = uint_const(source, "WIFI_WEB_STALL_TIMEOUT_MS")
    body = b"x" * 24000  # 首页 HTML 量级

    t_core, sent_core = core_networkclient_write(PeerLink(drain_bps_ms=0), body)
    assert t_core >= (WRITE_RETRY - 1) * SELECT_TIMEOUT_MS, (
        f"core 镜像应复现 ~10s 串行阻塞，实际 {t_core}ms"
    )

    t_new, sent_new, aborted = guarded_body_write(PeerLink(drain_bps_ms=0), body, timeout_ms)
    assert aborted, "停滞客户端必须被放弃"
    assert t_new <= timeout_ms + 50, f"守卫写耗时 {t_new}ms 应 bound 在阈值+余量内"
    assert sent_new <= SND_BUF + PEER_WINDOW, "停滞场景只应发出初始窗口内的字节"


def test_mirror_normal_client_completes_without_abort():
    """正常局域网客户端（持续排空）：守卫写完整送达、不放弃、毫秒级完成。"""
    source = SERVER.read_text(encoding="utf-8")
    timeout_ms = uint_const(source, "WIFI_WEB_STALL_TIMEOUT_MS")
    body = b"x" * 24000

    t_new, sent_new, aborted = guarded_body_write(PeerLink(drain_bps_ms=1436), body, timeout_ms)
    assert not aborted
    assert sent_new == len(body)
    assert t_new < timeout_ms


def test_mirror_cloud_tls_cpu_starvation_does_not_false_abort():
    """实车回归：云端 TLS 与 wifi_web 同核时，正常客户端可能超过 300ms
    得不到发送机会；这种短暂无进展不是对端停读，不得切断连接。"""
    source = SERVER.read_text(encoding="utf-8")
    timeout_ms = uint_const(source, "WIFI_WEB_STALL_TIMEOUT_MS")
    assert timeout_ms > 750
    peer = PeerLink(drain_bps_ms=64, stall_ms=750)
    t_new, sent_new, aborted = guarded_body_write(peer, b"x" * 8192, timeout_ms)
    assert not aborted, "同核 TLS 造成的短暂无进展不得被误判为停滞客户端"
    assert sent_new == 8192


def test_mirror_slow_but_alive_client_not_killed():
    """慢而活着的客户端（缓冲填满后停读 150ms < 放弃阈值，随后继续排空）：
    不得误伤，完整送达。"""
    source = SERVER.read_text(encoding="utf-8")
    timeout_ms = uint_const(source, "WIFI_WEB_STALL_TIMEOUT_MS")
    assert timeout_ms > 150  # 前置：阈值必须大于正常短促停读
    body = b"x" * 8192

    peer = PeerLink(drain_bps_ms=64, stall_ms=150)
    t_new, sent_new, aborted = guarded_body_write(peer, body, timeout_ms)
    assert not aborted, "有进展的慢客户端不得被放弃"
    assert sent_new == len(body)
