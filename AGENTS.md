> **Agent 阅读说明**：本文件面向 AI 编码代理。项目的主要人类文档是 `README.md`，而本文件补充构建、测试、代码风格与安全等代理需要快速掌握的约定。若本文件与源码冲突，**以源码和 `CHANGELOG.md` 为准**。

# AGENTS.md - MUS4 项目编码指南

MUS4（LP-MU-S4）是基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件。当前主 Sketch 为根目录 `MUS4_FW.ino`（约 560 行），固件版本 `v1.7.4`（定义于 `BuildInfo.h`），目标硬件为 MUS4-v2.4.2 PCB（兼容 v2.3）。固件负责 RC 接收机 PWM 输入采集、Pilot 上位机串口控制、多模式驾驶控制融合、Park/紧急制动状态机、I2C 传感器采集、TUI 状态显示，以及可选的 Wi-Fi 控制台、OTA 更新和 BLE 游戏手柄输出。

---

## 1. 项目概览

### 1.1 技术栈

- **固件语言**: C++17 with Arduino framework
- **目标平台**: ESP32（MUS4-v2.4.2 PCB，兼容 v2.3）
- **构建工具**: `arduino-cli`（核心），Python 3 封装脚本 `arduino-cli.py`（约 1334 行），PowerShell WSL 加速脚本 `arduino-cli-wsl.ps1`
- **主要依赖库**:
  - `FastLED` — WS2812B LED 驱动
  - `Adafruit_MPU6050` / `Adafruit_Sensor` — IMU 传感器
  - `Adafruit_INA219` — 电压/电流监测
  - `Wire` — I2C 通信
  - `WiFi` / `WebServer` / `DNSServer` / `ESPmDNS` / `ArduinoOTA` / `Update` / `Preferences` — Wi-Fi AP/STA、Web 控制台、OTA、NVS 持久化
  - `AsyncTCP` / `ESPAsyncWebServer` — WebSocket 遥测（仅在启用 `ENABLE_WIFI_WEBSOCKET_TELEMETRY` 时生效）
  - `BleGamepad` — 蓝牙游戏手柄模式（仅在未启用 `ENABLE_WIFI_CONSOLE` 时生效）
- **Python 依赖**: `pyyaml`, `pyserial`, `pytest`
- **包管理器清单**: 项目根目录不存在 `pyproject.toml`、`package.json`、`Cargo.toml` 或 `Makefile`；构建入口是 Python 脚本和 PowerShell 脚本。`pyproject.toml` 仅出现在 `libraries/FastLED/` 等第三方库内部，`package.json` 仅出现在 `provisioning_system/playwright_tests/`（当前为占位脚本）。

### 1.2 关键配置文件

| 文件 | 用途 | 备注 |
|------|------|------|
| `BuildInfo.h` | 固件版本宏 | `v1.7.4`，构建日期/时间由编译器 `__DATE__` / `__TIME__` 注入 |
| `FirmwareConfig.h` | 编译期功能开关、引脚定义、时序/滤波/日志目标 | 所有 `.h/.cpp` 均优先包含此文件 |
| `config.yaml` | `arduino-cli.py` 主配置 | FQBN、端口、波特率、串口自动检测关键字、复位策略、日志级别 |
| `sketch.yaml` | Arduino CLI 项目级默认配置 | FQBN 与端口 |
| `wslbuild.yaml` | WSL 构建脚本覆盖配置 | distro（当前 `DKC`）、工作目录 `/home/dkc/arduino-build/MUS4_FW`、`io_mode`、库同步规则 |
| `ArduFlux.json` | ArduFlux IDE 配置文件 | 当前板型、端口、Monitor 参数（不建议纳入版本控制，见 `.gitignore`） |
| `WirelessSecrets.h` | 本地 Wi-Fi STA SSID/密码 | **不提交**，由 `.gitignore` 排除 |
| `.mus4_ota_target` | HTTP OTA 默认目标主机首行 | **不提交**，由 `.gitignore` 排除 |

