# LP-MU-S4 / MUS4

MUS4（LP-MU-S4）是基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件，当前以 MUS4-v2.3 PCB 为准。固件负责 RC PWM 输入采集、Pilot 上位机串口控制、多模式控制融合、Park/紧急制动、I2C 传感器采集、TUI 状态显示、Wi-Fi/TCP/Web Console、OTA 更新，以及在未启用 Wi-Fi Console 时可选的 BLE Gamepad 输出。

当前固件版本定义在 [`BuildInfo.h`](BuildInfo.h)，发布记录见 [`CHANGELOG.md`](CHANGELOG.md)。

## 主要功能

- **RC PWM 输入采集**：CH1-CH6 支持遥控转向、油门、Park、模式、Drift Assist 开关与强度比例。
- **Pilot 串口控制**：支持 `Throttle:Steering`、序列号和校验和格式的上位机控制协议。
- **三种驾驶模式**：手动、半自动、全自动，按模式融合 RC 与 Pilot 控制源。
- **Park / 紧急制动**：安全状态机覆盖油门输出，并联动 LED 指示。
- **执行器 PWM 输出**：使用 ESP32 `ledc` 驱动转向舵机与电调。
- **Wi-Fi / TCP / Web Console**：支持无线命令、状态页面、日志、动态曲线和 WebSocket 遥测。
- **OTA 更新**：支持 ArduinoOTA 与 Web Console HTTP `/update` 上传。
- **I2C 传感器**：支持 INA219 与 MPU6050 数据采集。
- **Drift Assist**：基于 IMU 角速度和 CH5/CH6 状态叠加转向补偿。
- **BLE Gamepad**：仅在未启用 Wi-Fi Console 的编译路径中生效。

## 硬件引脚（MUS4-v2.3）

权威引脚定义见 [`Doc/Hardware/pin_definitions.md`](Doc/Hardware/pin_definitions.md)。

| 功能 | GPIO | 说明 |
| --- | --- | --- |
| RC CH1 转向 | 36 | 仅输入 |
| RC CH2 油门 | 39 | 仅输入 |
| RC CH3 Park | 34 | 仅输入 |
| RC CH4 模式 | 26 | PWM 输入 |
| RC CH5 Drift 开关 | 27 | PWM 输入 |
| RC CH6 Drift 强度 | 35 | 仅输入 |
| 转向舵机 | 23 | `ledc` PWM，300Hz / 14bit，1000-2000µs 映射 |
| 油门电调 | 25 | `ledc` PWM，300Hz / 14bit，1000-2000µs 映射 |
| 备用 PWM_1 | 32 | 预留输出 |
| 备用 PWM_2 | 33 | 预留输出 |
| WS2812B LED | 5 | 模式与紧急停车指示 |
| UART_SEL | 12 | UART 路由选择 |
| Serial1 RX | 16 | RS232 / Pilot 输入 |
| Serial1 TX | 17 | RS232 / Pilot 输出 |
| I2C SDA | 21 | INA219 / MPU6050 |
| I2C SCL | 22 | INA219 / MPU6050 |

> GPIO 34、35、36、39 是 ESP32 仅输入引脚，不能配置为输出，也没有内部上拉/下拉。

## 控制模式

| 模式 | ID | 宏定义 | 转向来源 | 油门来源 | LED |
| --- | --- | --- | --- | --- | --- |
| 手动 | 0 | `CAR_MODE_MANUAL` | RC | RC | 绿色 |
| 半自动 | 1 | `CAR_MODE_SEMI_AUTO` | Pilot | RC | 黄色 |
| 全自动 | 2 | `CAR_MODE_FULL_AUTO` | Pilot | Pilot | 蓝色 |

## 串口协议

输入格式：

```text
Throttle:Steering\n
Throttle:Steering:Seq\n
Throttle:Steering*XX\n
```

其中 `XX` 是两位十六进制校验和。

响应格式：

```text
ACK
NACK
ACK:Seq
NACK:Seq
```

Serial1 状态回传：

```text
Txx:Sxx\n
```

OTA 窗口打开或 OTA 传输中会暂停 Serial1 遥测。

## 快速开始

### 依赖

```bash
pip install pyyaml pyserial pytest
```

还需要安装：

- Arduino CLI
- ESP32 Arduino core
- 固件依赖库：FastLED、Adafruit INA219、Adafruit MPU6050、AsyncTCP、ESPAsyncWebServer 等

### WSL 加速构建（Windows 推荐）

固件修改后的默认验证方式是 WSL 编译：

```powershell
.\arduino-cli-wsl.ps1 -Compile
```

常用命令：

