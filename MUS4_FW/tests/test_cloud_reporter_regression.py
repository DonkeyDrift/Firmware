"""v1.10.17 云端上报回归的修复锁定（实车 192.168.3.9 取证 + 链接 map 内存论证）。

回归现象：上一轮把 TLS 会话改成「按次建连、用完释放」后 `POST /report -> -1`
反复出现，每次尝试对 DRAM 池（MALLOC_CAP_8BIT，malloc 实际服务的池）打入
~48KB 瞬时冲击；60s 快速重试不停开火，该池实测最低只剩 108 字节，25 分钟两次
PANIC，崩溃落在重试节拍上。

根因（core 3.3.10 源码 + 实车日志 + map 交叉验证）：
  - HTTPClient 的 -1 = HTTPC_ERROR_CONNECTION_REFUSED，是 `client->connect()`
    整体失败的统一编码（DNS/TCP/TLS 握手任一失败都折叠成它）。
  - 失败尝试期间 free_heap 仍降 ~48KB（83.5→34.7KB INTERNAL）→ 缓冲区分配
    成功，失败发生在握手内部的分配点——DRAM 池碎片化时 malloc 失败。
  - 口径掩盖：`ESP.getMinFreeHeap()` 是 MALLOC_CAP_INTERNAL 口径（8BIT 池 +
    IRAM-only 池）。map 计算本构建 IRAM-only 池稳态 ~34.7KB，故 INTERNAL 读数
    34712 时 8BIT 池恰好只剩 ~108 字节——稳态 INTERNAL ~85KB 对应 8BIT 池仅
    ~50.7KB，而一次 TLS 尝试峰值 ~48.8KB：**按次建连只有 ~1.9KB 余量，
    在该 core/库组合（无 setBufferSizes/conf_bufsizes 缩冲缩减 API）下结构性
    不可行**，任何门限都无法既保安全又保可用。

修复语义（父任务授权的 Plan B：常驻会话 + 明确释放/重建策略）：
  1. TLS 会话「纪元」常驻：WiFiClientSecure/HTTPClient 在任务级构造，心跳经
     HTTP keep-alive 复用（稳态每跳 ~0KB 新分配）；任一跳失败立即 client.stop()
     整体释放进入冷态。冷态下分配必然失败（常驻态仅剩 ~17.7KB < 重握手所需
     33KB），算术上杜绝了 stale-context 重复 ssl_setup 的 32KB 泄漏路径；
  2. 冷态重建走 DRAM 门限（free8/maxAlloc8 双条件，常量带依据注释），不达标
     零分配跳过本次；
  3. 连续失败（含低堆跳过）退避 60s→120s→300s 封顶，首次成功后恢复 5 分钟
     节奏；
  4. 日志带回 min_heap8/max_blk/next/fails，stack_hwm 继续回传。
"""

import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
REPORTER_PATH = (
    PROJECT_ROOT / "libraries" / "mus4_cloud" / "src" / "CloudReporter.cpp"
)


def _source():
    return REPORTER_PATH.read_text(encoding="utf-8")