### 1.3 代码组织

根目录是多文件 Arduino 项目，主 Sketch `MUS4_FW.ino` 仅保留 `setup()` / `loop()`、全局变量装配和中断快照读取，所有业务逻辑均已拆分为成对的 `.h/.cpp` 模块（部分纯头文件工具除外）：

```text
根目录/
├── MUS4_FW.ino                    # 主固件入口：setup/loop/全局装配/混控
├── BuildInfo.h                    # 固件版本与构建时间宏（v1.7.4）
├── FirmwareConfig.h               # 编译期功能开关、引脚定义、时序/滤波/日志目标等核心常量
├── SharedTypes.h                  # 跨模块共享数据结构（SensorData、ControlData）
├── RuntimeState.h                 # Wi-Fi / OTA 运行时聚合状态结构体（WifiRuntimeState / OtaRuntimeState）
├── SerialBufferTypes.h            # SerialBuf 结构体定义
├── StringPrint.h                  # 基于 String 的 Print 实现（header-only）
├── WifiConsoleTypes.h             # Wi-Fi 控制台常量、WebLogEntry/WebDataPoint 等结构体
├── WebConsoleAssets.h             # Web Console HTML（Drifter Console）与 OTA 上传页（PROGMEM）
├── WirelessSecrets.h              # 本地 Wi-Fi STA 凭据（本地文件，不提交）
├── WirelessSecrets.example.h      # 凭据模板
├── TUI.h / TUI.cpp                # ANSI 终端仪表盘，支持脏矩形增量刷新与降级模式
├── Buzzer.h / Buzzer.cpp          # 蜂鸣器状态机（模式/停车提示音）
├── LedStatus.h / LedStatus.cpp    # WS2812B LED 颜色与闪烁控制
├── Mus4Log.h / Mus4Log.cpp        # 日志路由（Serial / Web）
├── Sensors.h / Sensors.cpp        # INA219 / MPU6050 读取与 I2C 扫描
├── I2CBusTools.h / I2CBusTools.cpp# I2C 底层读写与设备探测
├── RcFilter.h / RcFilter.cpp      # RC 6 通道滑动窗口中值滤波与防抖
├── RcPwmCapture.h / RcPwmCapture.cpp       # RC PWM 输入捕获：中断、MCPWM、脉冲验证
├── ControlMixer.h / ControlMixer.cpp       # 驾驶模式切换、RC/Pilot 控制融合、Drift Assist
├── SafetyState.h / SafetyState.cpp         # Park 状态机、紧急制动 FSM
├── ActuatorOutput.h / ActuatorOutput.cpp   # PWM 执行器输出：舵机/电调映射与驱动
├── CommandParser.h / CommandParser.cpp     # 串口命令解析、校验和、单元测试入口
├── LocalCommands.h / LocalCommands.cpp     # 本地串口行处理（Pilot 数据帧解析）
├── CommandDispatcher.h / CommandDispatcher.cpp # 命令分发（本地/无线/OTA/Wi-Fi STA）
├── SerialLineReader.h / SerialLineReader.cpp   # Serial/Serial1 行缓冲读取
├── WirelessConsole.h / WirelessConsole.cpp     # 无线命令权限与命令分类
├── WifiStaConfig.h / WifiStaConfig.cpp         # STA 配置持久化与状态管理
├── WifiIdentity.h / WifiIdentity.cpp           # AP SSID / mDNS 主机名校验
├── WifiOta.h / WifiOta.cpp                     # OTA 窗口、ArduinoOTA 生命周期、Park 保护
├── WebLogBuffer.h / WebLogBuffer.cpp           # Web 日志 ring buffer 与日志桥接
├── WebConsoleServer.h / WebConsoleServer.cpp   # HTTP route、API handler、captive portal、OTA upload
├── WebTelemetry.h / WebTelemetry.cpp           # WebSocket 遥测、数据采样与推送
├── WifiManager.h / WifiManager.cpp             # Wi-Fi runtime 状态机（AP/STA/mDNS/DNS/TCP Console）
├── DriftAssist.h / DriftAssist.cpp             # 漂移辅助：基于 GyroZ 与 CH5/CH6 的转向补偿
├── SteeringControl.h / SteeringControl.cpp     # 转向 PID 平滑与故障安全
├── SteeringCalibration.h / SteeringCalibration.cpp # 转向通道交互式标定
├── Diagnostics.h / Diagnostics.cpp             # 降级检测、BENCH/STRESS/REGRESS 诊断入口
├── GamepadMode.h / GamepadMode.cpp             # BLE 游戏手柄输出
├── JsonUtil.h / JsonUtil.cpp      # JSON 字符串转义辅助
├── config.yaml                    # arduino-cli.py 主配置
├── sketch.yaml                    # Arduino CLI 项目级默认配置
├── wslbuild.yaml                  # WSL 构建脚本覆盖配置
├── ArduFlux.json                  # ArduFlux IDE 配置文件
├── .mus4_ota_target               # HTTP OTA 默认目标主机（本地文件，不提交）
├── arduino-cli.py                 # 跨平台构建/上传/监控 Python 主入口
├── arduino-cli-wsl.ps1            # Windows WSL 加速构建与 OTA 上传包装脚本
├── wireless_console_policy.py     # Wi-Fi/TCP/Web Console 权限策略的 Python 镜像
├── tests/                         # Python 单元/集成测试
│   ├── test_arduino_cli.py        # 串口选择、OTA 工具链、编译命令的单元测试
│   ├── test_wireless_console_policy.py # 无线权限策略、Web Log Buffer、网络状态格式化测试
│   ├── test_firmware_feature_flags.py  # 源码结构断言（81 个 test 函数、约 958 处断言）
│   ├── test_train_tub_driver.py   # Tub 训练工具测试
│   └── test_mus4_pilot_infer.py   # Pilot 推理控制器测试
├── tools/                         # 模型与数据采集工具
│   ├── train_tub_driver.py        # Tub JSON 行为克隆训练工具
│   └── mus4_pilot_infer.py        # Pilot 推理控制器
├── examples/                      # 独立示例 Sketch
│   ├── getcurrent/                # INA219 电流读取示例
│   ├── testIIC/                   # I2C 扫描与通信测试
│   └── smart_provisioning/        # Wi-Fi 配网示例（AP + Web Server）
├── docs/                          # 项目文档（中文为主）
│   ├── Arch/architecture.md       # 固件主循环、状态机、数据流架构
│   ├── Hardware/pin_definitions.md# 权威引脚定义（v2.3/v2.4.2）
│   ├── Hardware/CONFIG.md         # 硬件配置说明
│   ├── Tools/ArduinoCLI.md        # arduino-cli.py 使用说明
│   ├── Tools/arduino-cli-wsl_manual.md # WSL 构建背景与排障
│   ├── Tools/train_tub_driver.md  # Tub 训练工具说明
│   ├── Tools/mus4_pilot_infer.md  # Pilot 推理工具说明
│   ├── README/DevNote.md          # 开发环境配置、串口协议、常量表
│   ├── README/OPERATIONS.md       # 串口运行时操作命令与数据帧
│   ├── Plan/                      # 设计方案与实施路线
│   ├── Algo/                      # 算法逻辑说明
│   ├── Inspect/                   # 问题排查与分析
│   ├── Guide/                     # 操作指南
│   └── Valid/                     # 验证指南
├── multi_agent_framework/         # 独立多智能体协作框架（Python）
│   ├── framework/                 # 智能体核心与消息队列 IPC
│   ├── esp32_firmware/            # 生成的 ESP-IDF 示例固件
│   ├── linux_scripts/             # 系统监控 Shell 脚本
│   ├── web_ui/                    # WebSocket 控制面板
│   └── docs/README.md             # 框架说明
└── provisioning_system/           # 独立 Wi-Fi 配网系统
    ├── docs/
    ├── esp32/                     # Arduino 配网固件（AP + Web Server）
    ├── linux_agent/               # Linux 配网代理守护进程
    ├── playwright_tests/          # Web UI 端到端测试资源（当前为占位脚本）
    ├── tests/                     # 配网代理单元测试
    └── docs/deployment_and_testing.md # 部署与测试文档
```

