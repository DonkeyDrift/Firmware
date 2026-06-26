# CHANGELOG.md

## 2026-06-26 v1.7.25

- 固件版本号从 `v1.7.24` 更新到 `v1.7.25`。
- fix(OTA 稳定性): OTA 窗口打开或 HTTP OTA 上传开始时，设置 `OtaRuntimeState.closeWsPending` 标志；主循环在 `updateWifiWebSocket()` 中检测到该标志后关闭并发的 WebSocket 遥测连接，避免 WS 数据流与 OTA 传输挤占 AsyncTCP 资源导致上传中断。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言与 OTA 关闭 WebSocket 断言到 v1.7.25。

## 2026-06-26 v1.7.24

- 固件版本号从 `v1.7.23` 更新到 `v1.7.24`。
- fix(手柄校准浮窗): 修复 Web Console 中“开始校准/重试/保存”按钮在未认证时静默无响应的问题。前端现在会识别 `NACK:UNAUTHORIZED`，弹出 AP 密码输入框自动发送 `AUTH` 命令；若认证失败或 Park 未锁定，则通过 `showCommandError` 明确提示用户。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言与前端校准错误处理断言到 v1.7.24。

## 2026-06-26 v1.7.23

- 固件版本号从 `v1.7.22` 更新到 `v1.7.23`。
- feat(手柄/摇杆校准): 新增统一双轴（方向 + 油门）零位与正负最大值校准模块 `JoystickCalibration`，NVS 持久化，旧方向盘校准数据自动迁移，Drift Console 新增校准向导 UI 与 `/api/joystick-cal` 端点。
- feat(OTA 稳定性): 启动时自动检测并擦除状态为 `INVALID`/`ABORTED` 的 OTA 分区，避免上一次中断/失败的 OTA 遗留数据导致后续 OTA 稳定失败。
- refactor(Web Console 曲线区域): 将全屏/退出全屏按钮从底部工具栏移动到曲线画布右下角，并新增 `.chartCanvasWrap` 容器使其随画布缩放始终保持在该位置。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言到 v1.7.23。

## 2026-06-24 v1.7.22

- 固件版本号从 `v1.7.21` 更新到 `v1.7.22`。
- refactor(AP SSID 派生退役): AP/STA 自 v1.7.18 起已互斥切换（STA 上线后 AP 关闭），AP 与 STA 永远不会同时广播，历史在 STA 连接后给 AP SSID 追加「STA 短码 + IP 尾两段」的派生逻辑失去意义。
- **删除**：
  - `libraries/mus4_wifi/src/WifiManager.cpp` 中 `wifiStaSsidShortUpper()` / `wifiStaIpTailText()` / `buildWifiDevApSsid()` 三个 static 辅助函数；`getActiveWifiApSsid()` 简化为直接返回基础 `wifiApSsid`。
  - Web Console 中文与英文 AP 配置面板文案中「开启 DEV 模式且 STA 连接成功后，AP 名称会自动追加 STA SSID 前 3 位大写和 IP 后两段」的说明。
  - `tests/test_firmware_feature_flags.py::test_wifi_ap_ssid_prefix_is_limited_to_six_chars_with_dev_mode_suffix` 中对派生函数与文案的断言。
- `WIFI_AP_SSID_SUFFIX` (`"-ESP"`) 与 `WIFI_AP_SSID_PREFIX_MAX_LEN`（6 字符）常量保留——它们是 AP SSID 命名规则的基础，与派生无关。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言到 v1.7.22。

## 2026-06-24 v1.7.21

- 固件版本号从 `v1.7.20` 更新到 `v1.7.21`。
- fix(STA 实际连不上 → 全部 timeout): 实机验证 v1.7.20 后即便正确 SSID/密码也持续 timeout。串口日志显示路径走到 `[wifi] STA apply: switching to AP_STA` → `STA connecting:` → 15s 后 `STA failed: timeout`。根因：v1.7.18 把开机模式从历史的 `WIFI_AP_STA` 改为 `WIFI_AP`，`applyWifiStaCredentials` 每次都得做 `WIFI_AP → WIFI_AP_STA` 的反复切换；ESP-IDF 在这条切换路径上 STA netif 重建有 race，导致 `WiFi.begin()` 拿不到信道、握手永不开始。历史 v1.7.17 全程 `WIFI_AP_STA` 已验证 newhome_iot 等路由器可正常连接。
- **修复**：
  - `setupWifiConsole()` 开机模式改回 `WIFI_AP_STA`。互斥语义只在 `stopWifiApForStaOnly()` 落 STA-only 和 `restoreApAfterStaLost()` 回 AP-only 两个事件点切 mode；开机阶段如果 STA 未配置，STA 部分不会 begin，对外等效于 AP-only。
  - `applyWifiStaCredentials()` 在 `WiFi.mode(WIFI_AP_STA)` 之后插入 `delay(50)`，给 STA netif 留出初始化窗口；常态（已是 AP_STA）不触发 mode 切换，没有额外开销。
  - `wifiInApOnlyMode` 初始化对齐开机模式（开机 = `WIFI_AP_STA` → false）；`updateWifiConsole()` 的 retry 闸门改为「STA 未在线即可重试」，覆盖开机 AP 起不来和 AP-only 下 AP 异常两种情况。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `test_apply_wifi_sta_credentials_restores_ap_before_begin` 新增 `delay(50)` 断言。
  - `setupWifiConsole` 中 `WiFi.mode(WIFI_AP_STA)` 出现位置断言更新。
  - 版本号断言更新到 v1.7.21。

