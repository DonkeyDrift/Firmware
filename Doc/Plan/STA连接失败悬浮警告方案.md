# STA 连接失败悬浮警告方案

## 背景

MUS4 Web Console 已支持在页面中配置 STA Wi-Fi，并通过 `/api/wifi-sta` 查询 `configured`、`connected`、`timed_out`、`ssid`、`password_set`、`ap_ip`、`sta_ip` 等状态。当前体验存在两个问题：

1. 用户点击“保存并连接”后，页面无法明确等待连接结果；保存请求成功并不代表 STA 已连接成功。
2. STA 连接失败时，页面只能看到卡片处于 pending 或 timed out，缺少醒目的失败提示与原因说明。

本方案目标是在 Web Console 内提供页面悬浮窗警告：当用户保存 STA 配置并尝试连接后，如果连接不成功，应在页面内弹出悬浮窗，说明失败原因与可操作建议。

## 范围

### 本次纳入

- 扩展固件 STA 状态模型，记录连接中状态与最后失败原因。
- 扩展 `/api/wifi-sta` 与 `WIFI_STA_STATUS` 输出，保持旧字段兼容。
- Web Console 在“保存并连接”后等待连接结果，失败时显示页面内悬浮警告窗。
- 增加 Python 镜像测试与源码形态测试，保护状态契约和前端行为。

### 本次不纳入

- 不改变 AP+STA 启动模式。
- 不改变 Wi-Fi 凭据持久化键名与权限策略。
- 不使用浏览器原生 `alert()` 作为 STA 失败提示。
- 不尝试在 ESP32 上扫描周边 SSID 或做复杂网络诊断。

## 数据模型

固件在现有 STA 字段基础上新增轻量状态：

- `wifiStaConnecting`：当前是否处于用户触发的 STA 连接等待期。
- `wifiStaLastError`：最后一次 STA 连接失败原因码，空字符串表示无错误。
- `wifiStaLastErrorMessage`：面向 Web Console 展示的中文原因说明。

推荐原因码：

| 原因码 | 触发条件 | 用户提示 |
| --- | --- | --- |
| `timeout` | 连接超过 `WIFI_STA_CONNECT_TIMEOUT_MS` 仍未成功 | STA 连接超时，请检查 SSID、密码与路由器信号。 |
| `no_ssid` | `WiFi.status()` 返回 `WL_NO_SSID_AVAIL` | 未找到目标 SSID，请检查网络名称或距离。 |
| `auth_failed` | `WiFi.status()` 返回可判定认证失败或连接失败的状态 | STA 认证失败，请检查 Wi-Fi 密码。 |
| `connect_failed` | 其他非成功失败状态 | STA 连接失败，请检查路由器状态。 |

ESP32 Arduino 对失败原因的细粒度支持有限；无法可靠区分时优先给出保守、可操作的通用提示。

## API 契约

`GET /api/wifi-sta` 保持现有字段，并新增：

```json
{
  "configured": true,
  "connected": false,
  "timed_out": true,
  "connecting": false,
  "last_error": "timeout",
  "last_error_message": "STA 连接超时，请检查 SSID、密码与路由器信号。",
  "ssid": "HomeWiFi",
  "password_set": true,
  "ap_ip": "192.168.4.1",
  "sta_ip": "0.0.0.0"
}
```

`WIFI_STA_STATUS` 文本输出追加 `connecting`、`last_error` 与 `last_error_message`，用于串口、TCP Console 与 Web Console 命令保持一致。旧解析方仍可继续读取原有字段。

## Web Console 交互

点击“保存并连接”后的流程：

1. 前端提交 `POST /api/wifi-sta`。
2. 请求成功后清空密码输入，但不立即认为连接成功。
3. 前端启动等待循环，最长等待 `WIFI_STA_CONNECT_TIMEOUT_MS` 对应的 15 秒左右。
4. 轮询 `GET /api/wifi-sta`：
   - `connected=true`：关闭 STA 配置悬浮窗，刷新 STA 卡片。
   - `last_error` 非空或 `timed_out=true`：显示页面内失败悬浮窗。
   - 仍在 `connecting=true`：继续等待。