---

## 2. 构建与上传命令

### 2.1 Python 依赖

```bash
pip install pyyaml pyserial pytest
```

### 2.2 WSL 加速构建（推荐）

**WSL 编译是首选方式**，相比 Windows 原生编译速度提升 3-5 倍。当前 `wslbuild.yaml` 配置：发行版 `DKC`，工作目录 `/home/dkc/arduino-build/MUS4_FW`，`io_mode: native`。

```powershell
# 一键：编译 + 上传（默认）
.\arduino-cli-wsl.ps1

# 仅编译（默认：同步到 WSL 原生文件系统后编译）
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -c -Sketch MUS4_FW.ino

# 清理 WSL 构建目录后重新编译
.\arduino-cli-wsl.ps1 -Compile -Clean -Sketch MUS4_FW.ino

# 检查依赖与分区占用
.\arduino-cli-wsl.ps1 -Compile -CheckPartition -Sketch MUS4_FW.ino

# 编译后通过 HTTP OTA 上传（优先方式，使用 Web Console /update 端点）
# 优先读取项目根目录 .mus4_ota_target 第一行作为目标地址
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost <设备IP> -Sketch MUS4_FW.ino

# 使用已有 build_wsl 产物通过 HTTP OTA 上传
.\arduino-cli-wsl.ps1 -Upload -HttpOta -HttpOtaHost <设备IP> -Sketch MUS4_FW.ino

# 若 .mus4_ota_target 已存在，可省略 -HttpOtaHost
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -Sketch MUS4_FW.ino

# 仅当明确要求串口上传/监控时
.\arduino-cli-wsl.ps1 -Upload -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -Upload -Serial -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -s

# 回退：旧 ArduinoOTA 方式（端口 3232）
.\arduino-cli-wsl.ps1 -Compile -Upload -Ota -OtaHost <设备IP> -Sketch MUS4_FW.ino
```