## 2026-06-24 v1.7.20

- 固件版本号从 `v1.7.19` 更新到 `v1.7.20`。
- fix(STA 失败不回 AP): 实机验证发现在 STA-only 状态下保存错误密码，15 秒后 STA timeout 但**设备并未回到 AP-only**——`updateWifiSta()` 的三个失败分支（`WL_NO_SSID_AVAIL` / `WL_CONNECT_FAILED` / connect timeout）只调用 `setWifiStaLastError()` 把 `staConnecting=false`，**没有触发任何模式切换**。结果设备卡在 `WIFI_AP_STA` 但 SoftAP 又已经在 `stopWifiApForStaOnly()` 阶段被 `softAPdisconnect(true)` 关掉，同时 ESP32 内置的 STA 自动重连还在后台用错密码反复重试。
- **修复**：三条失败路径（`no_ssid` / `auth_failed` / `timeout`）写完 `lastError` 后立即调 `restoreApAfterStaLost()` 切回 `WIFI_AP` 并 `startWifiApServices()`，与 down-grace 后的 `sta_lost` 路径收敛到同一刀。`restoreApAfterStaLost()` 内部 `esp_wifi_disconnect()` 把自动重连关掉，避免 RF 调度持续被错密码扰动。
- `restoreApAfterStaLost()` 签名由 `(bool withErrorReason)` 改回无参——错误码 / 日志文案改由 4 处调用方按场景写入（`sta_lost` / `no_ssid` / `auth_failed` / `timeout` / `WIFI_STA_CLEAR`），收敛点更清晰。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `restoreApAfterStaLost(bool)` → `restoreApAfterStaLost()` 签名断言更新。
  - 新增 `test_update_wifi_sta_failure_paths_restore_ap` 保护 4 条失败 / 清除路径都调 `restoreApAfterStaLost()`。
  - 版本号断言更新到 v1.7.20。

## 2026-06-24 v1.7.19

- 固件版本号从 `v1.7.18` 更新到 `v1.7.19`。
- fix(STA 应用时丢失 AP 兜底): 实机验证发现在 STA-only 状态下保存一个错误的 STA 密码会让设备彻底失联（AP 不广播，STA 失败也不恢复 AP）。根因：v1.7.18 的 `applyWifiStaCredentials()` 只在 `wifiInApOnlyMode=true` 时才 `WiFi.mode(WIFI_AP_STA)`，STA-only 状态下保持 `WIFI_STA`、且 SoftAP 已被 `softAPdisconnect(true)` 关停。此时 `WiFi.begin()` 失败后 `updateWifiSta()` 走的是「未连接 → 等待 connect timeout」分支，**没有任何路径恢复 AP**——`restoreApAfterStaLost()` 仅在曾经 `wifiStaConnected=true` 后断链时触发。
- **修复**：`applyWifiStaCredentials()` 改为：发起 `WiFi.begin()` 前先无条件确认 `WiFi.getMode()==WIFI_AP_STA` 且 `WiFi.softAPIP()!=0.0.0.0`；若任一不满足，立即 `WiFi.mode(WIFI_AP_STA)` + `startWifiApServices("AP restored for STA apply")`。这样 STA 连接失败时 AP 仍是兜底入口，与「STA 正在尝试连接期间 AP 保留」的设计一致。
- 串口烧写恢复路径：当 AP/STA 都不可达时，可走 USB 串口（COM20/COM21）的 `python arduino-cli.py -u -i build_wsl/MUS4_FW.ino.bin --port COMxx` 重新烧写。

## 2026-06-23 v1.7.18

