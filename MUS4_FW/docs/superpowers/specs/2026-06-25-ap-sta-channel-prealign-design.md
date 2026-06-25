# AP 配 STA 前预对齐信道以避免 AP 客户端断联设计

## 背景

用户实机确认：通过设备 AP 给 STA 配网时，手机/电脑会从 MUS4 AP 掉线，需要重新连接 AP 后才能查看 STA IP。此前已实现 60 秒 AP/STA 共存 grace，但仍不能避免掉线。

根因不只是 `stopWifiApForStaOnly()` 的关闭时机。ESP32 是单射频，AP+STA 共存时 SoftAP 与 STA 必须工作在同一 Wi-Fi 信道。如果设备 AP 当前在固定 `WIFI_CONSOLE_CHANNEL = 6`，而目标路由器在其它信道，`WiFi.begin()` 成功关联目标路由器时，ESP32 会把 SoftAP 跟随 STA 切到目标信道，已连接 AP 的手机/电脑可能被踢下线。因此即使固件没有主动关闭 AP，客户端也会经历 Wi-Fi 层断联。

用户已选择方案 A：在发起 STA 连接前，把设备 AP 预先对齐到目标 STA 网络的信道，减少 STA 成功瞬间的被动信道迁移，从而让 AP 页面保持连接并显示 STA IP。

## 目标

1. 通过 AP 页面选择并保存 STA 网络后，STA 连接前尽量把 SoftAP 信道预先切到目标路由器信道。
2. STA 成功后 AP 与 STA 保持同信道共存 60 秒，Web 页面能显示 STA IP。
3. 保留错误密码/找不到 SSID 后恢复 AP 的兜底路径。
4. 不改变 BOOT 长按清除 STA 的行为。
5. 不引入长期 AP+STA 共存；60 秒后仍关闭 AP。

## 非目标

- 不保证所有客户端在 AP 信道重启期间零丢包；如果必须重启 SoftAP，可能有一次短暂抖动。
- 不修改 AP 密码、认证策略、OTA 策略。
- 不重构整个 Wi-Fi 生命周期状态机。

## 设计

### 前端：保存扫描到的目标信道

`/api/wifi-sta/scan` 已返回：

```json
{ "ssid": "...", "rssi": -50, "channel": 11, "secure": true }
```

前端在用户选择 SSID 时保存信道：

- 新增状态变量：`staSelectedChannel`。
- `selectWifiSsid(ssid, channel)` 保存 SSID 和 channel。
- 扫描列表按钮调用 `selectWifiSsid(n.ssid, n.channel)`。
- 手动输入 SSID 时将 `staSelectedChannel` 清零。
- `saveWifiSta()` 若 `staSelectedChannel` 在 1..14，则提交 `channel=<value>`。

### 后端：接收并保存本次 apply 的目标信道

`handleWifiWebStaSet()` 读取表单参数：

```cpp
int channel = wifiWebServer.arg("channel").toInt();
```

若为 AP 配网路径，且 channel 在 1..14，则记录到运行时状态，例如：

```cpp
wifiStaTargetChannel = channel;
```

如果没有合法 channel，保持 0，表示未知。

目标信道是一次 apply 的运行时信息，不需要写入 NVS。STA 凭据仍按现有 `saveWifiStaPreference()` 持久化。

### Wi-Fi apply：STA begin 前预对齐 SoftAP 信道

在 `applyWifiStaCredentials()` 中，`WiFi.begin()` 之前执行：

1. 若 `wifiStaApplyFromAp == true`；
2. 且 `wifiStaTargetChannel` 在 1..14；
3. 且 SoftAP 当前在线；
4. 则用目标 channel 重启 SoftAP 服务。

可新增辅助函数：

```cpp
bool restartWifiApOnChannel(uint8_t channel);
```