def _function_block(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


def _call_block(source, signature):
    start = source.index(signature)
    opening = source.index("(", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "(":
            depth += 1
        elif source[index] == ")":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated call: {signature}")


# --- 1. TLS 会话「纪元」生命周期：任务级常驻 + 失败整体释放 ---


def test_tls_epoch_lifecycle_resident_with_explicit_teardown():
    """会话对象在任务级（for(;;) 之前）构造并跨心跳常驻——按次建连已被实测
    证伪（8BIT 池稳态 ~50.7KB vs 尝试峰值 ~48.8KB）。释放/重建策略必须显式：
    成功跳保持会话复用（不得 stop）；失败跳立即 client.stop() 整体释放。"""
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")
    loop_start = worker.index("for (;;")

    # 任务级常驻：网络对象在循环前构造，恰好一处
    assert "WiFiClientSecure client;" in worker[:loop_start]
    assert "HTTPClient http;" in worker[:loop_start]
    assert worker.count("WiFiClientSecure client;") == 1
    assert worker.count("HTTPClient http;") == 1
    # 构造后先配置再进循环
    assert worker.index("client.setInsecure();") < loop_start
    assert worker.index("http.setConnectTimeout(") < loop_start

    # 失败跳显式整体释放（纪元终结）；成功跳保持会话（不 stop）
    assert "client.stop();" in worker
    stop_idx = worker.index("client.stop();")
    assert stop_idx > loop_start  # stop 在循环内的失败分支,不在构造处
    # 会话健康标志:成功置位、失败清零并释放
    assert "epochHealthy = true;" in worker
    assert "epochHealthy = false;" in worker
    assert worker.index("epochHealthy = false;") < stop_idx

    # reportNow 内部顺序：begin 成功才 POST，POST 之后必须 end() 再返回
    #（begin 失败分支里还有一处前置 http.end() 清理，属正常防御，取 POST 之后那处）
    attempt = _function_block(source, "static ReportAttemptResult reportNow(")
    begin = attempt.index("http.begin(client, CLOUD_REPORT_URL)")
    post = attempt.index("result.httpCode = http.POST(body);")
    end = attempt.index("http.end();", post)
    assert begin < post < end


# --- 2. 冷态重建门限：DRAM（8BIT 池）双条件 + 不达标零分配跳过 ---


def test_cold_rebuild_gate_constants_and_zero_alloc_skip():
    """门限常量必须在本库内并带依据注释；冷态（epochHealthy=false）时门限检查
    先于任何网络动作，不达标零分配跳过（第三轮收紧：free8≥64KB = 峰值 48.8KB
    + ≥15KB 残余地板；maxAlloc8≥49KB = 两块 16.4KB 连续缓冲最坏同块 + ~15.7KB
    握手瞬态，与实测峰值总量双向印证；第二轮失败形态 81KB/38.9KB 被拒绝）。"""
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")

    free_threshold = re.search(
        r"CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD\s*=\s*(\d+)", source
    )
    block_threshold = re.search(
        r"CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD\s*=\s*(\d+)", source
    )
    assert free_threshold is not None, "缺少 DRAM 池空闲门限常量"
    assert block_threshold is not None, "缺少 DRAM 池最大连续块门限常量"
    # 依据：单次 TLS（重）建连瞬时实测消耗 ~48.8KB，全部由 8BIT 池供给；
    # free8 门限 = 峰值 + ≥15KB 残余地板（单次尝试不可能把池打到 KB 级以下），
    # 且不得高过开机期无会话 free8 实测 ~81KB 减余量；maxAlloc8 门限 = 33.3KB
    # 双连续缓冲 + 握手瞬态 ≈ 峰值总量，必须高于实测失败案例的 38.9KB。
    assert int(free_threshold.group(1)) >= 60000
    assert int(free_threshold.group(1)) <= 70000
    assert int(block_threshold.group(1)) > 38900
    assert int(block_threshold.group(1)) >= 2 * 16640 + 8000
    assert int(block_threshold.group(1)) <= 54000

    # 门限判据是纯函数（free 地板 / 连续块两个谓词 + 组合判据），worker 用
    # 8BIT 口径读取后判断
    free_floor = _function_block(source, "static bool reportHeapFreeFloorOk(")
    contiguous = _function_block(source, "static bool reportHeapContiguousOk(")
    gate = _function_block(source, "static bool reportHeapGateOk(")
    assert "free8 >= CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD" in free_floor
    assert "maxAlloc8 >= CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD" in contiguous
    assert "reportHeapFreeFloorOk(free8)" in gate
    assert "reportHeapContiguousOk(maxAlloc8)" in gate
    assert "heap_caps_get_free_size(MALLOC_CAP_8BIT)" in worker
    assert "heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)" in worker

    # 冷态门限判断先于发起尝试；低堆/碎片走独立分支：零网络动作，POD 回传
    gate_check = worker.index("reportHeapGateOk(")
    attempt_call = worker.index("reportNow(client, http, hostname)")
    assert gate_check < attempt_call
    assert "result.skippedLowHeap = true;" in worker
    assert "result.skippedFrag = true;" in worker
    assert (
        worker.index("result.skippedLowHeap = true;") < attempt_call
    )
    assert (
        worker.index("result.skippedFrag = true;") < attempt_call
    )


def test_worker_and_attempt_still_do_not_log_directly():
    """worker/attempt 不得直接写日志（含低堆跳过路径），日志只在主 loop
    消费 POD 结果时输出——沿用既有跨任务约束。"""
    source = _source()
    for signature in (
        "void cloudReportTask(",
        "static ReportAttemptResult reportNow(",
        "static bool reportHeapGateOk(",
    ):
        body = _function_block(source, signature)
        for log_call in ("mus4LogLine", "mus4Logf", "appendWebLog"):
            assert not re.search(rf"\b{log_call}\s*\(", body)


# --- 3. 失败退避：60s→120s→300s 封顶，成功后回 5 分钟节奏 ---


def test_failure_backoff_ladder_and_success_reset():
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")
    ladder = _function_block(source, "static unsigned long retryBackoffMs(")
    interval = _function_block(
        source, "static unsigned long retryIntervalMs("
    )

    # 阶梯：0 次 → 60s；1 次 → 120s；≥2 次 → 300s 封顶
    assert "CLOUD_REPORT_RETRY_MS" in ladder
    assert "CLOUD_REPORT_RETRY_MS * 2" in ladder
    assert "CLOUD_REPORT_INTERVAL_MS" in ladder
    assert ladder.index("CLOUD_REPORT_RETRY_MS * 2") < ladder.index(
        "CLOUD_REPORT_INTERVAL_MS"
    )
    # 间隔选择（第三轮）：成功过 → 5 分钟；未成功且在开机早期窗口 → 15s
    # 积极重试；否则 → 退避阶梯（详见 tests/test_cloud_early_session.py）
    assert "nowMs < CLOUD_REPORT_EARLY_WINDOW_MS" in interval
    assert "return CLOUD_REPORT_EARLY_RETRY_MS;" in interval
    assert "return retryBackoffMs(consecutiveFailures);" in interval
    assert (
        "retryIntervalMs(now, everSucceeded, consecutiveFailures)" in worker
    )

    # worker 状态机：非 2xx 一律计失败（含低堆/碎片跳过），2xx 清零
    assert "consecutiveFailures += 1;" in worker
    assert "consecutiveFailures = 0;" in worker
    # 低堆/碎片跳过也计入退避（不能在低堆期每分钟刷一条跳过日志）
    skip_assign = worker.index("result.skippedLowHeap = true;")
    fail_inc = worker.index("consecutiveFailures += 1;")
    ok_reset = worker.index("consecutiveFailures = 0;")
    assert skip_assign < fail_inc  # 跳过路径与失败路径共用同一计数
    assert ok_reset < fail_inc  # 成功清零在失败递增之前可达


# --- 4. 可追溯性：日志字段 + 实测继续回传 ---


def test_traceability_log_fields_and_pod_channel():
    """主 loop 日志必须带回：结果码、当前堆、8BIT 池最低水位、最大连续块、
    下次重试间隔、连续失败数；stack_hwm/heap/min_heap 原字段继续保留。"""
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")
    emit = _function_block(source, "static void emitPendingReportLog(")

    # 实测点仍在 worker 上下文
    assert "uxTaskGetStackHighWaterMark(nullptr)" in worker
    assert "ESP.getFreeHeap()" in worker
    assert "ESP.getMinFreeHeap()" in worker
    assert "esp_get_minimum_free_heap_size()" in worker  # 8BIT 口径低水位

    # POD 回传通道扩展（不跨任务共享 String）
    for field in (
        "skippedLowHeap",
        "freeHeap8",
        "maxAlloc8",
        "nextIntervalMs",
        "consecutiveFailures",
    ):
        assert field in source

    # 日志字段：原字段保留 + 新增追溯字段
    assert "POST /report -> %d" in emit
    assert "min_heap8=" in emit
    assert "max_blk=" in emit
    assert "stack_hwm=" in emit
    assert "next=" in emit
    assert "fails=" in emit
    # 低堆跳过有独立日志行，带当前堆与重试间隔
    assert "skip low heap" in emit


def test_backoff_does_not_touch_protocol_or_task_shape():
    """退避只改节奏：上报内容/云端接口/worker 独立性不得被顺手改动。"""
    source = _source()
    update = _function_block(source, "void update()")
    body = _function_block(source, "static String buildReportBody(")

    assert "CLOUD_REPORT_INTERVAL_MS = 300000UL" in source
    assert "CLOUD_REPORT_RETRY_MS = 60000UL" in source
    # 协议体不变（按源码实际字面量匹配，JSON 引号在 C++ 源里是转义形式）
    assert 'body += "{\\"device_id\\":\\"";' in body
    assert '\\"type\\":\\"esp32\\"' in body
    assert '\\"port\\":\\"80\\"' in body
    # 主 loop 仍不做同步网络操作
    assert "reportNow(" not in update
    assert ".POST(" not in update
    create = _call_block(source, "xTaskCreatePinnedToCore")
    args = create[create.index("(") + 1 : -1].split(",")
    assert args[0].strip() == "cloudReportTask"
    assert args[-1].strip() == "0"