- 固件版本号从 `v1.7.17` 更新到 `v1.7.18`。
- refactor(wifi 生命周期): AP+STA 长期共存稳定性差（共享 RF / 内部调度冲突，表现为 Web Console 卡顿、TCP Console 掉线、WebSocket 推送被抢占），把 `WIFI_AP_STA` 改为**AP/STA 互斥切换**：
  - **STA 进入 `WL_CONNECTED` 后等待 `WIFI_STA_GRACE_UP_MS=1000ms`**，由 `updateWifiSta()` 调用新增的 `stopWifiApForStaOnly()` 主动 `WiFi.softAPdisconnect(true)` + `WiFi.mode(WIFI_STA)`，落地为 STA-only。
  - **STA 脱离 `WL_CONNECTED` 后等待 `WIFI_STA_GRACE_DOWN_MS=1000ms`**，由 `updateWifiSta()` 调用新增的 `restoreApAfterStaLost(bool)` 切回 `WIFI_AP` 并 `startWifiApServices()`，恢复 AP-only；grace 期间链路恢复则取消重启。
  - **STA 正在尝试连接（未确认成功）期间 AP 仍保留**，避免连接失败时把用户踢出。
  - **AP 模式下不再后台轮询重连 STA**，必须用户在 AP 页面重新保存或重连。
  - **STA→STA 切换**（旧设计的 `wifiStaHandoff*` 三态共存）退役：`startWifiStaHandoff` / `finishWifiStaHandoff` / `clearWifiStaHandoff` 改为 no-op，新 SSID 由 `applyWifiStaCredentials()` 走「短暂回到 `WIFI_AP_STA` → 1s grace 后切 STA_ONLY」的统一链路；JSON `handoff_*` 字段保留以兼容前端解析，`handoff_active` 永远为 false。
  - `setupWifiConsole()` 开机直接进入 `WIFI_AP`，仅在 `wifiStaConfigured` 时由 `applyWifiStaCredentials()` 切回 `WIFI_AP_STA`。
  - `updateWifiConsole()` 不再在 STA-only 状态下周期重启整个 console（否则会把 AP 又拉起来破坏互斥）。
  - 新增 `WifiRuntimeState` 字段 `staUpGraceDeadlineMs` / `staDownGraceDeadlineMs` / `inApOnlyMode`，与之配套的 extern 别名在 `MUS4_FW.ino` 中补齐。
- Web Console STA Modal 文案微调：`AP 保持开启` → `AP 将在 1 秒后关闭，请用新 IP 继续`。
- fix(web console gating): `updateWebConsoleServer()` 的 `if (!ws.consoleStarted) return;` 闸门改为 `if (!ws.consoleStarted && !ws.staConnected) return;`——v1.7.18 互斥切换下 `wifiConsoleStarted` 的语义聚焦到「AP 服务是否就绪」，STA-only 状态下它会被 `stopWifiApForStaOnly()` 置 false，但 HTTP 必须继续在 STA 接口响应，否则浏览器通过 STA IP 访问会被立刻 TCP RST。本次先用最小改动恢复服务面，后续可考虑把 `wifiConsoleStarted` 进一步拆成 `apServicesReady` / `webServerRunning` 两个独立标志。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `test_web_console_keeps_ap_running_after_successful_wifi_sta_connection` 重写为 `test_web_console_closes_ap_after_sta_grace`，断言 `stopWifiApForStaOnly` 体内 `WiFi.softAPdisconnect(true)` + `WiFi.mode(WIFI_STA)` + `WIFI_STA_GRACE_UP_MS=1000`。
  - `test_sta_disconnect_keeps_soft_ap_clients_connected_and_services_available` 重写为 `test_sta_disconnect_restores_ap_after_grace`，断言 `restoreApAfterStaLost(bool)` 体内 `WiFi.mode(WIFI_AP)` + `startWifiApServices` + grace deadline 武装。
  - `test_soft_ap_disconnect_is_limited_to_explicit_ap_restart` 放宽：`softAPdisconnect(true)` 出现两次（restart / stopForStaOnly），允许 `WiFi.mode(WIFI_STA)` 出现。
  - `test_wifi_mdns_lifecycle_follows_sta_connection` / NetBIOS / LLMNR 改为断言停止动作在 `restoreApAfterStaLost` 体内。
  - 文案断言更新到 `AP 将在 1 秒后关闭`；版本号断言更新到 v1.7.18。

## 2026-06-21 v1.7.17

- 固件版本号从 `v1.7.16` 更新到 `v1.7.17`。
- fix(WebSocket race 收尾): 上一刀 v1.7.16 把 `mus4LogLine` 从 AsyncTCP task 撤出，但 `sendWifiWebSocketHello` 在 `WS_EVT_CONNECT` 回调里仍然在 AsyncTCP task 上写**同一个**共享 `static String wifiWebSocketPayload`，与 main loop 上的 `sendWebLogToSocket` 写同一个 String 并发 realloc —— race 没消除，bad magic 与 reboot 复现。
- **修复**：
  - **彻底删掉** `static String wifiWebSocketPayload` —— 不再有跨函数/跨上下文共享的 text 缓冲。
  - `sendWifiWebSocketHello(uint32_t clientId)` 与 `sendWebLogToSocket(...)` 都改为在函数体内声明**栈上局部 `String payload`** 并 `reserve()`，Arduino `String::operator+=` 的 realloc 只触碰本函数私有堆块。
  - `handleWifiWebSocketEvent::WS_EVT_CONNECT` 不再调 `sendWifiWebSocketHello`；改翻新增的 `volatile uint32_t pendingWsConnectClientId` 与原有 `pendingWsConnectEvent` 标志。
  - `updateWifiWebSocket()` 在 main loop 里消费 `pendingWsConnectEvent` 时**先 `sendWifiWebSocketHello(pendingWsConnectClientId)` 再 `mus4LogLine("web", "ws connected")`** —— 所有 text JSON 写入永远只在 main loop 单一上下文发生。
  - `setupWifiWebSocket` 删去无用的 `wifiWebSocketPayload.reserve(1536)`。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `test_websocket_event_callback_does_not_invoke_log_sink_in_async_task` 新增 `handleWifiWebSocketEvent` 内**不得**出现 `sendWifiWebSocketHello(` 的负断言，以及 `updateWifiWebSocket` 体内**必须**出现 `sendWifiWebSocketHello(` 的正断言。
  - 新增 `test_websocket_text_payloads_never_share_a_static_string`：禁止 `static String wifiWebSocketPayload` 与 `wifiWebSocketPayload.reserve` 出现；强制 `sendWifiWebSocketHello` / `sendWebLogToSocket` 体内出现 `String payload`。
  - 版本号断言更新到 v1.7.17。