### 2.3 原生构建（Windows/Linux/macOS）

```bash
# 仅编译
python arduino-cli.py -c --sketch MUS4_FW.ino

# 仅上传（按 config.yaml 自动检测串口）
python arduino-cli.py -u --sketch MUS4_FW.ino

# 指定串口上传
python arduino-cli.py -u --sketch MUS4_FW.ino --port COM9

# 编译 + 上传
python arduino-cli.py -cu --sketch MUS4_FW.ino

# 编译 + 上传 + 串口监控
python arduino-cli.py -cus --sketch MUS4_FW.ino

# 使用预编译固件上传
python arduino-cli.py -u -i build/MUS4_FW.ino.bin --sketch MUS4_FW.ino

# OTA 上传（ArduinoOTA，旧方式）
python arduino-cli.py --ota -i build_wsl/MUS4_FW.ino.bin --ota-host mus4-ota

# HTTP OTA 上传（新方式，使用 curl.exe）
curl.exe -X POST http://<设备IP>/update -F "firmware=@build_wsl/MUS4_FW.ino.bin" --progress-bar

# 列出检测到的串口
python arduino-cli.py --list-ports

# 逐行日志（适合 CI）
python arduino-cli.py -cu --no-progress --sketch MUS4_FW.ino
```

### 2.4 关键配置参数