该函数复用现有 AP 启动逻辑，但允许指定 channel，而不是固定 `WIFI_CONSOLE_CHANNEL`。为避免全局改动过大，可以让 `startWifiApServices()` 接收可选 channel，或新增 `startWifiApServicesOnChannel()`。

关键要求：

- 重启 AP 应发生在 `WiFi.begin()` 之前；
- 不调用 `WiFi.mode(WIFI_OFF)`；
- 不清除 STA 配置；
- 不影响失败恢复路径；
- 预对齐后仍设置 `wifiStaApplyFromAp = true`，STA 成功后使用 60 秒 grace。

### 状态与清理

- 新增运行时字段或模块内变量：`wifiStaTargetChannel`。
- 每次 `applyWifiStaCredentials()` 开始前消费该值；成功或失败路径可清零。
- `restoreApAfterStaLost()` / `stopWifiApForStaOnly()` 清理该值，避免影响下一次 STA apply。

## 数据流

1. 用户连接 MUS4 AP。
2. Web Console 扫描 Wi-Fi，得到目标 SSID 的 channel。
3. 用户选择 SSID，前端保存 `staSelectedChannel`。
4. 用户点击保存，前端提交 SSID、密码、source、channel。
5. 后端保存 STA 配置，并记录本次目标 channel。
6. `scheduleWifiStaApply()` 延迟执行。
7. `applyWifiStaCredentials()` 在 `WiFi.begin()` 前把 SoftAP 重启到目标 channel。
8. STA 连接目标路由器；由于 AP 已在同信道，客户端不应在 STA 成功瞬间被信道迁移踢下线。
9. STA 获得 IP 后，AP+STA 共存 60 秒，页面显示 STA IP。
10. 60 秒后关闭 SoftAP，进入 STA-only。

## 边界条件

- 如果用户手动输入 SSID，无法知道 channel：不做预对齐，仍走现有 60 秒 grace 与重连提示。
- 如果扫描 channel 非法或缺失：忽略 channel。
- 如果目标路由器在连接过程中自动换信道，仍可能断联；这超出固件可控范围。
- 如果 AP 重启到目标 channel 本身导致短暂断线，应优先保证后续 STA 成功阶段不再二次断线。
- 如果 STA 连接失败，恢复 AP 时使用默认或当前 AP 配置，保持现有兜底入口。

## 测试计划

先补源码断言，再实现：

1. 前端断言：
   - 扫描按钮调用 `selectWifiSsid(n.ssid,n.channel)`。
   - 存在 `staSelectedChannel`。
   - `saveWifiSta()` 提交 `body.set('channel', ...)`。
2. 后端断言：
   - `handleWifiWebStaSet()` 读取 `wifiWebServer.arg("channel")`。
   - 合法 channel 记录到运行时变量。
3. Wi-Fi 状态机断言：
   - 存在 `restartWifiApOnChannel` 或等价函数。
   - `applyWifiStaCredentials()` 在 `WiFi.begin()` 前调用该函数。
   - 该函数调用 `WiFi.softAP(..., channel, ...)`。
   - `restoreApAfterStaLost()` / `stopWifiApForStaOnly()` 清理目标 channel。
4. 保留已有测试：
   - AP 配网成功后 60 秒 grace。
   - STA 失败恢复 AP。
   - BOOT 长按恢复 AP。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -k "wifi_sta_channel or ap_sta_configuration_keeps_ap_open_long_enough_to_show_ip"
pytest tests/test_firmware_feature_flags.py
pytest tests/test_wireless_console_policy.py
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -u
```

## 验收标准

1. 通过 AP 页面选择扫描到的 STA 网络并保存后，AP 客户端不应在 STA 成功瞬间被踢下线。
2. STA 成功后，AP 页面能显示 STA IP。
3. AP 与 STA 共存约 60 秒。
4. 60 秒后 AP 关闭，STA IP 继续可访问。
5. 错误密码/目标 SSID 不存在时，AP 仍可恢复用于重新配网。