## 2026-06-21 v1.7.16

- 固件版本号从 `v1.7.15` 更新到 `v1.7.16`。
- fix(WebSocket race / heap 腐蚀): 实机上 v1.7.15 部署后浏览器 Web Console 报 `ws parse error: Error: bad magic` + 设备周期性 reboot（`[3632][web] ws connected` 时间戳回零）。Explore 子代理静态分析定位到 main loop 与 AsyncTCP task 之间对 `wifiWebSocketPayload` 这个 `static String` 的无锁并发写：
  - `handleWifiWebSocketEvent` 在 `WS_EVT_CONNECT` / `WS_EVT_DISCONNECT` 里直接调 `mus4LogLine("web", "ws connected/disconnected")`，sink 在 AsyncTCP task 上下文写共享 String；同时 main loop 上的 `appendWebLog`（T..S.. / M:P 等）也在 sink 这条路径写同一个 String。两个上下文并发 realloc 撕裂堆元数据 → AsyncTCP 内部 `_queueMessage` 拿到的 message buffer 内容/opcode 被踩 → 浏览器解码到前 2 字节非 `'M','4'` 的"binary"帧（bad magic）→ 不久 `std::__throw_bad_alloc` 或 LoadProhibited 触发 panic reboot。
  - 二次风险：`pushWifiWebSocketData` / `sendWebLogToSocket` 持裸 `wifiWebSocketClient` 指针并 deref，AsyncTCP task 上 `WS_EVT_DISCONNECT` 把指针置 nullptr 之间存在 TOCTOU，叠加 `cleanupClients()` 真正 free 客户端对象后是 use-after-free。
- **修复**：
  - `libraries/mus4_web/src/WebTelemetry.cpp::handleWifiWebSocketEvent` 不再调 `mus4LogLine`；新增 `volatile bool pendingWsConnectEvent` / `pendingWsDisconnectEvent` 标志，由 AsyncTCP task 翻起，main loop 在 `updateWifiWebSocket()` 里读到后再 `mus4LogLine`。sink (`sendWebLogToSocket`) 现在永远只在 main loop 单一上下文运行。
  - 全部走 id 路径：`sendWifiWebSocketHello(uint32_t clientId)`、`sendWebLogToSocket`、`pushWifiWebSocketData` 改用 `wifiWebSocket.text(id, ...)` / `wifiWebSocket.binary(id, ..., len)`，让 ESPAsyncWebServer 内部 `_ws_clients_lock` 兜底 dangling client；不再持 `wifiWebSocketClient->...` 调用。
  - `pushWifiWebSocketData` 入口的 `canSend() || queueIsFull()` 检查改为只调 `wifiWebSocket.availableForWrite(id)`（同样锁下检查 client 存活 + 队列容量），消除裸指针 deref。
- 同步更新 `tests/test_firmware_feature_flags.py`：新增 `test_websocket_event_callback_does_not_invoke_log_sink_in_async_task` 与 `test_websocket_send_paths_use_id_not_raw_client_pointer`；版本号断言更新到 v1.7.16。

## 2026-06-21 v1.7.15

- 固件版本号从 `v1.7.14` 更新到 `v1.7.15`。
- fix(稳定性): 排查实机上 v1.7.14 部署后再次出现的 `Failed to fetch`（`/api/sta`、`/api/data`、`/api/log` 三 API 同时无响应）+ `ws disconnect/connect` 循环 + 设备周期性自重启（`[3440][web] ws connected` 时间戳回零）现象，定位到两个叠加因素并修复：
  - 根因 A：v1.7.13 的 100Hz `$IMU` 帧用 `String("$IMU,") + ... + String(x, 4)` 拼装，每秒约 900 次堆 `malloc/free`，十几秒后堆碎片化与 AsyncTCP 内部 PCB tx queue 抢资源 → 某次 `malloc` 失败触发 AsyncWebSocket 异常 → ws 断连风暴 → 最终 OOM 重启。**修复**：`MUS4_FW.ino` 改为 `char imuBuf[96]` + `snprintf` + `Serial1.write` 一次写入，每帧零堆分配。
  - 根因 B：60Hz `T..S..` 通过 `appendWebLog → sendWebLogToSocket` 每秒推 60 条 JSON 到浏览器 WS，叠加 ~60Hz 曲线二进制帧后顶满 AsyncWebSocket 8 槽队列（即使 v1.7.14 已经把 `$IMU` 不入 web log）。**修复**：新增 `TELEM_WEB_LOG_INTERVAL_MS=100`（10Hz），通过 `lastTelemWebLogMs` 节流 T..S.. 写 Web 日志的频率；Serial1 上行给上位机仍是 60Hz，HTTP `/api/log?source=serial1` 的 64 条环形缓冲不受影响，前端日志窗口实际可见的 T..S.. 由约 ~每秒 60 条降到 ~10 条。
