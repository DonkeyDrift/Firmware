# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MUS4（LP-MU-S4）是基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件，当前以 MUS4-v2.3 PCB 为准。固件负责 RC PWM 输入采集、Pilot 上位机串口控制、多模式控制融合、Park/紧急制动、I2C 传感器采集、TUI 状态显示、Wi-Fi/TCP/Web Console、OTA 更新，以及在未启用 Wi-Fi Console 时可选的 BLE Gamepad 输出。

根目录同时包含两类主线代码：
- Arduino/C++ 固件：主 sketch 为 `mus4.ino`，公共类型与外设辅助模块也在根目录。
- 构建与烧录工具：`arduino-cli.py` 与 `arduino-cli-wsl.ps1` 负责编译、上传、串口检测、OTA 上传和 WSL 加速构建。

主要依赖包括 FastLED、Wire、Adafruit_INA219、Adafruit_MPU6050、WebServer、ArduinoOTA、AsyncTCP、ESPAsyncWebServer，以及 BLE Gamepad 相关库（仅在 Wi-Fi Console 未启用的编译路径中生效）。

## Commands

### Python 依赖

```bash
pip install pyyaml pyserial pytest
```

项目未发现专用 Python lint/format 配置；修改 Python 工具脚本或策略镜像后，至少运行相关 `pytest`。

### Arduino CLI 构建、上传、监视

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

# 指定构建输出目录
python arduino-cli.py -c --build-path build

# 列出检测到的串口
python arduino-cli.py --list-ports

# 逐行输出日志，适合重定向或 CI 日志
python arduino-cli.py -cu --no-progress

# ArduinoOTA 上传，默认端口 3232，密码默认 mus4-debug
python arduino-cli.py --ota -i build/mus4.ino.bin --ota-host mus4-ota
```

常用参数：
- `--config config.yaml`：指定构建配置文件。
- `--fqbn esp32:esp32:esp32`：覆盖开发板 FQBN。
- `--sketch mus4.ino`：覆盖 sketch 路径。
- `--baud 115200`：覆盖串口波特率。
- `--auto-reset` / `--no-auto-reset`：控制串口监视前复位。
- `--regress-reset --regress-count 10`：运行复位回归测试。
- `--ota-host` / `--ota-port` / `--ota-password` / `--espota-tool`：覆盖 ArduinoOTA 上传目标与工具路径。

### Windows + WSL 加速构建

固件编译优先使用 WSL 脚本；项目当前优先使用 OTA 上传，不要在编译通过后自动新开 PowerShell 串口上传或串口监视，除非用户明确要求。

```powershell
# 仅编译；这是固件修改后的默认验证命令
.\arduino-cli-wsl.ps1 -Compile

# 清理 WSL 构建目录后重新编译
.\arduino-cli-wsl.ps1 -Compile -Clean

# 构建在 /mnt/c 上而不是同步到 WSL ext4，适合快速排查同步问题
.\arduino-cli-wsl.ps1 -Compile -IoMode mnt

# 检查 WSL、rsync、arduino-cli 等依赖
.\arduino-cli-wsl.ps1 -Check

# 编译并检查固件大小与分区占用
.\arduino-cli-wsl.ps1 -Compile -CheckPartition

# 编译后通过 ArduinoOTA 上传；上传前需先在设备 Web/TCP Console 中打开 OTA 窗口
.\arduino-cli-wsl.ps1 -Compile -Upload -Ota -OtaHost <设备IP或主机名>

# 使用已有 build_wsl 产物通过 ArduinoOTA 上传
.\arduino-cli-wsl.ps1 -Upload -Ota -OtaHost <设备IP或主机名>

# 编译后通过 Web Console 的 HTTP /update 端点上传
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost <设备IP或主机名>

# 当前调试目标可按 .mus4_ota_target 首行；显式传入可避免被旧目标覆盖
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost 192.168.3.144

# 使用已有 build_wsl 产物通过 HTTP OTA 上传；未传主机时读取 .mus4_ota_target 首行
.\arduino-cli-wsl.ps1 -Upload -HttpOta

# 仅当用户明确要求串口上传时使用已有 build_wsl 产物
.\arduino-cli-wsl.ps1 -Upload

# 仅当用户明确要求串口监视时使用
.\arduino-cli-wsl.ps1 -Upload -Serial

