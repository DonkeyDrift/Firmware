# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MUS4（LP-MU-S4）是基于 ESP32 的遥控车辆/机器人底层控制系统，当前代码面向 MUS4-v2.3 PCB。固件负责 RC PWM 输入采集、Pilot 上位机串口控制、多模式控制融合、Park/紧急制动、I2C 传感器采集、TUI 状态显示与可选 BLE Gamepad 输出。

根目录同时包含两类代码：
- Arduino/C++ 固件：主 sketch 在根目录 `mus4.ino`，公共类型与外设辅助模块也在根目录。
- 构建与烧录工具：`arduino-cli.py` 与 `arduino-cli-wsl.ps1` 负责编译、上传、串口检测、WSL 加速构建。

## Commands

### Python 依赖

```bash
pip install pyyaml pyserial pytest
```

项目未发现专用 Python lint/format 配置；若修改 Python 工具脚本，至少运行相关 `pytest`。

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
```

常用参数：
- `--config config.yaml`：指定构建配置文件。
- `--fqbn esp32:esp32:esp32`：覆盖开发板 FQBN。
- `--sketch mus4.ino`：覆盖 sketch 路径。
- `--baud 115200`：覆盖串口波特率。
- `--auto-reset` / `--no-auto-reset`：控制串口监视前复位。
- `--regress-reset --regress-count 10`：运行复位回归测试。

### Windows + WSL 加速构建

```powershell
# 默认执行编译并上传；脚本自动探测项目根目录、WSL 发行版、sketch 与 FQBN
.\arduino-cli-wsl.ps1

# 仅编译
.\arduino-cli-wsl.ps1 -Compile

# 仅上传已有 build_wsl 产物
.\arduino-cli-wsl.ps1 -Upload

# 编译 + 上传 + 串口监视
.\arduino-cli-wsl.ps1 -Compile -Upload -Serial

# 指定串口会透传给 arduino-cli.py
.\arduino-cli-wsl.ps1 -Upload -ExtraArgs "--port COM9"

# 清理 WSL 构建目录后重新编译
.\arduino-cli-wsl.ps1 -Compile -Clean

# 构建在 /mnt/c 上而不是同步到 WSL ext4，适合快速排查同步问题
.\arduino-cli-wsl.ps1 -Compile -IoMode mnt

# 检查 WSL、rsync、arduino-cli 等依赖
.\arduino-cli-wsl.ps1 -Check
```

WSL 脚本默认 `IoMode=native`：先把源码同步到 WSL 原生文件系统，再在 `build_wsl/` 生成并回传 `.bin` / `.elf` 产物，最后通过 Windows 端 `arduino-cli.py` 上传。

### Python 测试

```bash
# 运行全部 Python 测试
pytest tests/

# 运行单个测试文件
pytest tests/test_arduino_cli.py

# 运行单个测试用例
pytest tests/test_arduino_cli.py -k "test_prefers_explicit_port_when_available"