- 同步更新 `tests/test_firmware_feature_flags.py::test_serial1_uplink_matches_host_pilot_protocol` 新增 `char imuBuf[96]` / `snprintf` / `Serial1.write` 三项正向断言，`appendWebLog("serial1", imuBuf)` 负断言，与 `lastTelemWebLogMs` / `TELEM_WEB_LOG_INTERVAL_MS` 节流断言；版本号断言更新到 v1.7.15。

## 2026-06-21 v1.7.14

- 固件版本号从 `v1.7.13` 更新到 `v1.7.14`。
- fix(web ws 稳定性): 排查 v1.7.13 上车后 Web Console 持续出现的 `ws disconnected` / `ws connected` 循环和曲线卡顿：
  - 根因 A：100Hz 的 `$IMU` 行被同时镜像到 `appendWebLog("serial1", imuLine)`，经 `sendWebLogToSocket` 包成 ~90 字节 JSON 推到浏览器 WS，配合 60Hz `T..S..` 与曲线二进制帧顶爆 AsyncWebSocket 发送队列（默认 8 条），触发 `queueIsFull()` → 主动断连 → 浏览器重连 → 再次堵塞，1–3s 一轮。修复方式：`$IMU` 不再写 Web 日志，Web Console 通过 WebSocket 二进制 schema v2 的 `latest` 区获取 IMU（v1.7.11 已实装的 `gx/gy/ax/ay/az`）。`T..S..` / `M:P` 仍保留日志旁路。
  - 根因 B：mDNS / NetBIOS / LLMNR 三种主机名发现协议在弱 Wi-Fi 下的多播查询风暴 + mDNS 每 60s 周期重启会和 AsyncWebSocket 抢资源。v1.7.14 起 `FirmwareConfig.h` 新增 `DISABLE_WIFI_NAME_DISCOVERY` 总开关并默认启用：`startWifiMdnsIfNeeded()` 首行短路返回，`ENABLE_WIFI_NETBIOS_DISCOVERY` / `ENABLE_WIFI_LLMNR_DISCOVERY` 由该开关 gating（默认不再定义，对应 NetBIOS/LLMNR 包处理函数体在编译期被剪掉）。`wifiMdnsStarted` 恒为 false，因此 `WifiManager.cpp` 末尾的 60s mDNS 周期重启块自然不触发，无须额外改动。STATUS / `/api/sta` 中的 `mdns_host` / `mdns_url` / `mdns_started` 字段保留，Web UI 网络面板表现为 `mdns_started=0` 与空 `mdns_url`，便于将来注释掉 `DISABLE_WIFI_NAME_DISCOVERY` 一行恢复。
- 同步更新 `tests/test_firmware_feature_flags.py` 新增 `test_wifi_mdns_startup_short_circuits_when_name_discovery_disabled` 与对 `appendWebLog("serial1", imuLine)` 的负断言；改写 `test_wifi_discovery_compile_switches_exist` 验证 NetBIOS/LLMNR 受 `DISABLE_WIFI_NAME_DISCOVERY` gating；版本号断言更新到 v1.7.14。

## 2026-06-21 v1.7.13

- 固件版本号从 `v1.7.12` 更新到 `v1.7.13`。
- feat(Serial1 协议): 对齐上位机 DonkeyCar 真车 101 的 `ArdImu` / `Arduino` part 与 GRU drift pilot 推理链路：
  - **MANUAL 上行人工油门/转向帧**：`T<t>:S<s>\n` → `T<t>S<s>\n`（去掉历史冒号分隔符），匹配上位机 `Arduino` part 的正则解析。
  - **MANUAL 上行新增 `M<m>:P<p>\n`**：m∈{0,1,2}（MANUAL/SEMI/FULL），p∈{0,1}（UNLOCKED/LOCKED），状态变化时立即发，否则 1Hz 心跳；新增 `MODE_PARK_HEARTBEAT_MS=1000`。
  - **所有模式上行新增 `$IMU,seq,ts_ms,ax,ay,az,gx,gy,gz\n`**：MPU6050 6 轴 m/s²+rad/s（由 `Adafruit_MPU6050` 直接产出，无 ESP32 端二次换算），seq 用 `uint16_t` 自然回绕仅作丢帧检测，无校验；新增 `IMU_TELEMETRY_INTERVAL_MS=10`（~100Hz）；MPU 不在线（`mpu6050Data.valid==false`）时静默不发。
  - **OTA 闸门复用 `shouldEmitSerial1Telemetry(otaRuntime)`**：三类上行帧在 OTA 真正传输期间一并暂停，避免与 OTA 抢占 UART；OTA 结束自动恢复。下行 `<thr>:<str>[:seq][*CRC]` 解析不变。
