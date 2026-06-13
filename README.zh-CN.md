# LP-MU-S4 / MUS4 固件

[English](README.md)

MUS4 是 LP-MU-S4 遥控车/机器人底层控制固件，运行在 ESP32 + Arduino framework 上。固件负责 RC PWM 输入、Pilot 串口控制、多模式控制融合、Park 与紧急制动保护、舵机/电调 PWM 输出、I2C 传感器采样，以及 Wi-Fi/TCP/Web Console 和 OTA。

当前代码只描述已经存在的功能；未来想法放在 `docs/Plan/ROADMAP.md`，不混入现状说明。

## 当前功能

- RC CH1-CH6 PWM 输入：转向、油门、Park、模式、Drift Assist 开关、Drift Assist 强度。
- USB `Serial`、RS232 `Serial1`、TCP Console、Web Console 接收 Pilot/维护命令。
- 手动、半自动、全自动三种控制融合模式。
- Park 锁定与紧急制动状态机，可覆盖油门输出。
- ESP32 `ledc` PWM 驱动转向舵机与油门电调。
- `Serial1` 输出 `Txx:Sxx` 遥测。DEV mode 保持 OTA window 打开时遥测继续输出；只有 OTA 实际传输中暂停。
- INA219 与 MPU6050 I2C 采样。
- USB `Serial` TUI 状态显示，前提是日志目标切到 serial。
- Wi-Fi AP/STA、TCP Console、Web Console、WebSocket 遥测、ArduinoOTA、HTTP `/update`。
- BLE Gamepad 只在关闭 Wi-Fi Console 的编译路径中启用。

## 源码结构

固件按标准 Arduino sketch 方式组织：一个入口 sketch、一个项目头文件、少量按职责分组的实现文件。

- `MUS4_FW.ino`：`setup()`、`loop()`、全局运行时对象装配与周期调度。
- `MUS4.h`：唯一项目头文件，包含编译开关、引脚、版本、共享类型、运行时状态结构和模块 API。
- `MUS4_IO.cpp`：日志、串口行读取、TUI、LED、蜂鸣器、I2C 工具、传感器。
- `MUS4_Control.cpp`：RC 捕获/滤波、转向、Drift Assist、混控、安全状态机、执行器输出。
- `MUS4_Command.cpp`：Pilot 命令解析、本地命令、诊断、转向标定。
- `MUS4_Wifi.cpp`：无线命令权限、AP/STA、OTA、Web Console、Web 日志、WebSocket。
- `WebConsoleAssets.h`：Web Console 的 PROGMEM HTML/CSS/JS 资产，作为生成资产单独保留。
- `WirelessSecrets.h`：本地 Wi-Fi 凭据，已被 git 忽略。

## 串口协议

输入帧：

```text
Throttle:Steering
Throttle:Steering:Seq
Throttle:Steering*XX
```

`Throttle` 和 `Steering` 范围为 `-100..100`。`XX` 是两位十六进制校验和。

响应：

```text
ACK
NACK
ACK:Seq
NACK:Seq
```

`Serial1` 遥测：

```text
Txx:Sxx
```

## 快速验证

Python 依赖：

```bash
pip install pyyaml pyserial pytest
```

运行测试：

```bash
pytest tests/
```

Arduino CLI 包装脚本：

```bash
python arduino-cli.py -c --sketch MUS4_FW.ino
python arduino-cli.py -u --sketch MUS4_FW.ino --port COM9
python arduino-cli.py -cu --no-progress --sketch MUS4_FW.ino
```

WSL 辅助脚本：

```powershell
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost <设备IP> -Sketch MUS4_FW.ino
```

## 操作注意

- 调试控制命令、OTA、转向标定前，先确认车辆处于 Park 锁定状态。
- USB Serial、RS232 Serial1、TCP Console、Web Console 输入都应视为不可信输入。
- 无线命令权限策略需要同步维护 `wireless_console_policy.py` 和对应测试。
- `provisioning_system/` 与 `multi_agent_framework/` 是独立项目，不属于主固件构建路径。

## 文档

- `docs/Guide/MUS4_用户说明书.md`：操作说明书。
- `docs/Arch/architecture.md`：固件架构说明。
- `docs/Hardware/pin_definitions.md`：硬件引脚参考。
- `docs/Tools/ArduinoCLI.md`：Arduino CLI 包装脚本说明。
- `docs/Plan/ROADMAP.md`：未来路线图，不代表当前固件已实现。