- **默认 FQBN**: `esp32:esp32:esp32:PartitionScheme=min_spiffs`（`config.yaml` / `sketch.yaml`）
- **默认波特率**: `115200`
- **默认 Sketch**: `MUS4_FW.ino`
- **构建输出目录**: `build/`（原生）或 `build_wsl/`（WSL）
- **本地库目录**: `libraries/`（存在时构建脚本会自动追加 `--libraries`）
- **日志文件**: `ArduinoCLI.log`

---

## 3. 测试策略

### 3.1 Python 单元/集成测试

```bash
# 运行全部 Python 测试
pytest tests/

# 运行单个测试文件
pytest tests/test_arduino_cli.py
pytest tests/test_wireless_console_policy.py
pytest tests/test_firmware_feature_flags.py
pytest tests/test_train_tub_driver.py
pytest tests/test_mus4_pilot_infer.py
```

测试说明：
- `test_arduino_cli.py`：基于 `unittest`，使用 `MagicMock` 对 `arduino-cli.py` 的串口选择、OTA 工具发现、编译命令组装、上传重试逻辑进行单元测试。
- `test_wireless_console_policy.py`：基于 `unittest`，验证 `wireless_console_policy.py` 中的权限矩阵、Wi-Fi 状态格式化、Web Log Buffer、Tub 数据包格式。
- `test_firmware_feature_flags.py`：基于 `pytest`，包含 **81 个 test 函数、约 958 处源码级结构断言**。它读取固件源码并验证：模块是否正确拆分、符号是否存在于预期文件、Web Console HTML/JS/CSS 结构、Wi-Fi 状态机行为、编译开关状态等。**修改固件源码（尤其是 Web Console UI）后必须同步更新此测试并确保通过。** 当前该文件对 Web Console CSS 结构有较强的断言，若仅修改 UI 样式而未同步测试，会导致断言失败。
- `test_train_tub_driver.py` / `test_mus4_pilot_infer.py`：工具链测试。

### 3.2 固件运行时串口测试命令

在 USB Serial（115200 baud）、Serial1、TCP Console 或 Web Console 中发送以下命令（回车结尾）。无线入口受认证和 Park 权限限制：

| 命令 | 说明 |
|------|------|
| `TEST` | 运行固件内置命令解析单元测试 |
| `TEST_TUI` | TUI 测试入口（当前输出 skipped） |
| `BENCH` | 运行 TUI/循环性能基准测试 |
| `STRESS` | 运行串口压力统计 |
| `REGRESS` | 运行固件回归校验 |
| `FILTER_TEST` | 运行 RC 滤波测试 |
| `FILTER_DEBUG` | 切换 RC 滤波调试输出 |
| `STEER_CAL` / `CAL_SAVE` / `CAL_RETRY` / `CAL_ABORT` / `CAL_RESET` / `CAL_STATUS` | 转向通道交互式标定流程 |
| `WIFI_STA_SSID` / `WIFI_STA_PASSWORD` / `WIFI_STA_APPLY` / `WIFI_STA_CLEAR` / `WIFI_STA_STATUS` | Wi-Fi STA 配置与状态 |
| `ENABLE_OTA` / `OTA_STATUS` / `DISABLE_OTA` | OTA 窗口控制 |
| `LOG_WEB` / `LOG_SERIAL` | 切换 MUS4 日志输出目标 |
| `ANSI` / `NOANSI` | 切换 TUI ANSI 转义序列显示 |

### 3.3 独立子项目测试

```bash
# 配网代理测试
python provisioning_system/tests/test_agent.py -v
```

> `provisioning_system/playwright_tests/` 下存在 Playwright 资源，但当前 `npm test` 是占位脚本并会退出失败；除非先补充有效测试脚本，否则不要把它当作验证命令。

---

## 4. 运行时架构

### 4.1 主循环时序

`loop()` 采用非阻塞架构，通过 `millis()` 时间间隔控制各任务频率（具体值来自 `FirmwareConfig.h`）：