```powershell
# 清理 WSL 构建目录后重新编译
.\arduino-cli-wsl.ps1 -Compile -Clean

# 检查 WSL、rsync、arduino-cli 等依赖
.\arduino-cli-wsl.ps1 -Check

# 编译并检查固件大小与分区占用
.\arduino-cli-wsl.ps1 -Compile -CheckPartition

# 编译后通过 Web Console HTTP OTA 上传
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost 192.168.4.1

# 使用已有 build_wsl 产物通过 HTTP OTA 上传
.\arduino-cli-wsl.ps1 -Upload -HttpOta -HttpOtaHost 192.168.4.1
```

HTTP OTA 需要 Web Console 已认证且 Park 锁定，或开发模式允许。目标主机可通过 `-HttpOtaHost` 指定，也可写入 `.mus4_ota_target` 首行。

### Arduino CLI 原生命令

```bash
# 仅编译
python arduino-cli.py -c

# 仅上传，默认按 config.yaml 自动检测串口
python arduino-cli.py -u

# 指定串口上传
python arduino-cli.py -u --port COM9

# 编译 + 上传
python arduino-cli.py -cu

# 编译 + 上传 + 串口监视
python arduino-cli.py -cus

# 使用预编译固件上传
python arduino-cli.py -u -i build/mus4.ino.bin

# 列出检测到的串口
python arduino-cli.py --list-ports

# 逐行输出日志，适合 CI
python arduino-cli.py -cu --no-progress

# ArduinoOTA 上传
python arduino-cli.py --ota -i build/mus4.ino.bin --ota-host mus4-ota
```

默认配置见 [`config.yaml`](config.yaml)：

- FQBN：`esp32:esp32:esp32:PartitionScheme=min_spiffs`
- sketch：`mus4.ino`
- build path：`build`
- baudrate：`115200`
- port：`auto`

## 测试

```bash
# 运行全部 Python 测试
pytest tests/

# 运行单个测试文件
pytest tests/test_arduino_cli.py
pytest tests/test_wireless_console_policy.py
pytest tests/test_firmware_feature_flags.py
pytest tests/test_train_tub_driver.py
pytest tests/test_mus4_pilot_infer.py

# 运行单个测试用例
pytest tests/test_arduino_cli.py -k "test_prefers_explicit_port_when_available"
pytest tests/test_wireless_console_policy.py -k "test_requires_authentication_and_park_locked_for_ota_open"
pytest tests/test_firmware_feature_flags.py -k "test_websocket_curve_data_feature_is_enabled"

# 配网代理测试
python provisioning_system/tests/test_agent.py -v
```

`provisioning_system/playwright_tests/` 下存在 Playwright 依赖，但当前 `package.json` 的 `npm test` 是占位脚本并会退出失败；除非先补充有效测试脚本，否则不要把 `npm test` 当作该目录的验证命令。

## 数据采集与 Pilot 工具链

```bash
# 检查 Web Console 导出的 Tub JSON 并生成报告
python tools/train_tub_driver.py <tub.json> --report-only --dry-run

# 训练 GRU baseline，默认排除泄漏字段
python tools/train_tub_driver.py <tub.json> --out-dir <model_dir> --overwrite

# 模型 Pilot 推理默认 dry-run，不会向车辆发送控制
python tools/mus4_pilot_infer.py --model-dir <model_dir> --esp32-url http://<设备IP>

# live 模式必须显式确认风险，并指定串口
python tools/mus4_pilot_infer.py --model-dir <model_dir> --serial-port COM9 --mode live --i-understand-risk
```

## 代码结构

- [`mus4.ino`](mus4.ino)：主固件入口，包含 setup/loop、RC 中断采集、串口命令、控制融合、Park/紧急制动、Wi-Fi/Web/OTA 等主状态机。
- [`SharedTypes.h`](SharedTypes.h)：跨模块共享数据结构与状态枚举。
- [`BuildInfo.h`](BuildInfo.h)：固件名称、版本与构建信息宏。
- [`TUI.h`](TUI.h) / [`TUI.cpp`](TUI.cpp)：ANSI 终端仪表盘渲染。
- [`Buzzer.h`](Buzzer.h) / [`Buzzer.cpp`](Buzzer.cpp)：蜂鸣器状态机。
- [`arduino-cli.py`](arduino-cli.py)：跨平台 Arduino CLI 封装，负责编译、上传、串口检测、监视和 ArduinoOTA。
- [`arduino-cli-wsl.ps1`](arduino-cli-wsl.ps1)：Windows/WSL 加速构建与 OTA 上传包装脚本。
- [`wireless_console_policy.py`](wireless_console_policy.py)：Wi-Fi/TCP/Web Console 权限策略的 Python 镜像。
- [`tools/`](tools)：Tub JSON 训练与 Pilot 推理工具。
- [`tests/`](tests)：构建工具、权限策略、功能开关、训练工具和 Pilot 推理的 Python 测试。
- [`examples/`](examples)：I2C、传感器等独立示例 sketch。
- [`provisioning_system/`](provisioning_system)：独立 Wi-Fi provisioning 与 Linux agent 工具链。
- [`multi_agent_framework/`](multi_agent_framework)：独立 Python 多智能体框架，不属于 ESP32 固件主链路。

