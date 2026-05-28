# WebSocket 曲线数据通道实施计划

## Context

当前 Web 曲线通过前端短轮询 `/api/data?since=...` 获取数据，`req` 反映的是浏览器 HTTP 请求往返和 JSON 解析耗时，因此受 Wi‑Fi、HTTP 请求调度、ESP32 同步 `WebServer` 处理和返回点数量影响，波动明显。用户已明确要求采用 WebSocket 方案，以减少短轮询开销并让曲线数据通道更连续。

目标是在不替换现有 HTTP Web Console、不影响控制主循环和安全路径的前提下，为曲线遥测新增只读 WebSocket 通道；保留 `/api/data` 作为 fallback 和诊断接口。

## 推荐方案

### 1. 依赖与服务端端口

- 在 WSL Arduino CLI 环境安装 `ESP Async WebServer` 与 `Async TCP` 库，使用 `AsyncWebSocket`。
- 在 `mus4.ino` 中新增 WebSocket telemetry 编译开关和 include。
- 保留现有 `WebServer wifiWebServer(WIFI_WEB_CONSOLE_PORT)` 处理页面、状态、命令、日志、OTA 相关 HTTP API。
- 新增独立 WebSocket 服务：独立 `AsyncWebServer wifiWebSocketServer(81)` 与 `AsyncWebSocket wifiWebSocket("/")`。
- 仅将 `ESP Async WebServer` 用于独立 81 端口的 WebSocket telemetry，不替换现有 `WebServer` HTTP 路由和安全逻辑。

### 2. 服务端数据结构与限流

修改 `mus4.ino`：

- 在现有 Wi‑Fi 常量附近增加：
  - `WIFI_WEB_SOCKET_PORT = 81`
  - `WIFI_WEB_SOCKET_PUSH_INTERVAL_MS = 32`
  - `WIFI_WEB_SOCKET_MAX_CLIENTS = 1`
  - `WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME = 8`
  - `WIFI_WEB_SOCKET_MIN_FREE_HEAP`
- 在现有 `WebDataPoint` 环形缓冲附近增加 WebSocket 状态：客户端连接状态、客户端 id、最后发送 seq、丢点计数、上次推送时间、复用的 `String` payload 缓冲。
- 保持 `sampleWifiWebData()` 的 16ms 采样不变。
- WebSocket 推送按 32ms 限频，每帧最多 8 个点；backlog 过大时丢弃旧点，只保留最近数据，避免阻塞控制 loop。
- OTA 进行中只维护 WebSocket loop，不推送 telemetry，降低 OTA 期间网络和 heap 压力。
- free heap 过低时跳过本次 telemetry 推送。

### 3. JSON 复用与 HTTP fallback

- 抽取现有 `/api/data` 中轻量曲线点 JSON 拼接逻辑为 `appendWifiWebPlotPointJson(String&, WebDataPoint&)`。
- `handleWifiWebData()` 继续返回原有 `{points, latest}` 格式，但内部复用新 helper。
- WebSocket data frame 也使用同一 helper，并继续用现有 `appendWifiWebStateJson()` 输出完整 `latest` 状态。
- `/api/data` 不删除，供 WebSocket 不可用时 fallback。

### 4. WebSocket 事件与只读安全边界

新增函数：

- `setupWifiWebSocket()`：启动 81 端口、注册事件、预留 payload 缓冲、记录日志。
- `handleWifiWebSocketEvent(...)`：处理连接、断开、极少量只读文本消息。
- `pushWifiWebSocketData()`：按限流规则发送 telemetry frame。
- `updateWifiWebSocket()`：调用 `wifiWebSocketServer.loop()` 并推送 telemetry。

安全约束：

- WebSocket 只传 telemetry，不承载控制命令。
- 只接受 `ping` 和可选 `since:<seq>`；其他文本忽略或返回只读错误。
- 不调用 `processWirelessConsoleLine()`、`openWifiOtaWindow()`、任何控制输出函数或 Wi‑Fi 配置函数。
- 现有 `/api/cmd`、认证、Park 锁定、Dev Mode、OTA 权限逻辑保持不变。

### 5. 接入现有 setup/update 流程

修改 `mus4.ino`：

- 在 `setupWifiWebConsole()` 中，现有 HTTP routes 和 `wifiWebServer.begin()` 后调用 `setupWifiWebSocket()`。
- 在 `updateWifiWebConsole()` 中保留：
  - `sampleWifiWebData()`
  - `wifiWebServer.handleClient()`
  - 然后调用 `updateWifiWebSocket()`。