| 任务 | 间隔 | 频率 |
|------|------|------|
| 传感器读取（INA219 / MPU6050） | 2 ms | ~500 Hz |
| RC 数据更新与 Serial1 遥测 | 2 ms | ~500 Hz |
| RC 滑动窗口中值滤波 | 2 ms | ~500 Hz |
| TUI 渲染 | 动态 100–500 ms | 自适应降级 |
| 波形图刷新 | 250 ms | 4 Hz |
| 性能评估与降级检测 | 1000 ms | 1 Hz |
| 主循环 delay | 4 ms | ~250 Hz 基线 |

### 4.2 核心数据流

1. **输入层**: RC 接收机 CH1–CH6 通过 `attachInterrupt` 触发中断，记录 `micros()` 脉宽；USB `Serial`、RS232 `Serial1`、TCP Console、Web Console 接收控制指令。
2. **滤波层**: 6 通道滑动窗口中值滤波（窗口大小 `PWM_FILTER_SIZE = 5`），主通道（转向/油门）附加平滑处理，辅助通道（Park/Mode/Drift/Drift Scale）附加稳定防抖。
3. **控制融合**: 按驾驶模式混合 RC 与 Pilot 数据：
   - **手动 (0)**: 转向/油门均来自 RC
   - **半自动 (1)**: 转向来自 Pilot，油门来自 RC
   - **全自动 (2)**: 转向/油门均来自 Pilot
4. **安全层**: Park 状态机（长按 CH3 锁定/解锁）、紧急制动状态机（`EST_IDLE → READY → BRAKING → DONE`）、Drift Assist 条件判断、转向信号故障安全模式、OTA 期间的强制 Park 保护。
5. **输出层**: `ledc` 生成 PWM（300 Hz / 14 bit）驱动舵机（GPIO 23）与电调（GPIO 25），Serial1 回传 `Txx:Sxx\n`，WS2812B LED 显示模式颜色。

### 4.3 中断约束

- 中断服务函数必须标注 `IRAM_ATTR`。
- 中断与主循环共享的变量必须使用 `volatile`。
- `noInterrupts()` / `interrupts()` 用于保护 PWM 快照读取。

---

## 5. 代码风格规范

### 5.1 命名约定

| 类型 | 风格 | 示例 |
|------|------|------|
| 常量 / 宏 | `ALL_CAPS` | `PWM_MIN_V`, `CH1_PIN`, `CLEAR_SCREEN` |
| 类名 | `PascalCase` | `TUI`, `SensorData`, `Buzzer` |
| 类方法 | `camelCase` | `setRefreshRate()`, `forceRedraw()` |
| 自由函数 | `snake_case` | `process_steering_signal()`, `read_ina219()` |
| 局部变量 | `camelCase` | `pwmValue`, `lastUpdate` |
| 结构体成员 | `snake_case` | `car_output.throttle`, `pilot_data.steering` |
| 私有成员 | 下划线前缀 | `_out`, `_lastUpdate`, `_state` |
| 枚举值 | 嵌套 `PascalCase` | `EmergencyStopState`, `EST_IDLE` |

### 5.2 文件组织

- 头文件使用 `#pragma once`。
- 头文件后缀 `.h`，实现文件后缀 `.cpp`，Arduino 入口 `.ino`。
- 包含顺序：Arduino 核心库 → 第三方库 → 项目头文件（`FirmwareConfig.h` 通常在最前）。
- 所有引脚定义、编译开关、时序常量集中放在 `FirmwareConfig.h`，主 Sketch 和各模块均通过 `#include "FirmwareConfig.h"` 引用。

### 5.3 注释语言

- **硬件相关注释**（引脚、PCB 版本、接线）使用**中文**。
- **代码逻辑与公共 API** 使用**英文**。

---

## 6. 串口协议

### 6.1 输入格式

