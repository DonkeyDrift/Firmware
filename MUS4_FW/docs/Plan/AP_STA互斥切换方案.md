# AP / STA 互斥切换方案（v1.7.18）

## 背景

v1.7.17 及以前固件长期采用 `WIFI_AP_STA` 双模共存：开机 SoftAP 常驻，STA 配置存在时再附加连接，STA 上线后 AP 仍不关闭。该设计的好处是 AP 始终是兜底维护入口，但实测发现 AP+STA 共存时稳定性显著下降——共享 RF 资源与内部调度冲突会引发 Web Console 卡顿、TCP Console 掉线、WebSocket 推送被抢占。本方案把生命周期改为**互斥切换**，由两个去抖锚点驱动。

## 目标行为

- **STA 进入 `WL_CONNECTED` 后等待 1 s** → 关 AP，落地 `WIFI_STA`（STA-only）。
- **STA 脱离 `WL_CONNECTED` 后等待 1 s** → 起 AP，回 `WIFI_AP`（AP-only）。
- **STA 正在尝试连接（未确认成功）期间 AP 保留**，避免连接失败把用户踢出。
- **AP 模式下不自动后台轮询重连** STA：用户必须从 AP 页面手动重连或重新保存。
- **STA→STA 切换**走「保存新配置 → applyWifiStaCredentials 短暂回到 AP_STA → 1s grace 后切 STA-only」统一链路，不再维护旧的三态共存 handoff。

## 状态机

```
[BOOT_AP] ──save/apply STA──▶ [AP_STA_TRYING]
                                  │
                                  ├─WL_CONNECTED──▶ [AP_STA_GRACE(1s)] ──▶ [STA_ONLY]
                                  │
                                  └─fail/timeout──▶ [BOOT_AP] (保留 lastError, AP 已在)
[STA_ONLY] ──!WL_CONNECTED 持续 1s──▶ [BOOT_AP]
[STA_ONLY] ──user save new STA──▶ [AP_STA_TRYING] (applyWifiStaCredentials 切回 AP_STA)
[STA_ONLY] ──WIFI_STA_CLEAR──▶ [BOOT_AP] (restoreApAfterStaLost(false))
```

两个核心去抖锚点：
- `WifiRuntimeState::staUpGraceDeadlineMs`：STA 首次进入 `WL_CONNECTED` 时武装；到期由 `stopWifiApForStaOnly()` 关 AP。
- `WifiRuntimeState::staDownGraceDeadlineMs`：STA 首次脱离 `WL_CONNECTED` 时武装；到期由 `restoreApAfterStaLost(true)` 起 AP；grace 内链路恢复则清零，保持 STA_ONLY。

辅助字段：
- `WifiRuntimeState::inApOnlyMode`：标记当前是否落在 `WIFI_AP`（用于驱动 `applyWifiStaCredentials` 在发起新连接前先切到 `WIFI_AP_STA`，以及让 `updateWifiConsole` 在 STA_ONLY 状态下停止周期重启 console）。

常量（`libraries/mus4_core/src/WifiConsoleTypes.h`）：
- `WIFI_STA_GRACE_UP_MS = 1000`
- `WIFI_STA_GRACE_DOWN_MS = 1000`

## 关键函数（`libraries/mus4_wifi/src/WifiManager.cpp`）

| 函数 | 角色 |
| --- | --- |
| `setupWifiConsole()` | 开机直接 `WiFi.mode(WIFI_AP)`；启动 AP 服务；若 `wifiStaConfigured` 则调 `applyWifiStaCredentials()` 进入 AP_STA_TRYING。 |
| `applyWifiStaCredentials()` | 发起 STA 连接；若当前在 AP-only 先切 `WIFI_AP_STA`；清零两个 grace 锚点。 |
| `stopWifiApForStaOnly()` | STA up grace 到期：停 captive DNS、`softAPdisconnect(true)`、`WiFi.mode(WIFI_STA)`；置 `wifiInApOnlyMode = false`、`wifiConsoleStarted = false`。 |
| `restoreApAfterStaLost(bool withErrorReason)` | STA down grace 到期或 WIFI_STA_CLEAR：停 mDNS/NetBIOS/LLMNR、`esp_wifi_disconnect()`、`WiFi.mode(WIFI_AP)`、`startWifiApServices(...)`；置 `wifiInApOnlyMode = true`；`withErrorReason=true` 时写入 `sta_lost` 错误。 |
| `updateWifiSta()` | 主状态机：分别处理 WL_CONNECTED / 已断开（武装 down grace）/ 连接中（超时与失败码）三条分支。 |
| `updateWifiConsole()` | STA_ONLY 状态下不再周期重启 console（否则 AP 会被拉回来）。 |
| `restartWifiAp()` | AP SSID 修改入口：按 `wifiStaConnected` 决定切到 `WIFI_AP_STA` 还是 `WIFI_AP`。 |
| `startWifiStaHandoff` / `finishWifiStaHandoff` / `clearWifiStaHandoff` | 退化为 no-op，仅清残留字段；保留是为了不打散 `WebConsoleServer.cpp` / `wifiStaJson` 的调用面，前端 `handoff_active` 永远为 false。 |

## 兼容性与边界

- `/api/wifi-sta` JSON 中 `handoff_*` 字段保留，`handoff_active=false` 固定；前端 `wifiStaHandoffModal` 不会再被触发，相关 dead code 保留以缩小 PR。
- `restartWifiAp()` 仍是唯一显式 `softAPdisconnect(true)` 的入口之一；`stopWifiApForStaOnly()` 是另一处。
- 实机若发现 1 s grace 抖动过多，可调 `WIFI_STA_GRACE_DOWN_MS` 至 2000；这是单常量改动。
- OTA / Park Guard 逻辑与 AP/STA 状态机解耦，不需要随之改动；`shouldEmitSerial1Telemetry(otaRuntime)` 仍由 OTA `inProgress` gating。

## 验证方法

1. **静态**：`pytest tests/test_firmware_feature_flags.py tests/test_wireless_console_policy.py`，所有用例必须绿（v1.7.18 已通过 138/138）。
2. **编译**：`.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`。
3. **实机场景**（OTA 上传后）：
   - A：冷启动无 STA → 仅 AP，`http://192.168.4.1/` 可达；TCP `2323` 可登录。
   - B：冷启动有有效 STA → AP_STA_TRYING；连接成功 1s 后日志 `AP stopped after STA connected`，AP SSID 消失，浏览器跳 STA IP 成功。
   - C：STA 突然断网 → ~1s 后日志 `AP restored after STA lost`，AP 重新出现，浏览器可连 `192.168.4.1`；`/api/wifi-sta` 的 `last_error` 为 `sta_lost`。
   - D：AP 下用户重新保存 → 复现 B 链路。
   - E：STA→STA 切换 → 自动回到 AP_STA_TRYING，新 STA 成功 1s 后切 STA_ONLY。
   - F：STA 短暂抖动（<1s）→ AP 不会被启动（grace 内恢复），日志 `STA recovered within grace window`。
