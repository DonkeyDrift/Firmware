# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with this repository.

## Project Overview

**MUS4 (LP-MU-S4)** 是基于 ESP32 的扩展板控制系统，专为遥控车辆/机器人应用设计。当前版本适配 MUS4-v2.3 PCB，实现了 RC 接收机信号处理、多模式驾驶控制、紧急制动机制、I2C 传感器采集、蓝牙游戏手柄模式等功能。

### 项目根目录结构
- 主程序源码位于**根目录**（v1.1 版本已从 `mus4/` 子目录迁移）
- `build/` - 编译输出目录
- `tests/` - Python 端构建脚本测试
- `Doc/` - 架构、硬件、开发文档
- `examples/` - 示例代码
- `multi_agent_framework/` - 多智能体框架相关模块
- `provisioning_system/` - 烧录/配置相关工具

## Build Commands

### 常用构建命令（通过 `arduino-cli.py` 脚本）
```bash
# 仅编译
python arduino-cli.py -c

# 仅上传（自动检测串口，或通过 --port 指定）
python arduino-cli.py -u
python arduino-cli.py -u --port COM9

# 编译 + 上传
python arduino-cli.py -cu

# 编译 + 上传 + 串口监视
python arduino-cli.py -cus

# 使用预编译固件
python arduino-cli.py -u -i build/mus4.ino.bin

# 列出所有匹配的串口
python arduino-cli.py --list-ports

# 关闭单行进度条（逐行输出，适合日志重定向）
python arduino-cli.py -cu --no-progress
```

### WSL 交叉编译（Windows + WSL 环境）
```powershell
# 完整流程：WSL 中编译 -> Windows 端串口上传
.\arduino-cli-wsl.ps1

# 常用参数
.\arduino-cli-wsl.ps1 -Verbose        # 显示详细日志
.\arduino-cli-wsl.ps1 -SkipCompile    # 跳过 WSL 编译，直接上传已有固件
.\arduino-cli-wsl.ps1 -SkipUpload     # 仅编译，不上传
.\arduino-cli-wsl.ps1 -Port COM9      # 指定串口
```

**构建脚本分工**：
- `arduino-cli.py` - 跨平台主脚本，负责编译、上传、串口监控、串口自动匹配等核心逻辑
- `arduino-cli-wsl.ps1` - Windows 端包装脚本，负责调用 WSL 编译后再通过 Windows 串口烧录

### Python 侧测试
```bash
# 运行全部测试
pytest tests/

# 运行单个测试文件
pytest tests/test_arduino_cli.py

# 运行单个测试用例
pytest tests/test_arduino_cli.py -k "test_prefers_explicit_port_when_available"

# 查看详细输出
pytest tests/ -v
```

测试目录结构：
- `tests/unit/` - 单元测试（进度解析、串口匹配等纯函数）
- `tests/integration/` - 集成测试（编译、上传流程）

## Configuration

### 核心配置文件
- `config.yaml` - 构建脚本配置，包含开发板型号、串口自动检测规则、波特率等
- `sketch.yaml` - Arduino CLI 项目级配置（默认 FQBN、默认端口）

### 串口自动检测
`config.yaml` 中 `serial_detection` 段配置了 ESP32/CP210/CH340/FTDI 等常见芯片的 VID/PID 和描述关键字，脚本会自动匹配合适的串口，无需手动指定 `--port`。

### 开发板配置
- 默认 FQBN（config.yaml）: `esp32:esp32:esp32`（通用 ESP32 Dev Module）
- 备用 FQBN（sketch.yaml）: `esp32:esp32:dfrobot_firebeetle2_esp32e`（FireBeetle 2 ESP32-E）
- 默认波特率: 115200

## Architecture

### 代码层次划分
```
┌──────────────────────────────────────────┐
│  构建工具层（Python / PowerShell）        │
│  arduino-cli.py + arduino-cli-wsl.ps1     │
│  负责：编译编排、串口自动匹配、进度解析    │
└──────────────────────────────────────────┘
                      ↓ 产物为 .bin 固件
┌──────────────────────────────────────────┐
│  嵌入式应用层（Arduino/C++，根目录）       │
│  mus4.ino → 主状态机、中断、串口协议       │
│  SharedTypes.h → 跨模块数据结构契约        │
│  TUI.h/cpp → 终端仪表盘渲染               │
│  Buzzer.h/cpp → 蜂鸣器状态机              │
└──────────────────────────────────────────┘
                      ↓ I2C / PWM / UART
┌──────────────────────────────────────────┐
│  硬件层（MUS4-v2.3 PCB）                  │
│  ESP32、RC 接收机、舵机/电调、IMU、INA219  │
└──────────────────────────────────────────┘
```