```text
Throttle:Steering\n
Throttle:Steering:Seq\n
Throttle:Steering*XX\n      # XX = 两位十六进制校验和
```

示例：`10:20\n` 表示油门 10，转向 20。范围限制为 `-100 ~ 100`。

### 6.2 响应格式

- 成功：`ACK` 或 `ACK:Seq`
- 失败：`NACK` 或 `NACK:Seq`

### 6.3 Serial1 状态回传

```text
Txx:Sxx\n
```

OTA 窗口打开或 OTA 传输进行时会暂停 Serial1 遥测。

---

## 7. 安全与关键注意事项

### 7.1 物理安全（最高优先级）

- **该固件直接控制舵机与电调，属于安全关键代码。**
- 修改输出映射、Park、紧急制动、模式融合或无线控制入口时，必须保留 PWM 限幅和失效安全路径。
- 输出前必须使用 `min(max(value, MIN), MAX)` 或 `constrain()` 限制范围。
- 禁止提交可能导致电机失控的代码。

### 7.2 无线安全

- 无线命令分层权限（`wireless_console_policy.py` 与 `WirelessConsole.cpp` 必须保持同步）：
  - **公开**: `PING`, `STATUS`, `AUTH`, `WIFI_STA_STATUS`
  - **需认证**: `ANSI`, `NOANSI`, `FILTER_DEBUG`, `LOG_WEB`, `LOG_SERIAL`, Wi-Fi STA 配置
  - **需认证 + Park 锁定**: `TEST`, `TEST_TUI`, `BENCH`, `STRESS`, `REGRESS`, `FILTER_TEST`, `STEER_CAL`, `CAL_SAVE`, `CAL_RETRY`, `CAL_ABORT`, `CAL_RESET`, `CAL_STATUS`
  - **需认证 + Park 锁定（或 Web 开发模式）**: `ENABLE_OTA`
  - **需认证（或 Web 开发模式）**: `OTA_STATUS`, `DISABLE_OTA`
- 修改认证/权限逻辑时，**必须同步更新** `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py`。
- `WirelessSecrets.h` 与 `.mus4_ota_target` 包含真实凭据或目标地址，**不应纳入版本控制**（已由 `.gitignore` 排除）。
- 开发模式（DEV）持久化到 NVS，开启后 Web Console 可免 AUTH，但仍保留 Park Locked 安全限制；实际 OTA 传输期间固件会默认 Park Locked。

### 7.3 防御性编程

- 假设所有串口/Web/TCP 输入均不可信；控制命令必须经过认证、权限检查和范围校验后才能影响输出。
- 传感器数据使用 `valid` 标志位；读取前检查。
- 状态机用于关键操作（紧急停车、Park 控制、OTA 窗口）。
- 任何新增控制命令或配置项都应在 `wireless_console_policy.py` 中建立对应权限测试。

---

## 8. 配置与部署

### 8.1 编译开关（`FirmwareConfig.h`）

- `ENABLE_WIFI_CONSOLE`：已启用，开启 Wi-Fi AP/TCP/Web Console 全套功能。
- `ENABLE_WIFI_WEBSOCKET_TELEMETRY`：在 `ENABLE_WIFI_CONSOLE` 启用时定义，开启 WebSocket 二进制遥测。
- `ENABLE_GAMEPAD_MODE`：仅在 `ENABLE_WIFI_CONSOLE` 未定义时自动启用，避免 Wi-Fi 与 BLE 共存冲突。
- `ENABLE_DIAGNOSTIC_COMMANDS`：默认注释关闭，开启后增加 `TEST`/`BENCH`/`STRESS`/`REGRESS`/`FILTER_TEST` 等诊断实现。
- `ENABLE_BOOT_STEERING_SELF_TEST`：默认注释关闭。
- `ENABLE_WIFI_NETBIOS_DISCOVERY` / `ENABLE_WIFI_LLMNR_DISCOVERY`：在 `ENABLE_WIFI_CONSOLE` 启用时定义，用于 STA 网络发现。

