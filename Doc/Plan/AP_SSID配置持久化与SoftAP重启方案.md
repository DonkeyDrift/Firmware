# AP SSID 配置持久化与 SoftAP 重启方案

## 背景

当前 Web Console 的 Network 卡片已经提供 AP / STA 两个 tab，齿轮按钮固定打开 STA Wi-Fi 配置。AP SSID 仍由固件常量 `MUS4-DEBUG` 提供，无法在 AP 模式下通过 Web Console 修改并持久化。

本方案目标是在 AP tab 下点击齿轮时弹出 AP SSID 配置界面，保存后写入 NVS，并只重启 SoftAP 使新 SSID 生效；STA tab 下齿轮继续打开现有 STA Wi-Fi 配置。

## 范围

### 本次实现

- AP tab 下点击 Network 齿轮打开 AP SSID 配置弹窗。
- STA tab 下点击 Network 齿轮保持现有 STA Wi-Fi 配置行为。
- 仅支持修改 AP SSID，不修改 AP 密码。
- AP SSID 持久化保存到 NVS。
- 保存成功后自动重启 SoftAP，不执行整机 `ESP.restart()`。
- Web API 权限与现有 STA 配置一致：Web 已 `AUTH` 或 Web `DEV MODE` 开启即可保存，不要求 Park Locked。

### 暂不实现

- 不新增 TCP/串口 AP 配置命令。
- 不支持 AP 密码修改。
- 不做浏览器侧复杂自动重连；保存成功后提示用户连接新 SSID 并刷新页面。

## 状态模型与持久化

新增运行时 AP SSID 状态：

- `WIFI_CONSOLE_AP_DEFAULT_SSID = "MUS4-DEBUG"`：默认 AP SSID。
- `WIFI_AP_SSID_MAX_LEN = 32`：AP SSID 最大长度。
- `char wifiApSsid[WIFI_AP_SSID_MAX_LEN + 1]`：当前运行时 AP SSID。
- `bool wifiApRestartPending`：是否等待重启 SoftAP。
- `unsigned long wifiApRestartDeadlineMs`：延迟重启时间点。

新增 NVS key：

- `ap_ssid`：持久化 AP SSID。

加载语义：

1. 启动时优先读取 NVS `ap_ssid`。
2. 若未保存、读取失败或内容无效，回退默认 `MUS4-DEBUG`。
3. `STATUS`、Network 卡片、SoftAP 启动和日志均使用运行时 `wifiApSsid`，不再直接使用固定 AP SSID 常量。

SSID 校验：

- 保存前执行 `trim()`。
- 长度必须为 `1..32`。
- 校验失败不写入 NVS，也不重启 AP。

## Web API

### `GET /api/wifi-ap`

公开只读，返回当前 AP 状态：

```json
{
  "ssid": "MUS4-DEBUG",
  "ip": "192.168.4.1",
  "clients": 1
}
```

### `POST /api/wifi-ap`

请求体使用 `application/x-www-form-urlencoded`：

```text
ssid=<新的 AP SSID>
```

处理流程：

1. 检查 Web 权限：已 `AUTH` 或 `DEV MODE` 开启。
2. 校验 SSID。
3. 保存到 NVS。
4. 返回成功 JSON。
5. 调度 SoftAP 延迟重启。

错误响应：

- `403 {"error":"auth_required"}`：未认证且未开启 DEV MODE。
- `400 {"error":"invalid_ssid"}`：SSID 为空或超过 32 字符。
- `500 {"saved":false}`：NVS 写入失败。

## 前端交互

Network 齿轮按钮改为调用 `openNetworkSettings()`：

- 当前 tab 为 AP 时，调用 `openWifiApModal()`。
- 当前 tab 为 STA 时，调用现有 `openWifiStaModal()`。

新增 AP 配置弹窗：

- 标题：`AP SSID 配置`。
- 字段：`SSID` 输入框，打开时填入当前 AP SSID。
- 提示：`保存后会重启 AP，当前浏览器连接会短暂断开。请连接新的 SSID 后刷新页面。`
- 按钮：`取消`、`保存并重启 AP`。

保存流程：

1. 点击保存后显示保存中状态，避免重复提交。
2. 调用 `POST /api/wifi-ap`。
3. 成功后显示 toast：`AP SSID 已保存，正在重启 AP`。
4. 关闭弹窗。
5. SoftAP 重启后当前浏览器连接可能断开；用户连接新 SSID 后刷新页面。

失败处理：

- `auth_required`：复用现有命令错误说明，引导先 `AUTH` 或开启 `DEV MODE`。
- `invalid_ssid`：弹窗内提示 `SSID 长度需为 1-32 字符。`
- 保存失败：显示错误 toast，并保持弹窗打开。

## SoftAP 重启流程

新增函数：

- `scheduleWifiApRestart()`：设置延迟重启标记。
- `restartWifiAp()`：执行 SoftAP stop/start。

`POST /api/wifi-ap` 保存成功后先返回 HTTP 响应，再将 `wifiApRestartPending` 设置为 true，并将 `wifiApRestartDeadlineMs` 设置为 `millis() + 800`。

`restartWifiAp()` 执行步骤：

1. 停止 captive DNS。
2. 断开现有 AP 客户端。
3. 调用 `WiFi.softAPdisconnect(true)` 关闭 AP。
4. 短延迟。
5. 确保 `WiFi.mode(WIFI_AP_STA)`。
6. 用 `wifiApSsid`、固定密码 `mus4-debug`、现有信道和最大客户端数调用 `WiFi.softAP(...)`。
7. 重新启动 captive DNS：`wifiCaptiveDnsServer.start(53, "*", WiFi.softAPIP())`。
8. 写入 Web 日志：`AP restarted ssid=<ssid>`。

与现有服务关系：

- `WebServer wifiWebServer` 不重新注册路由。
- TCP Console server 对象保持存在，当前客户端可能断开，后续可重新连接。
- 不主动断开 STA；若 Wi-Fi 栈短暂扰动 STA，由现有 `updateWifiSta()` 继续维护状态。
- 若 AP 重启失败，设置 `wifiConsoleStarted = false`，复用 `updateWifiConsole()` 的现有重试路径，并记录 `AP restart failed`。

## 测试计划

### 红灯测试

先更新 `tests/test_firmware_feature_flags.py`，增加源码断言：

- 存在 `openNetworkSettings()`。
- 存在 `openWifiApModal()`。
- 存在 `id="wifiApModal"`。
- 存在 `POST /api/wifi-ap`。
- 存在 `保存并重启 AP`。
- 齿轮按钮调用 `openNetworkSettings()`，不再直接调用 `openWifiStaModal()`。
- 状态输出和 Network 卡片使用运行时 AP SSID。

本轮不新增串口 AP 命令，因此不扩展 `wireless_console_policy.py` 的命令集合。

### 验证命令

```powershell
python -m pytest tests/test_firmware_feature_flags.py
python -m pytest tests/test_wireless_console_policy.py
.\arduino-cli-wsl.ps1 -Compile
```

## 发布收尾

若实现和验证通过：

- 递增 `BuildInfo.h` 的 `MUS4_FIRMWARE_VERSION`。
- 更新 `CHANGELOG.md`。
- 按项目约定可创建本地稳定版本提交。
- 不自动 push。