### 关键编译宏
- `ENABLE_GAMEPAD_MODE` - 启用蓝牙游戏手柄模式，将 RC 通道映射为 BLE 游戏手柄轴
- 当前在 `mus4.ino` 中默认启用，修改后需重新编译

### 主入口文件
`./mus4.ino` - 主 Arduino sketch，包含:
- RC PWM 输入中断处理（GPIO 36, 39, 34, 26）
- 上位机串口命令解析（支持 `T:S` 格式、校验和、序列号）
- 模式切换控制循环
- 多级紧急制动状态机

### 核心头文件
- `SharedTypes.h` - 公共数据结构（SensorData、ControlData、模式常量、状态枚举）
- `TUI.h/cpp` - 基于 ANSI 转义序列的终端 UI 类，实现类 nvtop 的仪表盘
- `Buzzer.h/cpp` - 蜂鸣器控制（根目录，部分版本硬件支持）

### 控制模式
| 模式 | ID | 转向来源 | 油门来源 | LED 颜色 |
|------|----|----------|----------|----------|
| 手动 | 0 | RC 接收机 | RC 接收机 | 绿色 |
| 半自动 | 1 | 上位机 Pilot | RC 接收机 | 黄色 |
| 全自动 | 2 | 上位机 Pilot | 上位机 Pilot | 蓝色 |

### 核心数据结构
```cpp
struct struct_message {
    int throttle;  // -100 ~ 100
    int steering;  // -100 ~ 100
    int mode;      // 0=手动, 1=半自动, 2=全自动
    bool park;     // 上锁/解锁状态
};
```

## Serial Protocol

- **输入格式**: `Throttle:Steering\n`（例如 `10:20\n`）
- **带序列号**: `Throttle:Steering:Seq\n`
- **带校验和**: `Throttle:Steering*XX\n`（XX 为十六进制校验和）
- **响应**: `ACK` / `ACK:Seq` 或 `NACK` / `NACK:Seq`
- **输出反馈**: 通过 Serial1（RS232）输出 `Txx:Sxx\n`

## Pin Configuration (MUS4-v2.3 PCB)

| 功能 | GPIO | 说明 |
|------|------|------|
| RC CH1（转向） | 36 | 仅输入 |
| RC CH2（油门） | 39 | 仅输入 |
| RC CH3（Park） | 34 | 仅输入 |
| RC CH4（模式） | 26 | |
| 转向舵机 | 23 | PWM 50Hz |
| 油门电调 | 25 | PWM 50Hz |
| LED（WS2812B） | 5 | |
| Serial1 RX | 16 | RS232 |
| Serial1 TX | 17 | RS232 |
| I2C SDA | 21 | INA219（电源监测）、MPU6050（IMU） |
| I2C SCL | 22 | INA219、MPU6050 |

> 注意：README 中记录的是旧版硬件引脚（CH4=35, OUT1/2=32/33, I2C=13/14），以本文件的 v2.3 版本为准。

## BLE Gamepad Mode

通过 `#define ENABLE_GAMEPAD_MODE` 宏启用，将 RC 通道映射为游戏手柄轴:
- CH1（转向）→ 右摇杆 X 轴
- CH2（油门）→ 左摇杆 Y 轴

设备名称: "Gamepad MU02"

## Runtime Test Commands (串口交互)

在串口监视器中发送以下命令进行测试:
- `TEST` - 运行命令解析单元测试
- `BENCH` - TUI 渲染性能基准测试
- `STRESS` - 压力测试
- `REGRESS` - 回归测试
- `ANSI` / `NOANSI` - 切换 ANSI 转义序列显示

## Git Conventions

### 分支命名
- `master` - 主分支，稳定版本
- `v{版本号}-{特性}` - 特性分支，例如 `v1.2-WSL-Build`

### 提交规范
遵循约定式提交（Conventional Commits），并用中文书写：
- `feat:` - 新功能
- `fix:` - 修复 Bug
- `refactor:` - 重构
- `test:` - 测试相关
- `chore:` - 构建工具/辅助工具变动
- `docs:` - 文档更新

示例：`fix(arduino-cli): 修复串口上传进度回退闪烁的问题`

## Documentation

详细文档位于 `Doc/` 目录:
- `Arch/architecture.md` - 系统架构（含图示）
- `Hardware/pin_definitions.md` - 完整引脚定义
- `Tools/ArduinoCLI.md` - 构建脚本使用说明
- `README/DevNote.md` - 开发笔记