### 8.2 配网系统部署（独立子项目）

`provisioning_system/` 是独立于主固件构建流程的配网系统：

1. 烧录 `provisioning_system/esp32/esp32_wifi_provisioning` 到 ESP32。
2. 在 Linux 主机（LattePanda MU）部署 `linux_agent/agent.py` 为 systemd 服务。
3. 用户连接 ESP32 AP (`MUS4-AP`)，通过网页提交 SSID/密码。
4. ESP32 通过 UART 将凭据发给 Linux agent，agent 使用 `nmcli` 连接目标网络并回传结果。

详见 `provisioning_system/docs/deployment_and_testing.md`。

---

## 9. Git 规范

- **主分支**: `main`（历史远端分支 `master` 仍存在，但当前工作与远端 HEAD 为 `main`）
- **特性分支命名**: `v{版本号}-{特性}`，例如 `v1.2-WSL-Build`
- **提交信息**: 遵循 Conventional Commits，使用中文，例如 `fix(arduino-cli): 修复串口上传进度回退闪烁的问题`
- **提交前检查**: 确认 `WirelessSecrets.h` / `.mus4_ota_target` 不含敏感信息、相关 Python 测试通过、固件编译通过。
- **不要自动 push**：远端操作需用户明确授权。
- **本地排除文件**（已由 `.gitignore` 处理，勿手动提交）：
  - `WirelessSecrets.h`
  - `.mus4_ota_target`
  - `ArduFlux.json`
  - `build/`, `build_wsl/`
  - `.arduino_cli_state.json`, `.tmp_serial_state_test.json`
  - `*.log`

---

## 10. 文档地图

| 文档 | 内容 |
|------|------|
| `docs/Arch/architecture.md` | 固件主循环、状态机、数据流、时序分析（注意：部分时序参数基于历史版本，以 `FirmwareConfig.h` 为准） |
| `docs/Hardware/pin_definitions.md` | MUS4-v2.3/v2.4.2 权威引脚定义与连接图 |
| `docs/Hardware/CONFIG.md` | 硬件配置说明 |
| `docs/Tools/ArduinoCLI.md` | `arduino-cli.py` 使用说明 |
| `docs/Tools/arduino-cli-wsl_manual.md` | WSL 构建脚本背景与排障 |
| `docs/Tools/train_tub_driver.md` | Tub JSON 训练工具说明 |
| `docs/Tools/mus4_pilot_infer.md` | Pilot 推理工具说明 |
| `docs/README/DevNote.md` | 开发环境配置、串口协议、常量表 |
| `docs/README/OPERATIONS.md` | 串口运行时操作命令与数据帧 |
| `docs/Plan/` | 设计方案与实施路线；新增方案类内容写入此目录 |
| `docs/Algo/` | 算法逻辑说明 |
| `docs/Inspect/` | 问题排查记录 |
| `docs/Guide/` | 操作指南 |
| `docs/Valid/` | 验证指南 |
| `CHANGELOG.md` | 版本发布记录 |
| `CLAUDE.md` | 面向 Claude Code 的详细行为参考与编辑安全备忘 |

---

## 11. 自动构建策略

- 完成 `MUS4_FW.ino`、各模块 `.h`/`.cpp`、配置等固件源代码修改后，**自动调用** `mus4-wsl-build` Skill 进行 **WSL 编译并上传**。
- **必须首先使用 WSL 编译**（`.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`），只有 WSL 不可用时才回退到原生编译。
- OTA 目标优先读取项目根目录 `.mus4_ota_target` 第一行。
- **默认参数**：`-c -u -HttpOta -HttpOtaHost <目标地址>`（HTTP `/update` 端点，无 1KB/ACK 瓶颈）。
- 仅在设备固件不支持 HTTP `/update` 时回退到 `-Ota`。
- 不要自动执行 `git push`。