# 详细输出
pytest tests/ -v
```

### 固件运行时串口测试命令

在 USB Serial 或 Serial1 监视器中输入并回车：
- `TEST`：运行固件内置命令解析测试。
- `TEST_TUI`：TUI 测试入口，当前输出 skipped。
- `BENCH`：运行 TUI/循环性能基准。
- `STRESS`：运行串口压力统计。
- `REGRESS`：运行固件回归校验。
- `FILTER_TEST`：运行 RC 滤波测试入口。
- `ANSI` / `NOANSI`：切换 TUI ANSI 转义序列显示。

## Configuration

- `config.yaml`：`arduino-cli.py` 的主配置，包含 `arduino_cli`、`fqbn`、`port`、`baudrate`、`sketch_path`、`build_path`、串口自动检测与日志配置。
- `sketch.yaml`：Arduino CLI 项目级默认配置，当前默认 FQBN 为 `esp32:esp32:dfrobot_firebeetle2_esp32e`，默认端口为 `/dev/ttyS4`。
- `wslbuild.yaml`（若存在）：`arduino-cli-wsl.ps1` 会读取其中的 `distro`、`sketch`、`fqbn`、`work_dir` 等覆盖项。

当前 `config.yaml` 默认：
- FQBN：`esp32:esp32:esp32`
- sketch：`mus4.ino`
- build path：`build`
- baudrate：`115200`
- port：`auto`
- 日志文件：`ArduinoCLI.log`

串口自动检测会根据描述、厂商、VID/PID 和首选关键字匹配 ESP32/CP210/CH340/FTDI 等 USB 串口；双串口板当前优先 `SERIAL-A`。

## High-Level Architecture

### 构建工具层

`arduino-cli.py` 是跨平台主入口：读取 `config.yaml`，封装 `arduino-cli compile/upload/monitor`，处理串口自动匹配、上次成功端口缓存、双串口回退、复位、进度输出和日志。

`arduino-cli-wsl.ps1` 是 Windows/WSL 包装层：自动探测 WSL 环境，把源码同步到 WSL 原生文件系统编译，回传产物后调用 `arduino-cli.py` 执行 Windows 端串口上传。

Python 测试集中在 `tests/test_arduino_cli.py`，主要覆盖串口选择、双串口回退、进度解析和上传命令构造等纯逻辑。

### 固件应用层

`mus4.ino` 是主状态机，关键数据流是：
1. RC 接收机通过 CH1-CH4 PWM 输入触发中断，计算脉宽并滤波。
2. USB `Serial` 与 `Serial1` 接收 Pilot 指令，解析 `Throttle:Steering`、序列号与校验和格式。
3. 控制逻辑按模式融合 RC 与 Pilot 数据，更新 `car_output`。
4. Park/紧急制动状态机可覆盖油门输出并控制 LED 闪烁。
5. 输出层通过 ESP32 `ledc` 产生 PWM，驱动转向舵机与油门电调，并通过 Serial1 回传 `Txx:Sxx`。
6. TUI、I2C 传感器和 BLE Gamepad 作为旁路功能读取状态并输出显示/手柄轴数据。

### 核心模块

- `SharedTypes.h`：跨模块共享的数据结构与状态枚举，例如 `SensorData`、`ControlData`。
- `TUI.h` / `TUI.cpp`：ANSI 终端仪表盘渲染，支持降级模式和增量刷新。
- `Buzzer.h` / `Buzzer.cpp`：蜂鸣器状态机，硬件支持时使用。
- `examples/`：I2C、传感器等独立示例 sketch。
- `multi_agent_framework/`：独立 Python 多智能体框架代码，不属于 ESP32 固件主链路。
- `provisioning_system/`：ESP32 Wi-Fi provisioning 与 Linux agent 相关工具，独立于 MUS4 主固件构建流程。

## Firmware Behavior Reference

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
- Serial1 输出 `Txx:Sxx\n`

### 当前 v2.3 引脚

| 功能 | GPIO | 说明 |
| --- | --- | --- |
| RC CH1 转向 | 36 | 仅输入 |
| RC CH2 油门 | 39 | 仅输入 |
| RC CH3 Park | 34 | 仅输入 |
| RC CH4 模式 | 26 | PWM 输入 |
| 转向舵机 | 23 | `ledc` PWM，当前 300Hz/14bit |
| 油门电调 | 25 | `ledc` PWM，当前 300Hz/14bit |
| 备用 PWM_1 | 32 | 预留输出 |
| 备用 PWM_2 | 33 | 预留输出 |
| WS2812B LED | 5 | 模式与紧急停车指示 |
| UART_SEL | 12 | UART 路由选择 |
| Serial1 RX | 16 | RS232/Pilot 输入 |
| Serial1 TX | 17 | RS232/Pilot 输出 |
| I2C SDA | 21 | INA219 / MPU6050 |
| I2C SCL | 22 | INA219 / MPU6050 |

README 中仍包含旧版引脚和旧路径；以 `mus4.ino`、`Doc/Hardware/pin_definitions.md` 和本文件为准。

### BLE Gamepad Mode

`mus4.ino` 当前定义了 `ENABLE_GAMEPAD_MODE`。启用时将 RC 通道映射为 BLE Gamepad：
- CH1 转向 → 右摇杆 X 轴。
- CH2 油门 → 左摇杆 Y 轴。

设备名称在代码中维护；修改 BLE 相关行为后需要重新编译并在目标主机上重新配对验证。

### Drift Assist

`mus4.ino` 当前包含漂移辅助编译开关与参数：`DRIFT_ASSIST_ENABLED`、增益、阈值、最大补偿和平滑/衰减系数。相关逻辑依赖 IMU 角速度与 CH3 档位状态，修改时同时检查 Park/解锁语义，避免与安全状态机冲突。

## Documentation Map

- `Doc/Arch/architecture.md`：固件主循环、状态机和数据流。
- `Doc/Hardware/pin_definitions.md`：MUS4-v2.3 权威引脚定义。
- `Doc/Hardware/CONFIG.md`：硬件配置说明。
- `Doc/Tools/ArduinoCLI.md`：`arduino-cli.py` 使用说明。
- `Doc/Tools/arduino-cli-wsl_manual.md`：WSL 构建脚本背景与排障。
- `Doc/README/OPERATIONS.md`：串口运行时操作命令与数据帧。
- `Doc/Plan/`：历史实施方案，使用前需对照当前代码验证。

## Git Conventions

- 主分支：`master`
- 特性分支命名：`v{版本号}-{特性}`，例如 `v1.2-WSL-Build`
- 提交信息遵循 Conventional Commits，并使用中文，例如：`fix(arduino-cli): 修复串口上传进度回退闪烁的问题`
