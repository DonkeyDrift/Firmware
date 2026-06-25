# AP 配网 STA 保留窗口与 BOOT 长按恢复设计

## 背景

当前 MUS4 Web Console 支持通过设备 AP 提交 STA SSID/密码。设计上，若用户从 AP 页面完成 STA 配网，STA 成功连接并拿到 IP 后，AP 应继续保留 `WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS = 60000`，让原 AP 页面有足够时间轮询 `/api/wifi-sta` 并显示 STA IP。

实机现象是：通过 AP 配置 STA 后，AP 很快断开，前端无法显示 STA IP。代码检查显示前端只在 `location.hostname === '192.168.4.1'` 时提交 `source=ap`；若用户从 captive portal、AP 主机名、`.local` 或其它 AP 入口访问，后端会收到 `source=sta`，从而只使用普通 `WIFI_STA_GRACE_UP_MS = 1000`，AP 约 1 秒后关闭。

同时新增物理救援能力：长按 ESP32 BOOT 键超过 3 秒，清除 STA 配置并恢复 AP，避免配错 STA 或 STA 网络不可达时必须重新烧录/串口介入。

## 目标

1. 从 AP 页面保存 STA 配置并成功连接后，AP 至少保留 60 秒，保证前端能显示/复制 STA IP。
2. 长按 BOOT(GPIO0) 超过 3 秒后，清除已保存 STA 配置并恢复 AP 服务。
3. 保持 AP/STA 互斥切换的大方向：普通开机自动 STA 连接或 STA 侧重配不应无条件延长 AP 保留窗口。
4. 遵循现有模块边界，优先在 `mus4_wifi` / `mus4_web` 内做最小改动。

## 非目标

- 不修改 AP 名称、AP 密码、DEV 模式、OTA 密码或转向标定存储。
- 不改变控制输出、Park 状态机、OTA Park Guard。
- BOOT 长按不执行整机恢复出厂，不清除全部 NVS。
- 默认不调用 `ESP.restart()`；触发后直接恢复 AP。

## 方案

### AP 配网来源判定

`handleWifiWebStaSet()` 保留前端 `source` 参数，但不再完全依赖它。后端新增 AP 配网语义判定：

- 若 `source == "ap"`，按 AP 配网处理。
- 若当前 SoftAP 仍在线、STA 尚未 connected，并且本次保存会触发新的 STA apply，也按 AP 配网处理。
- 只有明确处于已连接 STA 场景且前端传 `source=sta` 时，才使用普通 `WIFI_STA_GRACE_UP_MS`。

判定结果写入 `wifiStaApplyFromAp`。`updateWifiSta()` 已根据该标志选择：

```cpp
wifiStaUpGraceDeadlineMs = millis() + (wifiStaApplyFromAp ? WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS : WIFI_STA_GRACE_UP_MS);
```

因此实现重点是确保 AP 配网页面无论通过 `192.168.4.1`、captive portal 还是 AP 主机名访问，后端都会设置 `wifiStaApplyFromAp = true`。

### BOOT 长按恢复

在 Wi-Fi 模块新增运行时轮询：

- 常量：
  - `WIFI_RECOVERY_BUTTON_PIN = 0`
  - `WIFI_RECOVERY_LONG_PRESS_MS = 3000`
- 初始化：`pinMode(WIFI_RECOVERY_BUTTON_PIN, INPUT_PULLUP)`。
- 轮询：低电平表示按下；连续低电平超过 3 秒且本轮未触发时执行恢复。
- 松手后重置触发锁，允许未来再次长按。

触发动作：

1. 调用 `clearWifiStaPreference()` 清除 NVS 中 STA 配置和运行时 STA 状态。
2. 停止 STA 连接/重连。
3. 清除 STA apply/grace 状态。
4. 确保 AP 服务可用，用户可以重新连接设备 AP 配网。
5. 写日志：`STA cleared by BOOT long press`。

为避免误触，短按 BOOT 不做任何事。GPIO0 是 ESP32 boot strap 引脚，本设计只在固件运行时读取，不改变启动行为。

## 数据流

### 正常 AP 配网成功

1. 用户连接设备 AP，打开 Web Console。
2. 前端 `POST /api/wifi-sta` 提交 SSID/密码。
3. 后端保存 NVS，判定为 AP 配网，设置 `wifiStaApplyFromAp = true`。
4. `scheduleWifiStaApply()` 延迟 800ms。
5. `applyWifiStaCredentials()` 保持/恢复 AP 兜底并发起 STA 连接。
6. STA 拿到有效 IP 后，`updateWifiSta()` 将 AP 关闭时间设为当前时间 + 60 秒。
7. 前端轮询 `/api/wifi-sta`，显示 STA IP。
8. 60 秒后 `stopWifiApForStaOnly()` 关闭 SoftAP，WebServer 继续在 STA 接口监听。

### BOOT 长按恢复

1. 用户长按 BOOT 超过 3 秒。
2. 固件检测到长按事件。
3. 清除 STA 配置并断开 STA。
4. 恢复 AP 服务。
5. 用户重新连接 AP 并重新配网。

## 错误处理

- 清除 STA 配置失败时记录日志；若运行时仍处于 STA-only 或 STA 连接尝试中，仍优先恢复 AP 作为救援入口，但不得误报“已清除”。
- AP 服务恢复失败时沿用现有 `startWifiApServices()` / `ensureWifiApAvailable()` 的日志与重试路径。
- 若 BOOT 长按发生在 STA 连接尝试期间，应取消 pending apply 与 grace，避免下一轮又自动连接旧 STA。
- 若 BOOT 长按发生在 STA-only 状态，应恢复 SoftAP 并重新启动 Web/TCP/DNS 服务。

## 测试计划

先补源码断言测试，再实现：

1. `tests/test_firmware_feature_flags.py` 新增/调整断言：
   - `WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS = 60000` 保持存在。
   - Web STA 保存路径不只依赖 `location.hostname === '192.168.4.1'` 来决定 AP 配网 grace。
   - `handleWifiWebStaSet()` 中存在后端 AP 配网判定，并写入 `wifiStaApplyFromAp`。
   - 存在 `WIFI_RECOVERY_BUTTON_PIN = 0`、`WIFI_RECOVERY_LONG_PRESS_MS = 3000`。
   - 存在 `pinMode(..., INPUT_PULLUP)`。
   - BOOT 长按处理调用 `clearWifiStaPreference()` 并恢复 AP。
2. 运行：
   - `pytest tests/test_firmware_feature_flags.py -k "wifi"`
3. 固件编译：
   - `./arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`

## 验收标准

1. 从 AP 页面保存正确 STA 后，前端能在 AP 未断开前看到非 `0.0.0.0` 的 STA IP。
2. AP 在 AP 配网成功后保留约 60 秒，然后关闭。
3. 长按 BOOT 超过 3 秒后，STA 配置被清除，设备 AP 恢复可连接。
4. 短按 BOOT 不清除 STA。
5. STA 失败路径仍恢复 AP，且不会卡在无 AP/无 STA 状态。