- 在 `printWirelessStatus()` 末尾追加诊断字段：`ws_port`、`ws_client`、`ws_dropped`，不改现有字段名。

### 6. 前端改造

修改 `WIFI_WEB_CONSOLE_HTML` 内嵌 JS：

- 新增 WebSocket 状态变量：连接对象、连接状态、重连计时、退避延迟、最后消息时间。
- 抽取 `handleDataPayload(j, transport, elapsed)`，让 HTTP fallback 和 WebSocket 共用数据处理逻辑。
- 新增 `connectDataSocket()`：
  - 连接 `ws://location.hostname:81/`。
  - `onopen` 后标记 WebSocket 在线，可发送 `since:lastDataSeq`。
  - `onmessage` 解析 `hello` / `data`，`data` 进入 `handleDataPayload(..., 'ws', 0)`。
  - `onclose/onerror` 后按 0.5s、1s、2s、5s 上限重连。
- 修改 `pollData()` 为 fallback：WebSocket 已连接时不再轮询 `/api/data`；WebSocket 断开时继续现有短轮询。
- 初始化时先 `connectDataSocket()`，约 1.2s 后若未连接再启动 `pollData()`。
- 曲线绘制逻辑保持现有 `dt`、`req`、Throttle、Steering、GyroZ 兼容；WebSocket 模式下 `req` 可显示为 `0` 或仅状态栏标记 `ws`，HTTP fallback 时继续显示真实请求耗时。

## Critical files

- `mus4.ino`
  - include 区：新增 `AsyncTCP.h` 与 `ESPAsyncWebServer.h`。
  - Wi‑Fi 常量/全局状态区：新增 WebSocket 配置和状态。
  - `sampleWifiWebData()`：保持不变。
  - `appendWifiWebStateJson()` / `handleWifiWebData()`：抽取轻量 point JSON helper 并复用。
  - `setupWifiWebConsole()` / `updateWifiWebConsole()`：接入 WebSocket setup 和 loop。
  - `printWirelessStatus()`：追加 WebSocket 诊断字段。
  - 内嵌 HTML/JS：新增 WebSocket 连接、fallback 和共用 payload 处理。
- `wslbuild.yaml`
  - 当前 `sync_libs: false`，因此依赖应直接安装到 WSL Arduino CLI 环境。
  - 需要使用 ESP32Async 维护的 `ESP Async WebServer` / `Async TCP`；若旧叉库 `ESPAsyncWebServer` / `AsyncTCP` 已安装，Arduino CLI 会优先选中旧目录并导致 ESP32 Core 3.x 编译失败，需要卸载旧叉库。

## 验证计划

1. 安装依赖：
   ```powershell
   wsl -d DKC ~/bin/arduino-cli lib uninstall "ESPAsyncWebServer" "AsyncTCP"
   wsl -d DKC ~/bin/arduino-cli lib install "ESP Async WebServer@3.11.0" "Async TCP@3.4.10"
   ```

2. 编译验证：
   ```powershell
   .\arduino-cli-wsl.ps1 -Compile
   ```

3. 编译通过后按已授权流程 OTA：
   ```powershell
   .\arduino-cli-wsl.ps1 -Upload -Ota -OtaHost 192.168.3.140
   ```

4. 页面验证：
   - 打开设备 Web Console。
   - 确认曲线正常滚动。
   - `dataMeta` 显示 WebSocket 模式，例如 `ws seq=...`。
   - 浏览器 Network 中 `/api/data` 不再持续高频请求，除非 WebSocket fallback。

5. fallback 验证：
   - 临时让前端连接错误端口或断开 WebSocket。
   - 确认页面自动回退到 `/api/data`，曲线不中断。

6. 回归验证：
   ```powershell
   pytest tests/test_wireless_console_policy.py -v
   pytest tests/test_arduino_cli.py -v
   ```

7. 安全和稳定性验证：
   - `PING`、`STATUS`、`AUTH`、`ENABLE_OTA`、`OTA_STATUS` 行为不变。
   - OTA 期间仍强制 Park Locked。
   - WebSocket 不接受控制命令。
   - 观察 10–30 分钟，确认无 watchdog reset、heap 持续下降或明显控制卡顿。

## 回滚策略

- 保留 `/api/data` 和前端 fallback，因此 WebSocket 异常时页面仍可工作。
- 若现场出现 WebSocket 兼容或资源问题，可关闭 WebSocket 编译开关并重新编译 OTA，恢复短轮询通道。