5. 失败悬浮窗内容包含：
   - 标题：`STA 连接失败`
   - SSID：目标网络名称
   - 原因：`last_error_message`
   - 建议：检查 SSID、密码、路由器距离或重新保存。

悬浮窗复用现有 `.modal` / `.dialog` 样式，新增独立 DOM 节点与关闭按钮。该提示必须是 Web Console 页面内悬浮窗，不使用浏览器 `alert()`。

## 固件状态流

### 发起连接

`applyWifiStaCredentials()` 在调用 `WiFi.begin()` 前：

- 设置 `wifiStaConnecting=true`。
- 设置 `wifiStaConnected=false`。
- 设置 `wifiStaTimedOut=false`。
- 清空 `wifiStaLastError` 与 `wifiStaLastErrorMessage`。
- 记录 `wifiStaConnectStartMs`。

### 连接成功

维护函数检测到 `WiFi.status() == WL_CONNECTED` 时：

- `wifiStaConnected=true`。
- `wifiStaConnecting=false`。
- `wifiStaTimedOut=false`。
- 清空最后失败原因。

### 连接失败

维护函数检测到明确失败状态或超时时：

- `wifiStaConnected=false`。
- `wifiStaConnecting=false`。
- `wifiStaTimedOut=true`（超时场景）。
- 设置 `wifiStaLastError` 与 `wifiStaLastErrorMessage`。
- 写入 Web 日志，便于排障，但不得记录明文密码。

### 清除配置

`clearWifiStaPreference()` 清空连接中状态与失败原因，避免旧错误继续显示。

## 测试计划

遵循 TDD 顺序，先补失败测试，再实现。

1. `tests/test_wireless_console_policy.py`
   - 新增 STA 状态格式化镜像函数或扩展现有测试，覆盖 `connecting`、`last_error`、`last_error_message`。
   - 覆盖连接成功时错误字段为空。
   - 覆盖超时时错误原因可读。

2. `tests/test_firmware_feature_flags.py`
   - 断言 Web Console HTML 包含 STA 失败悬浮窗节点。
   - 断言 `saveWifiSta()` 启动等待连接结果逻辑。
   - 断言 STA 失败提示不依赖浏览器 `alert()`。

3. 验证与上传命令

```powershell
python -m pytest tests/test_wireless_console_policy.py tests/test_firmware_feature_flags.py
.\arduino-cli-wsl.ps1 -Compile
.\arduino-cli-wsl.ps1 -Upload -HttpOta -HttpOtaHost 192.168.3.144
```

测试与编译通过后，默认通过 HTTP OTA 上传到当前 STA 地址 `192.168.3.144`；不自动执行串口上传或串口监视。

4. 实机验证

- 上传后打开 `http://192.168.3.144/` 进入 Web Console。
- 输入错误密码，点击“保存并连接”，确认 15 秒左右出现页面内悬浮窗。
- 输入不存在 SSID，确认出现“未找到目标 SSID”或保守失败提示。
- 输入正确凭据，确认不弹失败窗，STA 卡片显示 connected 与 STA IP。
- 清除 STA 后，确认旧失败原因不再显示。

## 边界条件

- Web Console 页面连接的是设备 AP，STA 失败不应影响 AP 页面继续可用。
- 保存请求成功但连接失败时，不回滚已保存凭据；用户可直接修改后重试。
- 开发模式只影响 Web Console 认证放宽，不影响 STA 失败判定。
- 密码为空表示开放网络；密码长度非法仍在保存阶段返回 `invalid_password`。
- 如果 ESP32 无法提供精确失败状态，弹窗必须使用保守原因，避免误导用户。

## 自检结论

- 无占位符或待定项。
- 方案保持现有 API 字段兼容，仅追加字段。
- 方案聚焦 STA 连接失败提示，不引入扫描、路由器诊断等额外范围。
- 失败原因采用保守分类，避免把无法可靠判断的原因说成确定事实。