# 指定串口会透传给 arduino-cli.py
.\arduino-cli-wsl.ps1 -Upload -ExtraArgs "--port COM9"
```

WSL 脚本默认 `IoMode=native`：先把源码同步到 WSL 原生文件系统编译，再在 `build_wsl/` 生成并回传 `.bin` / `.elf` 产物。HTTP OTA 使用设备 Web Console 的 `/update` 端点，需要 Web Console 已认证且 Park 锁定，或开发模式允许；目标主机可通过 `-HttpOtaHost` 指定，或写入项目根目录 `.mus4_ota_target` 的首行。当前调试优先使用 `.mus4_ota_target` 中的目标；截至 2026-06-05 当前目标为 `192.168.3.144`，如需改用 AP 默认地址再显式传入 `192.168.4.1`。

### Python 测试

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
pytest tests/test_train_tub_driver.py -k "test_build_windows_excludes_leakage_columns_by_default"
pytest tests/test_mus4_pilot_infer.py -k "test_live_mode_requires_explicit_risk_ack"

# 配网代理测试
python provisioning_system/tests/test_agent.py -v

# 详细输出
pytest tests/ -v
```

`provisioning_system/playwright_tests/` 下存在 Playwright 依赖，但当前 `package.json` 的 `npm test` 只是占位脚本并会退出失败；除非先补充有效测试脚本，否则不要把 `npm test` 当作该目录的验证命令。

### 数据采集与模型工具

```bash
# 仅检查 Web Console 导出的 Tub JSON 并生成报告
python tools/train_tub_driver.py <tub.json> --report-only --dry-run

# 训练 GRU baseline；默认排除 ch1/ch2/rct/rcs/thr/str 等泄漏字段
python tools/train_tub_driver.py <tub.json> --out-dir <model_dir> --overwrite

# 模型 Pilot 推理默认 dry-run，不会向车辆发送控制
python tools/mus4_pilot_infer.py --model-dir <model_dir> --esp32-url http://<设备IP>

# live 模式必须显式确认风险，并指定串口
python tools/mus4_pilot_infer.py --model-dir <model_dir> --serial-port COM9 --mode live --i-understand-risk
```

### 固件运行时串口测试命令

在 USB Serial、Serial1、TCP Console 或 Web Console 中输入并回车；无线入口受认证和 Park 权限限制：
- `TEST`：运行固件内置命令解析测试。
- `TEST_TUI`：TUI 测试入口，当前输出 skipped。
- `BENCH`：运行 TUI/循环性能基准。
- `STRESS`：运行串口压力统计。
- `REGRESS`：运行固件回归校验。
- `FILTER_TEST`：运行 RC 滤波测试入口。
- `FILTER_DEBUG`：切换 RC 滤波调试输出。
- `STEER_CAL` / `CAL_SAVE` / `CAL_RETRY` / `CAL_ABORT` / `CAL_RESET` / `CAL_STATUS`：转向通道交互式标定流程。
- `WIFI_STA_SSID` / `WIFI_STA_PASSWORD` / `WIFI_STA_APPLY` / `WIFI_STA_CLEAR` / `WIFI_STA_STATUS`：Wi-Fi STA 配置与状态命令。
- `ENABLE_OTA` / `OTA_STATUS` / `DISABLE_OTA`：OTA 窗口控制。
- `LOG_WEB` / `LOG_SERIAL`：切换 MUS4 日志输出目标。
- `ANSI` / `NOANSI`：切换 TUI ANSI 转义序列显示。

## Configuration

- `config.yaml`：`arduino-cli.py` 的主配置，包含 `arduino_cli`、`fqbn`、`port`、`baudrate`、`sketch_path`、`build_path`、串口自动检测与日志配置。
- `sketch.yaml`：Arduino CLI 项目级默认配置，当前默认 FQBN 为 `esp32:esp32:esp32:PartitionScheme=min_spiffs`，默认端口为 `/dev/ttyS4`。
- `wslbuild.yaml`（若存在）：`arduino-cli-wsl.ps1` 会读取其中的 `distro`、`sketch`、`fqbn`、`work_dir`、`io_mode`、`sync_libs`、`extra_sync_args` 等覆盖项；命令行参数优先级最高。
- `.mus4_ota_target`（若存在）：HTTP OTA 默认目标主机列表，脚本读取首个非空首行。
- `WirelessSecrets.example.h`：Wi-Fi STA 凭据模板；本地 `WirelessSecrets.h` 可被 `mus4.ino` 自动包含，但可能含真实凭据，提交前必须检查且通常不应纳入提交。