- 同步更新 `wireless_console_policy.py` 新增 `format_serial1_manual_frame` / `format_serial1_mode_park_frame` / `format_imu_telemetry_line` 三个镜像格式化函数（桌面侧 Tub 回放、单元测试无需启动固件即可拼出与 ESP32 一致的字节流）。
- 同步更新 `tests/test_firmware_feature_flags.py::test_serial1_telemetry_has_dedicated_web_log_buffer`、新增 `test_serial1_uplink_matches_host_pilot_protocol` 与 `test_wireless_console_policy_mirrors_serial1_uplink_format`；`tests/test_wireless_console_policy.py` 新增 6 项镜像格式化测试；190 项 pytest 全绿。

## 2026-06-21 v1.7.12

- 固件版本号从 `v1.7.11` 更新到 `v1.7.12`。
- feat(web tub): 前端 `TUB_SCHEMA` 从 `mus4.web_data_point.tub.v1` 升级到 **v2**，显式宣告 Tub JSON 字段集合扩展，避免下游训练脚本误把 v1（缺 IMU 五轴）与 v2 混在同一批次。
- feat(tools train): `tools/train_tub_driver.py::PREFERRED_FEATURE_ORDER` 在 `gzf` 之后追加 `gx/gy/ax/ay/az`，GRU baseline 默认特征列含完整 IMU 五轴；`DEFAULT_EXCLUDE_COLUMNS` 保持不变（新字段是真实物理观测，非泄漏列）。
- 同步更新 `tests/test_firmware_feature_flags.py::test_tub_schema_bumps_to_v2_with_imu_five_axes` 与 `tests/test_train_tub_driver.py::test_preferred_feature_order_contains_imu_five_axes`、`test_select_feature_columns_includes_imu_five_axes_when_present` 及版本号断言。

## 2026-06-21 v1.7.11

- 固件版本号从 `v1.7.10` 更新到 `v1.7.11`。
- feat(web 遥测 WS): WebSocket 二进制遥测帧升级到 schema **v2**，在 `latest` 区 `gz` 之后追加 `gx/gy/ax/ay/az` 五个 float32。前端 `decodeBinaryDataPayload` 同步把版本校验从 `version!==1` 改为 `version!==2`，并解出新字段写入 `latest`，确保 WS 路径下 `tp(latest)` 也能让 Tub 录制拿到完整 IMU 通道。
- `wifiWebSocketBinaryPayload` 缓冲扩容 `256 → 384` B：header+latest v2 ≈100 B + 8 个点 × 24 B = 192 B 合计 ≈292 B 已突破原 256 B 上限，扩到 384 留出余量。
- 同步更新 `tests/test_firmware_feature_flags.py::test_websocket_binary_frame_schema_v2_carries_imu_five_axes` 与版本号断言。

## 2026-06-21 v1.7.10

- 固件版本号从 `v1.7.9` 更新到 `v1.7.10`。
- feat(web 遥测): `/api/data` 的 `latest` 对象新增 `gx`/`gy`/`ax`/`ay`/`az` 五个 IMU 缩写键，三位小数与现有 `gz` 精度一致；polling 路径下浏览器 `tp(latest)` 写入 `tubSamples` 后下载的 `mus4-tub.json` 立即携带漂移建模所需通道。`/api/data` 的 plot 点数组未扩，避免每帧广播放大。
- 同步更新 `tests/test_firmware_feature_flags.py::test_http_api_data_latest_exposes_imu_five_axes` 与版本号断言。

## 2026-06-21 v1.7.9

- 固件版本号从 `v1.7.8` 更新到 `v1.7.9`。
- refactor(web 遥测): `WebDataPoint` 扩展承载 IMU 五轴 (`gyroX/gyroY/accelX/accelY/accelZ`)，`sampleWifiWebData` 把 `mpu6050Data` 已采样的五轴一并写入；HTTP / WS / Tub 三条对外链路本刀暂不暴露新字段，仅做后端缓冲铺垫，对外行为零变化。
- 同步更新 `tests/test_firmware_feature_flags.py::test_web_data_point_carries_imu_five_axes_for_tub_export` 与版本号断言。

## 2026-06-21 v1.7.8

