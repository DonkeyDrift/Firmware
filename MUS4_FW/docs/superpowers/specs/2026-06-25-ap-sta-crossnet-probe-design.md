# 配网后前端跨网探测 STA 并显示 IP 设计

## 背景

信道预对齐让 STA 连上时 AP 不再二次掉线，但保存配网时 AP 仍会切一次信道、断开当前客户端。若用户笔记本同时连着网线，操作系统判定已有网络，不会自动重连 MUS4-ESP AP，导致 `192.168.4.1` 页面回不来，无法自动显示 STA IP。

用户已确认方向：**前端跨网探测 STA**。即页面在 AP 断连后，利用笔记本的有线网络，通过设备 STA 侧 mDNS 地址直接拿到 STA 状态并在原页面显示 IP。

前提：用户有线网络与设备 STA 接入的是同一局域网（同一路由器），这样设备 STA 连上后，笔记本可经有线访问 `http://<mdns>.local/`。

## 目标

1. 保存 STA 后，前端在等待期间并行探测两类地址：
   - 相对路径 `/api/wifi-sta`（AP 恢复时可用）；
   - 绝对地址 `http://<mdns>.local/api/wifi-sta`（经有线访问 STA 侧）。
2. 任一探测拿到 `connected + sta_ip` 即在原页面直接显示 STA IP 和 `http://<sta_ip>/` 链接。
3. 后端 STA 状态端点返回 CORS 头，使跨源探测能读到响应。
4. 探测失败时回退到 mDNS / 路由器引导文案，不比现状差。
5. 不破坏 AP 侧原有显示路径与失败提示。

## 非目标

- 不保证所有浏览器/系统都能跨源 + 解析 `.local`（存在 PNA、mDNS 支持差异）。
- 不改 Wi-Fi 状态机、信道预对齐、BOOT 长按。
- 不要求笔记本自动重连 AP。

## 设计

### 后端：STA 状态端点加 CORS 头

修改 `MUS4_FW/libraries/mus4_web/src/WebConsoleServer.cpp`：

- `handleWifiWebSta()`（GET `/api/wifi-sta`）在 `send(200, ...)` 前加响应头：
  - `Access-Control-Allow-Origin: *`
  - `Access-Control-Allow-Private-Network: true`（应对 Chrome 私有网络访问限制）
- 该端点本就是未认证可读状态，加 CORS 不扩大敏感信息暴露面。
- 仅给该只读状态端点加 CORS，不全局放开。

### 前端：跨网探测

修改 `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`：

1. 保存前记录探测目标：
   - 新增 `let staProbeMdnsUrl='';`
   - `saveWifiSta()` 提交前从当前 `/api/wifi-sta` 响应或保存响应 `state` 读取 `mdns_host`/`mdns_url`，构造 `staProbeMdnsUrl = 'http://<mdns_host>/api/wifi-sta'`（去掉末尾斜杠等规范化）。
2. 改造 `waitWifiStaConnectionResult()`：
   - 每轮**并行**发起两个请求：
     - `fetch('/api/wifi-sta')`
     - 若 `staProbeMdnsUrl` 非空，`fetch(staProbeMdnsUrl, {mode:'cors',cache:'no-store'})`
   - 用 `Promise.allSettled` 或先到先用：任一成功且 JSON 有 `connected && sta_ip && sta_ip!=='0.0.0.0'` 即采用。
   - 任一探测失败（断连/跨源被拦）不判失败，继续重试。
   - 拿到 IP 后：
     - `staNotice` 显示 STA IP 与 `http://<sta_ip>/`；
     - `refreshStatus()` 让状态卡显示；
     - handoff modal 显示访问地址。
   - 仅当 AP 侧明确返回 `last_error`/`timed_out` 才判失败。
   - 总等待时长保持约 60 秒。
3. 文案兜底：
   - 等待开始时提示：本机若连着有线，请保持本页面打开，连上后会自动尝试通过 mDNS 显示 STA IP；也可手动打开 `http://<mdns>.local/` 或在路由器查 IP。

## 数据流

1. 用户在 `192.168.4.1` 保存 STA（携带 scan channel）。
2. 前端记录 `staProbeMdnsUrl`。
3. 固件保存配置、预对齐切信道、连接 STA。
4. AP 断开，笔记本（带网线）不重连 AP；`/api/wifi-sta` 相对探测失败。
5. STA 连上路由器并启动 mDNS。
6. 前端经有线对 `http://<mdns>.local/api/wifi-sta` 探测成功，读到 `connected + sta_ip`。
7. 原页面显示 STA IP 与访问链接。

## 边界条件

- 浏览器/系统不支持 `.local` 解析或拦截跨源：探测一直失败，60 秒后回退引导文案；用户按提示手动访问。
- 笔记本有线与 STA 不在同一局域网：mDNS 不可达，回退文案。
- AP 恢复（无网线场景）：相对路径探测成功，走原路径显示 IP。
- 错误密码/找不到 SSID：AP 侧或经有线的 STA 侧不会变 connected；AP 侧返回 `last_error` 时提示失败。

## 测试计划

源码断言（`tests/test_firmware_feature_flags.py`）：

1. 后端 `handleWifiWebSta()` 含 `Access-Control-Allow-Origin` 与 `Access-Control-Allow-Private-Network` 头。
2. 前端存在 `staProbeMdnsUrl`。
3. `waitWifiStaConnectionResult()` 含对 `staProbeMdnsUrl` 的 `fetch` 探测且 `mode:'cors'`。
4. 探测失败不提前判失败（保留 null/异常重试逻辑）。
5. 显示 STA IP 文案与 `refreshStatus()` 调用保留。
6. 引导文案含 mDNS 提示。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -q
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -u
```

## 验收标准

1. 笔记本带网线、配网后 AP 不自动恢复时，页面仍能经有线探测到 STA IP 并显示。
2. 显示 STA IP 与可点击的 `http://<sta_ip>/`。
3. 跨源/`.local` 不可用时回退到清晰的 mDNS / 路由器引导文案。
4. 不破坏无网线场景下 AP 恢复后的原显示路径。