当前 `config.yaml` 默认：
- FQBN：`esp32:esp32:esp32:PartitionScheme=min_spiffs`
- sketch：`mus4.ino`
- build path：`build`
- baudrate：`115200`
- port：`auto`
- 日志文件：`ArduinoCLI.log`

串口自动检测会根据描述、厂商、VID/PID 和首选关键字匹配 ESP32/CP210/CH340/FTDI 等 USB 串口；双串口板当前优先 `SERIAL-A`。

## High-Level Architecture

### 构建工具层

`arduino-cli.py` 是跨平台主入口：读取 `config.yaml`，封装 `arduino-cli compile/upload/monitor`，处理串口自动匹配、上次成功端口缓存、双串口回退、复位、进度输出、日志、预编译 `.bin` 选择和 ArduinoOTA 上传。

`arduino-cli-wsl.ps1` 是 Windows/WSL 包装层：自动探测 WSL 环境，把源码同步到 WSL 原生文件系统编译，回传产物后调用 `arduino-cli.py` 执行 Windows 端串口上传或 ArduinoOTA；HTTP OTA 则直接通过 `curl.exe` POST 到设备 Web Console 的 `/update` 端点。

Python 测试集中在 `tests/`：
- `tests/test_arduino_cli.py` 覆盖串口选择、双串口回退、进度解析、上传命令构造和预编译固件选择等纯逻辑。
- `tests/test_wireless_console_policy.py` 覆盖无线控制台认证、Park 锁定、OTA 窗口、Web/STA 状态格式和行缓冲规则。
- `tests/test_firmware_feature_flags.py` 用源码断言保护关键编译开关、Web Console/Donkey Console UI、状态卡片布局、曲线实现形态和前端安全门控。
- `tests/test_train_tub_driver.py` 覆盖 Tub JSON 读取、数据质量报告、特征防泄漏和窗口数据集构造。
- `tests/test_mus4_pilot_infer.py` 覆盖模型推理控制器的标准化校验、安全门控、串口命令和 ACK 解析。

### 固件应用层

`mus4.ino` 是主状态机，关键数据流是：
1. RC 接收机通过 CH1-CH6 PWM 输入触发中断，计算脉宽并滤波；CH5/CH6 当前用于 Drift Assist 开关与强度比例。
2. USB `Serial`、`Serial1`、TCP Console 与 Web Console 接收命令，解析 `Throttle:Steering`、序列号与校验和格式。
3. 控制逻辑按模式融合 RC 与 Pilot 数据，更新 `car_output`，Drift Assist 可在条件满足时叠加转向补偿。
4. Park/紧急制动状态机可覆盖油门输出并控制 LED 闪烁；OTA 窗口或 OTA 传输期间会暂停 Serial1 遥测。
5. 输出层通过 ESP32 `ledc` 产生 PWM，驱动转向舵机与油门电调，并通过 Serial1 回传 `Txx:Sxx`。
6. TUI、I2C 传感器、Web 数据曲线/日志缓冲、WebSocket 遥测和 BLE Gamepad 作为旁路功能读取状态并输出显示或手柄轴数据。

### 核心模块

- `SharedTypes.h`：跨模块共享的数据结构与状态枚举，例如 `SensorData`、`ControlData`。
- `TUI.h` / `TUI.cpp`：ANSI 终端仪表盘渲染，支持降级模式和增量刷新。
- `Buzzer.h` / `Buzzer.cpp`：蜂鸣器状态机，硬件支持时使用。
- `wireless_console_policy.py`：Wi-Fi/TCP/Web Console 权限策略的 Python 镜像，用于在桌面测试中覆盖认证、Park 锁定、OTA 窗口、STA 状态、日志脱敏和行缓冲行为。

### 数据采集与 Pilot 工具链

Web Console 的 Tub JSON 记录用于离线行为克隆训练。`tools/train_tub_driver.py` 负责读取一个或多个 Tub JSON、输出数据质量报告、构造窗口数据集并训练 GRU baseline；默认排除 `ch1/ch2/rct/rcs/thr/str/seq/t` 等泄漏字段，相关测试在 `tests/test_train_tub_driver.py`。

`tools/mus4_pilot_infer.py` 在主机侧加载训练产物，通过 Web Console 拉取遥测并经串口向 ESP32 发送 Pilot 控制命令；默认 `dry-run`，`zero-output`/`live` 要求串口，`live` 还必须传 `--i-understand-risk`，并受 Park、模式、限幅、速率和 ACK 失败保护。相关测试在 `tests/test_mus4_pilot_infer.py`。

### 辅助子系统