- 固件版本号从 `v1.7.7` 更新到 `v1.7.8`。
- 消除 DEV 模式对 Serial1 遥测的副作用：`shouldEmitSerial1Telemetry` 仅在 OTA 真正传输期间（`os.inProgress=true`）暂停 Serial1，DEV ON 时窗口长期打开不再阻塞 ESP32 与上位机通信。Park Guard 仍由 `forceWifiOtaParkLocked()` 在传输期内托底。
- 消除 DEV 模式对 AP 广播 SSID 的影响：`getActiveWifiApSsid()` 派生只看 STA 是否已连接，与 `wifiDevModeEnabled` 解耦；STA 连接后 AP 始终广播 `<前缀>-ESP-<STA短码>-<STA IP尾段>`（如 `MU03-ESP-HUA-3.43`），无论 DEV 开关状态。`saveDevModePreference()` 不再调用 `scheduleWifiApRestart()`，切换 DEV 不再丢一次 AP/Web Console 连接。
- 同步更新 `wireless_console_policy.py::should_emit_serial1_telemetry` 与 `tests/test_wireless_console_policy.py::test_serial1_telemetry_pauses_only_during_active_transfer`、`tests/test_firmware_feature_flags.py` 中 Serial1/AP SSID 相关断言。

## 2026-06-21 v1.7.7

- 固件版本号从 `v1.7.6` 更新到 `v1.7.7`。
- 修复 `libraries/mus4_web/src/WebConsoleServer.cpp` 中 `String::toUpperCase()` 在表达式拼接处的编译错误（ESP32 Arduino core 3.x 起返回 `void`），同步修正 `tests/test_firmware_feature_flags.py:497` 的源码断言。
- 收敛 DEV 模式安全边界：`isWirelessCommandAllowed` 重排序，DEV ON 仅显式放权 OTA + Web 配置 + 显示/日志切换 + WIFI_STA_*；控制命令与诊断命令（`Throttle:Steering`、`TEST`、`BENCH`、`REGRESS`、`STEER_CAL*`）严格要求认证；同步修正 `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py::test_web_dev_mode_does_not_bypass_authentication_for_control_or_diagnostic`。
- `processWirelessConsoleLine` 的 NACK 分流同步收敛：未认证用户（即使 DEV ON）一律返回 `NACK:UNAUTHORIZED`，不再返回 `NACK:PARK_REQUIRED`。
- 新增 `docs/Plan/DEV模式影响面与运行逻辑映射.md`，记录 DEV 开关在 v1.7.7 实现下的事实映射、放权清单、执行链路与历史偏差收敛过程。

## 2026-06-12 v1.7.6

- 固件版本号从 `v1.7.4` 更新到 `v1.7.6`。
- Web Console 屏保激活延时调整为 60 秒。
- Web Console 串口界面发送按钮与日志暂停按钮交换位置。
- Web Console 绘图区暂停、清空、全屏图标按钮上移至图例行左侧，与 Throttle / Steering / GyroZ 同处一行。

## 2026-06-11 v1.7.4

- 固件版本号从 `v1.7.3` 更新到 `v1.7.4`。
- 完成安全关键模块拆分：将 RC PWM 输入捕获迁入 `RcPwmCapture.h/.cpp`。
- 完成控制融合模块拆分：将驾驶模式切换、RC/Pilot 混控、Drift Assist 迁入 `ControlMixer.h/.cpp`。
- 完成安全状态机拆分：将 Park 状态机、紧急制动 FSM 迁入 `SafetyState.h/.cpp`。
- 完成执行器输出拆分：将 PWM 映射、限幅、`ledcWriteChannel` 迁入 `ActuatorOutput.h/.cpp`。
- `MUS4_FW.ino` 从 ~3700 行收敛到 ~556 行，缩减 85%。
- 清理死代码：`rise_time[]`、`lastParkState`、`adj()`、`MOTOR_OFFSET_V`/`SERVO_OFFSET_V`、波形数组、`counter`。
- 同步更新 `tests/test_firmware_feature_flags.py` 源码断言（75 项）与 `AGENTS.md` 模块清单。
- 更新 `Doc/Plan/MUS4_FW模块化拆分方案.md` 至 3.0 修订稿，标记全部计划内切片已完成。
- **修复 HTTP OTA 上传可靠性**：
  - `WebConsoleServer.cpp`：新增 query parameter `?auth=` 一次性认证，摆脱全局 session 依赖；将 OTA 错误消息从单一 `NACK:UPDATE_FAILED` 细化为 `NACK:AUTH_REQUIRED`/`PARK_REQUIRED`/`BEGIN_FAILED`/`WRITE_FAILED`/`END_FAILED`/`ABORTED`，便于诊断根因。
  - `arduino-cli-wsl.ps1`：上传前自动预检（`AUTH` + `ENABLE_OTA`）；curl 增加 `--connect-timeout 10`、`--max-time 180`、`--retry 2`、`--retry-delay 3`、`--retry-connrefused`，解决大文件在慢 Wi-Fi 下因 ESP32 5 秒超时断开导致的上传失败。

## 2026-06-10 v1.7.3

- 固件版本号从 `v1.7.2` 更新到 `v1.7.3`。
- 将 Web Console 的 Serial Log 显示区域限制为最多 16 行。
- 将 Web Console 屏保激活延时调整为 60 秒。