## 架构概览

固件主数据流：

1. RC 接收机通过 CH1-CH6 PWM 输入触发中断，计算脉宽并滤波。
2. USB `Serial`、`Serial1`、TCP Console 与 Web Console 接收命令。
3. 控制逻辑按当前模式融合 RC 与 Pilot 数据，更新 `car_output`。
4. Drift Assist 在条件满足时基于 IMU 角速度叠加转向补偿。
5. Park/紧急制动状态机可覆盖油门输出并控制 LED 闪烁。
6. 输出层通过 `ledc` 产生 PWM，驱动转向舵机与油门电调。
7. TUI、I2C 传感器、Web 曲线/日志、WebSocket 遥测和 BLE Gamepad 作为旁路功能读取状态并输出显示或外设数据。

更详细的设计见 [`Doc/Arch/architecture.md`](Doc/Arch/architecture.md)。

## Wi-Fi Console、Web Console 与 OTA

当前 `mus4.ino` 定义了 `ENABLE_WIFI_CONSOLE`，并在该路径下启用 `ENABLE_WIFI_WEBSOCKET_TELEMETRY`。启用后 ESP32 以 AP+STA 模式启动：

- AP SSID：`MUS4-DEBUG`
- TCP Console：`2323`
- Web Console：`80`
- WebSocket 遥测：`81`
- ArduinoOTA：默认主机名 `mus4-ota`，端口 `3232`

无线命令权限分层：

- `PING`、`STATUS`、`AUTH`、`WIFI_STA_STATUS` 可未认证访问。
- 控制指令和 `ANSI` / `NOANSI` / `FILTER_DEBUG` / `LOG_WEB` / `LOG_SERIAL` / Wi-Fi STA 配置命令需要认证。
- `TEST`、`BENCH`、`REGRESS`、转向标定等诊断/维护命令需要认证且 Park 锁定。
- `ENABLE_OTA` 需要认证且 Park 锁定；`OTA_STATUS` 与 `DISABLE_OTA` 需要认证。

修改无线权限逻辑时，需要同步更新 [`wireless_console_policy.py`](wireless_console_policy.py) 与 [`tests/test_wireless_console_policy.py`](tests/test_wireless_console_policy.py)。

## 文档索引

- [`CLAUDE.md`](CLAUDE.md)：Claude Code 操作本仓库的权威指南。
- [`CHANGELOG.md`](CHANGELOG.md)：版本发布记录。
- [`Doc/Arch/architecture.md`](Doc/Arch/architecture.md)：固件主循环、状态机和数据流。
- [`Doc/Hardware/pin_definitions.md`](Doc/Hardware/pin_definitions.md)：MUS4-v2.3 权威引脚定义。
- [`Doc/Hardware/CONFIG.md`](Doc/Hardware/CONFIG.md)：硬件配置说明。
- [`Doc/Tools/ArduinoCLI.md`](Doc/Tools/ArduinoCLI.md)：`arduino-cli.py` 使用说明。
- [`Doc/Tools/arduino-cli-wsl_manual.md`](Doc/Tools/arduino-cli-wsl_manual.md)：WSL 构建脚本背景与排障。
- [`Doc/Tools/train_tub_driver.md`](Doc/Tools/train_tub_driver.md)：Tub JSON 报告与 GRU baseline 训练说明。
- [`Doc/Tools/mus4_pilot_infer.md`](Doc/Tools/mus4_pilot_infer.md)：Pilot 模型推理控制器说明。
- [`Doc/README/OPERATIONS.md`](Doc/README/OPERATIONS.md)：串口运行时操作命令与数据帧。
- [`Doc/Plan/`](Doc/Plan/)：设计方案、实施路线和历史方案。

## 安全注意事项

该固件会直接控制舵机与电调。修改输出映射、Park、紧急制动、模式融合或无线控制入口时，必须保留 PWM 限幅、认证/权限校验和失效安全路径。串口、Web Console、TCP Console 输入都应视为不可信边界。
