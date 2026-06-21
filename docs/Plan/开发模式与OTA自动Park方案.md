# 开发模式与 OTA 自动 Park 方案

> **实施状态（2026-06-21 / v1.7.7）**：方案 §1–§6 已落地。v1.7.6 之前的实现里 `webDevMode`
> 误将 Web 来源未认证请求等同已认证，违反本方案 §3 第 48–49 行"dev mode 不放宽控制命令/诊断命令"约束；
> v1.7.7 收敛后严格按本方案落地。当前实现的事实映射、放权清单、执行链路与历史偏差收敛过程详见
> [`DEV模式影响面与运行逻辑映射.md`](DEV模式影响面与运行逻辑映射.md)。

## Context

当前 OTA 已成为主要上传路径，但 Web Console 打开 OTA 仍需要手动认证和 Park 锁定，开发迭代时步骤偏多。目标是在 Web Console 中增加持久化“开发模式”，开启后可通过 Web 免认证触发 OTA，同时在 OTA 激活后强制进入 Park，避免舵机/电调在升级窗口或上传过程中意外运行。

## 推荐实现

### 1. 开发模式持久化

修改 `mus4.ino`：

- 引入 ESP32 Arduino `Preferences`。
- 新增 `wifiDevModeEnabled` 状态，默认 `false`。
- 使用 NVS namespace `mus4`、key `dev_mode` 持久化。
- 在 `setup()` 中、启动 Web Console 前加载开发模式。
- 新增保存函数，仅在用户切换开关时写 NVS，避免频繁写 flash。

### 2. Web Console 增加开发模式 API 和 UI

修改 `mus4.ino` 内置 Web Console：

- 新增 `GET /api/devmode`：返回 `{"enabled":true/false}`。
- 新增 `POST /api/devmode`：接受 `1/0/true/false/on/off`，保存到 NVS 并返回保存结果。
- 在 Web 页面命令区增加“开发模式”开关和醒目提示。
- 页面加载时读取设备真实状态，不使用浏览器本地存储。
- 开启时弹出确认提示，说明开发模式会持久化，并且只放宽 Web OTA，不放宽控制命令。

### 3. 限定开发模式放权范围

为无线命令处理增加来源区分：

- `WIRELESS_ORIGIN_WEB`
- `WIRELESS_ORIGIN_TCP`

调整调用点：

- Web `/api/cmd` 调用 `processWirelessConsoleLine(..., WIRELESS_ORIGIN_WEB)`。
- TCP Console 调用 `processWirelessConsoleLine(..., WIRELESS_ORIGIN_TCP)`。

开发模式只对 Web 来源生效：

- Web + dev mode 允许未认证执行：
  - `ENABLE_OTA`
  - `OTA_STATUS`
  - `DISABLE_OTA`
- TCP Console 即使 dev mode 开启，仍保持现有认证/Park 要求。
- dev mode 不放宽控制命令，例如 `10:20`。
- dev mode 不放宽诊断命令，例如 `TEST`、`BENCH`、`REGRESS`。
- 不通过设置全局 `wifiConsoleAuthenticated = true` 实现开发模式，避免误放权。

### 4. OTA 激活后自动 Park Guard

新增 `wifiOtaParkGuardActive`。

当任意入口成功打开 OTA 窗口时启用 guard：

- Web 普通认证 OTA。
- Web dev mode OTA。
- TCP 认证 OTA。
- Serial/Serial1 本地 `ENABLE_OTA:<密码>`。

新增 `forceWifiOtaParkLocked()`：

- `rc_data.park = PARK_LOCKED`
- `car_output.park = PARK_LOCKED`
- `car_output.throttle = 0`

在 `loop()` 中 `park_change()` 之后、模式混控和输出计算之前调用 guard，防止 RC 输入在 OTA 期间重新解锁。

OTA 生命周期处理：

- `updateWifiOta()` 中如果 guard 激活，发现 Park 变化时重新强制 Park，而不是关闭 OTA。
- `closeWifiOtaWindow()`、`ArduinoOTA.onEnd()`、`ArduinoOTA.onError()` 清除 `wifiOtaParkGuardActive`。
- `DISABLE_OTA` 可主动关闭窗口并解除 guard。

### 5. 状态输出增强

修改 `printWirelessStatus()` 和 `printWifiOtaStatus()`：

- `STATUS` 增加 `dev_mode=` 和 `park_guard=`。
- `OTA_STATUS` 增加 `dev_mode=` 和 `park_guard=`。

便于 Web、自动化脚本和手工调试确认状态。

### 6. Python 策略镜像与测试

修改：

- `wireless_console_policy.py`
- `tests/test_wireless_console_policy.py`

策略函数更新为支持 `dev_mode` 和 `origin`：

- Web dev mode 允许未认证、未 Park 执行 `ENABLE_OTA`。
- Web dev mode 允许未认证执行 `OTA_STATUS`、`DISABLE_OTA`。
- TCP origin 不受 dev mode 放宽。
- Web dev mode 不允许未认证控制输出命令。
- Web dev mode 不允许未认证/未 Park 执行诊断命令。
- 增加 Park Guard 镜像函数测试：OTA 窗口或传输中应强制 Park。

## 关键文件

- `mus4.ino`
- `wireless_console_policy.py`
- `tests/test_wireless_console_policy.py`

## 验证方案

### 自动化测试

```powershell
pytest tests/ -q
```

### 固件编译

```powershell
.\arduino-cli-wsl.ps1 -Compile
```

### OTA 上传验证

不再自动新开 PowerShell 串口上传监视。编译通过后按 OTA 流程验证：

1. 打开 Web Console。
2. 开启开发模式并确认持久化。
3. 发送 `ENABLE_OTA`，期望无需 `AUTH` 即返回 `OTA_READY`。
4. 执行 OTA 上传：

```powershell
python arduino-cli.py --ota -i build_wsl/mus4.ino.bin --ota-host <设备IP或主机名>
```

### 手工行为验证

- 默认开发模式关闭，未认证 `ENABLE_OTA` 仍被拒绝。
- 开启开发模式后，Web Console 未认证 `ENABLE_OTA` 成功。
- 重启后开发模式状态保持。
- TCP Console 未认证 `ENABLE_OTA` 仍被拒绝。
- dev mode 下未认证控制命令 `10:20` 仍被拒绝。
- OTA 窗口期间 `STATUS` / `OTA_STATUS` 显示 `park=1`、`park_guard=1`。
- OTA 期间即使 RC 侧尝试解锁，输出仍保持 Park，油门保持安全。
- `DISABLE_OTA` 或窗口超时后 guard 解除，Park 重新由 RC 状态管理。