## 2026-06-10 v1.7.2

- 固件版本号从 `v1.6.3` 更新到 `v1.7.2`。
- 将 Web Console 品牌文案从 `DonkeyDrift Console` 调整为 `Drifter Console`。
- 将语言与帮助入口折叠为右下角单个发光圆点，点击后径向展开，减少默认遮挡数据区域。
- 保留语言选择持久化与中英文核心界面文案切换。

## 2026-06-07 v1.6.0

- 固件版本号从 `v1.5.23` 更新到 `v1.6.0`。
- 新增 Web Console AP tab 下的 AP SSID 配置弹窗，保存后持久化到 NVS。
- 保存 AP SSID 后自动重启 SoftAP，使新 SSID 无需整机重启即可生效。
- Network 齿轮按钮按当前 AP/STA tab 分流，STA tab 继续打开原 STA Wi-Fi 配置。

## 2026-06-05 v1.5.23

- 固件次版本号从 `v1.5.22` 更新到 `v1.5.23`。
- 将 Web Console 的 `RC Channels` 初始状态改为折叠，减少顶部状态区默认占用高度。
- 优化 `STATUS Details` 展开布局，宽屏显示 3 列，中等宽度 2 列，窄屏 1 列。

## 2026-06-05 v1.5.22

- 固件次版本号从 `v1.5.21` 更新到 `v1.5.22`。
- 为 Web Console 的 RC 通道与 STATUS 详情新增 `▸` / `▾` 折叠展示，STATUS 折叠时只保留标题。
- 将 Web Console 的 STATUS 文本展开视图改为 key/value 列表，并保留 `build` 等带空格引号值的完整内容。

## 2026-06-05 v1.5.21

- 固件次版本号从 `v1.5.20` 更新到 `v1.5.21`。
- 将 Web Console 顶部开发模式开关文案从 `DEBUG MODE` 调整为 `DEV MODE`，并同步认证失败提示。
- 收窄 Web Console 的 `MODE` 与 `PARK` 状态卡片到 `flex:0.30`，同时保留原字体大小与内边距。
- 调整 Web Console 标题行底边对齐，使版本号文字与 `MUS4 Web Console` 标题底边对齐。

## 2026-06-01 v1.5.20

- 固件次版本号从 `v1.5.19` 更新到 `v1.5.20`。
- 修复 STA 断开或重连时运行时断开操作扰动 SoftAP，导致 AP 需要多次重试才能连接的问题。
- 为 Windows 连通性探测提供本地 DNS 捕获和 `/connecttest.txt`、`/ncsi.txt` 响应，降低系统因“无 Internet”自动断开 MUS4-DEBUG AP 的概率。

## 2026-06-01 v1.5.19

- 固件次版本号从 `v1.5.18` 更新到 `v1.5.19`。
- Web Console 保存 STA Wi-Fi 后会等待连接结果；连接失败时在页面内悬浮窗显示原因和处理建议。
- 扩展 STA 状态输出，新增连接中状态、失败原因码和失败原因说明，便于 Web Console 与命令行排障。

## 2026-06-01 v1.5.18

- 固件次版本号从 `v1.5.17` 更新到 `v1.5.18`。
- 调整 DEBUG MODE 权限策略：Web Console 操作免 AUTH，但仍保留 Park Locked 安全限制，并在非 Park 或未授权时给出明确弹窗引导。
- 同步更新无线权限策略镜像测试，覆盖 STA 修改、OTA 和控制命令的开发模式免认证行为。

## 2026-06-01 v1.5.17

- 固件次版本号从 `v1.5.16` 更新到 `v1.5.17`。
- 将 Web Console 右上角开发模式开关标签从 `Auto OTA` 改为 `DEBUG MODE`，使其更准确表达开关含义。

## 2026-06-01 v1.5.16

- 固件次版本号从 `v1.5.15` 更新到 `v1.5.16`。
- 修复 Web Console 保存 STA Wi-Fi 配置时立即重连可能中断当前 HTTP 请求，导致浏览器提示 `Failed to fetch` 的问题。

## 2026-06-01 v1.5.15

- 固件次版本号从 `v1.5.14` 更新到 `v1.5.15`。
- 修复 Web Console 的 STA Wi-Fi 配置弹窗在用户输入 SSID 后离焦时，周期刷新可能把输入值覆盖为当前保存值或编译默认值的问题。

## 2026-05-30 v1.5.14

- 固件次版本号从 `v1.5.13` 更新到 `v1.5.14`。
- 新增 Web Console Tub JSON 连续记录与浏览器下载功能，便于采集 CH1-CH6 等遥测样本交给模型分析。
- 压缩 Tub 控件文案，降低 Web Console HTML 体积，规避 HTTP OTA 接近分区末尾写入失败。

## 2026-05-30 v1.5.13

- 固件次版本号从 `v1.5.12` 更新到 `v1.5.13`。
- 保留当前已验证的 WebSocket 曲线实时显示能力，便于通过 HTTP OTA 发布当前稳定固件。
