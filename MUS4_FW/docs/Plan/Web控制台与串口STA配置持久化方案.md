# Web 控制台与串口 STA 配置持久化方案

> **历史方案（v1.7.x 早期）**：本文写于 AP+STA 长期共存设计下，描述中的「AP+STA 模式状态查询」「AP 始终可见」等措辞已被 v1.7.18 起的 **AP/STA 互斥切换**覆盖。STA 凭据 NVS 持久化、`/api/wifi-sta` 接口形态、`WIFI_STA_*` 串口命令集仍有效；只是 STA 连接成功后 AP 会在 1s grace 通过后自动关闭，用户走 STA IP 访问。详见 [`docs/Plan/AP_STA互斥切换方案.md`](./AP_STA互斥切换方案.md)。

## 背景

MUS4 当前 Web Console 已支持 AP+STA 模式状态查询，但 STA SSID 和 Wi-Fi 密码主要来自编译期 `WirelessSecrets.h`，缺少运行时配置与持久化能力。此次优化目标是让 Web Console 明确显示 AP/STA IP，并允许通过 Web Console 与串口配置 STA 凭据，配置重启后仍生效。

## 推荐方案

### 1. 无线策略与测试先行

更新 `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py`：

- `WIFI_STA_STATUS` 作为公开只读命令。
- `WIFI_STA_SSID:<ssid>`、`WIFI_STA_PASSWORD:<password>`、`WIFI_STA_APPLY`、`WIFI_STA_CLEAR` 作为需要认证的无线配置命令。
- `dev_mode` 不绕过 STA 配置认证。
- 增加 `AUTH:<...>` 与 `WIFI_STA_PASSWORD:<...>` 脱敏测试，避免敏感信息进入 Web 日志。

### 2. 固件 STA 凭据模型

在 `mus4.ino` 中复用 `Preferences mus4Prefs` 保存 STA 配置：

- `sta_en`：是否存在用户显式 STA 配置。
- `sta_ssid`：持久化 SSID。
- `sta_pass`：持久化密码。

加载语义：

- `sta_en=true`：使用 NVS 中的 SSID/password。
- `sta_en=false`：明确禁用 STA，不回退编译期凭据。
- 未找到 `sta_en`：兼容旧行为，回退 `WIFI_STA_SSID` / `WIFI_STA_PASSWORD`。

### 3. 串口命令

新增命令：

- `WIFI_STA_STATUS`
- `WIFI_STA_SSID:<ssid>`
- `WIFI_STA_PASSWORD:<password>`
- `WIFI_STA_APPLY`
- `WIFI_STA_CLEAR`

本地 USB Serial / Serial1 可直接执行；TCP/Web 入口必须认证后才能执行修改类命令。所有响应不得回显明文密码。

### 4. Web API 与 UI

新增 Web API：

- `GET /api/wifi-sta`：返回配置、连接状态、AP IP、STA IP，不返回明文密码。
- `POST /api/wifi-sta`：认证后保存 `ssid` / `password` 并立即发起 STA 连接。
- `POST /api/wifi-sta/clear`：认证后清除并禁用 STA，保持 AP/Web Console 可用。

Web Console 首页增加 Network card 和 STA 配置表单，显示 AP IP、STA IP、连接状态、保存/清除结果，并提示密码不会回显、凭据会保存到设备 NVS。

### 5. 日志脱敏

新增统一脱敏逻辑：

- `AUTH:<...>` → `AUTH:<redacted>`
- `WIFI_STA_PASSWORD:<...>` → `WIFI_STA_PASSWORD:<redacted>`

替换 Web `/api/cmd` 与 TCP console 的日志入口。Web STA 配置 API 不记录 password body。

## 验证计划

1. 运行策略测试：

```powershell
python -m pytest tests/test_wireless_console_policy.py
```

2. 固件只编译：

```powershell
.\arduino-cli-wsl.ps1 -Compile
```

3. 如需实机验证，优先 OTA 上传：

```powershell
python arduino-cli.py --ota -i build_wsl/mus4.ino.bin --ota-host <设备IP或主机名>
```

4. 手工验证串口、Web、TCP：确认 STA 配置可保存、应用、清除，重启后持久化生效，Web 日志不出现明文密码。