- `examples/`：I2C、传感器等独立示例 sketch。
- `multi_agent_framework/`：独立 Python 多智能体框架代码，不属于 ESP32 固件主链路。
- `provisioning_system/`：ESP32 Wi-Fi provisioning 与 Linux agent 相关工具，独立于 MUS4 主固件构建流程；ESP32 AP/Web Server 通过 UART 把 Wi-Fi 凭据发给 Linux agent，agent 使用 NetworkManager/nmcli 连接目标网络并回传结果。

## Firmware Behavior Reference

### 版本与发布记录

当前固件版本定义在 `BuildInfo.h` 的 `MUS4_FIRMWARE_VERSION`；发布或稳定版本更新时，同步递增该值并维护 `CHANGELOG.md`。每次版本更新后，确认 `BuildInfo.h` 中的版本号与 `CHANGELOG.md` 最新条目一致；当前两者应为 `v1.5.23`。README 中部分硬件与路径描述可能滞后，硬件细节以 `mus4.ino` 与 `Doc/Hardware/pin_definitions.md` 为准。

### 控制模式

| 模式 | ID | 宏定义 | 转向来源 | 油门来源 | LED |
| --- | --- | --- | --- | --- | --- |
| 手动 | 0 | `CAR_MODE_MANUAL` | RC | RC | 绿色 |
| 半自动 | 1 | `CAR_MODE_SEMI_AUTO` | Pilot | RC | 黄色 |
| 全自动 | 2 | `CAR_MODE_FULL_AUTO` | Pilot | Pilot | 蓝色 |

### 串口协议

输入：
- `Throttle:Steering\n`
- `Throttle:Steering:Seq\n`
- `Throttle:Steering*XX\n`，其中 `XX` 为两位十六进制校验和。

响应：
- `ACK` / `NACK`
- 带序列号时返回 `ACK:Seq` / `NACK:Seq`

状态回传：
- Serial1 输出 `Txx:Sxx\n`；OTA 窗口打开或 OTA 进行中时暂停。

### 当前 v2.3 引脚

| 功能 | GPIO | 说明 |
| --- | --- | --- |
| RC CH1 转向 | 36 | 仅输入 |
| RC CH2 油门 | 39 | 仅输入 |
| RC CH3 Park | 34 | 仅输入 |
| RC CH4 模式 | 26 | PWM 输入 |
| RC CH5 Drift 开关 | 27 | PWM 输入 |
| RC CH6 Drift 强度 | 35 | 仅输入 |
| 转向舵机 | 23 | `ledc` PWM，当前 300Hz/14bit，1000-2000µs 映射 |
| 油门电调 | 25 | `ledc` PWM，当前 300Hz/14bit，1000-2000µs 映射 |
| 备用 PWM_1 | 32 | 预留输出 |
| 备用 PWM_2 | 33 | 预留输出 |
| WS2812B LED | 5 | 模式与紧急停车指示 |
| UART_SEL | 12 | UART 路由选择 |
| Serial1 RX | 16 | RS232/Pilot 输入 |
| Serial1 TX | 17 | RS232/Pilot 输出 |
| I2C SDA | 21 | INA219 / MPU6050 |
| I2C SCL | 22 | INA219 / MPU6050 |

README 中仍包含旧版引脚和旧路径；以 `mus4.ino`、`Doc/Hardware/pin_definitions.md` 和本文件为准。

### Wi-Fi Console、Web Console 与 OTA

`mus4.ino` 当前定义了 `ENABLE_WIFI_CONSOLE`，并在该路径下启用 `ENABLE_WIFI_WEBSOCKET_TELEMETRY`。启用后 ESP32 以 AP+STA 模式启动：AP SSID 为 `MUS4-DEBUG`，TCP 控制台端口为 `2323`，Web Console/Donkey Console 端口为 `80`，WebSocket 遥测端口为 `81`，ArduinoOTA 默认主机名为 `mus4-ota`、端口 `3232`。

无线命令权限分层：`PING`、`STATUS`、`AUTH`、`WIFI_STA_STATUS` 可未认证访问；控制指令和 `ANSI`/`NOANSI`/`FILTER_DEBUG`/`LOG_WEB`/`LOG_SERIAL`/Wi-Fi STA 配置命令需要认证；`TEST`、`BENCH`、`REGRESS`、转向标定等诊断/维护命令还要求 Park 锁定；`ENABLE_OTA` 要求认证且 Park 锁定，`OTA_STATUS` 与 `DISABLE_OTA` 要求认证。修改这部分逻辑时，同步更新 `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py`。

Web UI 的 HTML/CSS/JS 目前内嵌在 `mus4.ino` 的 `WIFI_WEB_CONSOLE_HTML` 中，页面品牌已显示为 `Donkey Console`。修改标题、顶部 DEV/OTA 区、状态卡片、串口日志、Tub JSON 或图表行为时，优先用 `tests/test_firmware_feature_flags.py` 增加源码断言，再做最小实现。

### BLE Gamepad Mode

`ENABLE_GAMEPAD_MODE` 位于 `#ifndef ENABLE_WIFI_CONSOLE` 分支中；当前 Wi-Fi Console 开启时 BLE Gamepad 不会启用。启用 BLE Gamepad 时将 RC 通道映射为：
- CH1 转向 → 右摇杆 X 轴。
- CH2 油门 → 左摇杆 Y 轴。

设备名称在代码中维护；修改 BLE 相关行为后需要重新编译并在目标主机上重新配对验证。

### Drift Assist 与转向标定

`mus4.ino` 当前包含漂移辅助编译开关与参数：`DRIFT_ASSIST_ENABLED`、增益、阈值、最大补偿和平滑/衰减系数。相关逻辑依赖 IMU 角速度与 CH3/CH5/CH6 通道状态，修改时同时检查 Park/解锁语义，避免与安全状态机冲突。

转向标定状态存储在 Preferences 中，相关命令受无线权限策略保护。修改标定命令、持久化键或转向映射后，同时检查 `wireless_console_policy.py` 和运行时串口命令列表。

## Safety-Critical Editing Notes

- 该固件会直接控制舵机与电调；修改输出映射、Park、紧急制动、模式融合或无线控制入口时，必须保留 PWM 限幅和失效安全路径。
- 当前执行器输出使用 `ledcAttachChannel(..., 300, 14, ...)`，PWM 常量按 300Hz/14bit、1000-2000µs 计算；部分旧文档仍写 50Hz，改输出参数时以 `mus4.ino` 为准并同步文档。
- 中断处理函数必须保留 `IRAM_ATTR`，与中断共享的数据继续使用 `volatile` 或等价保护。
- 串口、Web Console、TCP Console 输入都视为不可信边界；控制命令必须经过认证/权限和范围校验后才能影响输出。
- `ENABLE_DIAGNOSTIC_COMMANDS` 与 `ENABLE_BOOT_STEERING_SELF_TEST` 默认应保持注释状态；`tests/test_firmware_feature_flags.py` 会保护这一点。

## Documentation Map

- `Doc/Arch/architecture.md`：固件主循环、状态机和数据流。
- `Doc/Hardware/pin_definitions.md`：MUS4-v2.3 权威引脚定义。
- `Doc/Hardware/CONFIG.md`：硬件配置说明。
- `Doc/Tools/ArduinoCLI.md`：`arduino-cli.py` 使用说明。
- `Doc/Tools/arduino-cli-wsl_manual.md`：WSL 构建脚本背景与排障。
- `Doc/Tools/train_tub_driver.md`：Tub JSON 报告与 GRU baseline 训练说明。
- `Doc/Tools/mus4_pilot_infer.md`：Pilot 模型推理控制器、安全门控和部署说明。
- `Doc/README/OPERATIONS.md`：串口运行时操作命令与数据帧。
- `provisioning_system/docs/deployment_and_testing.md`：独立配网系统的 ESP32 固件、Linux agent 和测试部署说明。
- `Doc/Plan/`：方案、设计方案、实施路线和历史实施方案目录；新增方案类内容应写入此目录，使用清晰的中文文件名，使用前需对照当前代码验证。
- `README.md`：项目早期介绍，部分构建命令、硬件引脚和文档路径已滞后；引用前必须对照 `mus4.ino`、`Doc/Hardware/pin_definitions.md`、`Doc/Arch/architecture.md` 和本文件验证。
- `AGENTS.md`：其他代理工具的历史指南，包含旧版本号和外部工具专属指令；Claude Code 操作本仓库时优先遵循本文件、源码和当前项目文档。

## Git Conventions

- 主分支：`master`
- 特性分支命名：`v{版本号}-{特性}`，例如 `v1.2-WSL-Build`
- 提交信息遵循 Conventional Commits，并使用中文，例如：`fix(arduino-cli): 修复串口上传进度回退闪烁的问题`
- 完成实现且相关测试、编译或实机验证通过后，可主动创建本地稳定版本提交；不要自动 push，远端操作仍需用户明确授权。
