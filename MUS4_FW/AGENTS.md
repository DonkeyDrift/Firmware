<!-- AGENTS.md - MUS4 项目编码指南 -->
> **Agent 阅读说明**：本文件面向 AI 编码代理。项目的主要人类文档是 `README.md`，而本文件补充构建、测试、代码风格与安全等代理需要快速掌握的约定。若本文件与源码冲突，**以源码和 `CHANGELOG.md` 为准**。

# MUS4（LP-MU-S4）项目编码指南

MUS4 是基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件。当前主 Sketch 为根目录 `MUS4_FW.ino`，固件版本定义于 `libraries/mus4_core/src/BuildInfo.h` 的 `MUS4_FIRMWARE_VERSION` 宏（当前为 `v1.7.33`），目标硬件为 MUS4-v2.4.2 PCB（兼容 v2.3）。固件负责 RC 接收机 PWM 输入采集、Pilot 上位机串口控制、多模式驾驶控制融合、Park/紧急制动状态机、I2C 传感器采集、TUI 状态显示，以及可选的 Wi-Fi 控制台、OTA 更新和 BLE 游戏手柄输出。

> **重要结构变更**：自 2026-06 起，原根目录下成对的 `.h/.cpp` 业务模块已按功能域抽取为本地 Arduino 库，位于 `libraries/mus4_*/src/`。主 Sketch 仅保留全局变量装配、`setup()` / `loop()` 与 Wi-Fi 运行时状态别名。v1.7.33 起新增 Serial2 双向通道（RX=19, TX=18）独立处理 Linux 上位机 ping-pong/身份识别/配网协议。任何新增业务模块都应优先考虑以 `libraries/mus4_<domain>/` 本地库形式存在，而不是放回根目录。

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
| `libraries/mus4_core/src/BuildInfo.h` | 固件版本宏 | `MUS4_FIRMWARE_VERSION`（当前 `v1.7.33`），构建日期/时间由编译器 `__DATE__` / `__TIME__` 注入 |
| `libraries/mus4_core/src/FirmwareConfig.h` | 编译期功能开关、引脚定义、时序/滤波/日志目标 | 所有 `.h/.cpp` 均优先包含此文件 |
| `config.yaml` | `arduino-cli.py` 主配置 | FQBN、端口、波特率、串口自动检测关键字、复位策略、日志级别 |
| `sketch.yaml` | Arduino CLI 项目级默认配置 | FQBN 与端口 |
| `wslbuild.yaml` | WSL 构建脚本覆盖配置 | distro（当前 `DKC`）、工作目录 `/home/dkc/arduino-build/MUS4_FW`、`io_mode`、库同步规则 |
| `wslbuild.example.yaml` | WSL 配置模板 | 复制为 `wslbuild.yaml` 后按需修改 |
| `ArduFlux.json` | ArduFlux IDE 配置文件 | 当前板型、端口、Monitor 参数（不建议纳入版本控制，见 `.gitignore`） |
| `WirelessSecrets.h` | 本地 Wi-Fi STA SSID/密码 | **不提交**，由 `.gitignore` 排除 |
| `libraries/mus4_core/src/WirelessSecrets.example.h` | Wi-Fi STA 凭据模板 | 复制到根目录并改名为 `WirelessSecrets.h` 后填写真实凭据 |
| `.mus4_ota_target` | HTTP OTA 默认目标主机首行 | **不提交**，由 `.gitignore` 排除 |

### 1.3 代码组织

根目录是多文件 Arduino 项目，主 Sketch `MUS4_FW.ino` 仅保留 `setup()` / `loop()`、全局变量装配和中断快照读取。所有业务逻辑已按功能域抽取为本地 Arduino 库，每个库在 `libraries/mus4_<domain>/src/` 下提供成对 `.h/.cpp`，并通过同名聚合头文件 `mus4_<domain>.h` 统一导出。

```text
根目录/
├── MUS4_FW.ino                    # 主固件入口：setup/loop/全局装配/混控
├── libraries/                     # 本地 Arduino 库（构建时优先使用）
│   ├── mus4_core/                 # 核心类型与配置
│   │   └── src/
│   │       ├── BuildInfo.h        # 固件版本与构建时间宏（当前 v1.7.33）
│   │       ├── FirmwareConfig.h   # 编译期功能开关、引脚定义、时序/滤波/日志目标
│   │       ├── SharedTypes.h      # SensorData / ControlData
│   │       ├── RuntimeState.h     # WifiRuntimeState / OtaRuntimeState
│   │       ├── WifiConsoleTypes.h # Wi-Fi 控制台常量、WebLogEntry/WebDataPoint/WifiScanEntry
│   │       ├── SerialBufferTypes.h# SerialBuf 结构体
│   │       ├── StringPrint.h      # 基于 String 的 Print 实现
│   │       ├── WirelessSecrets.example.h  # STA 凭据模板
│   │       └── mus4_core.h        # 聚合头文件
│   ├── mus4_ui/                   # 用户界面与指示
│   │   └── src/
│   │       ├── TUI.h/.cpp         # ANSI 终端仪表盘
│   │       ├── Buzzer.h/.cpp      # 蜂鸣器状态机
│   │       ├── LedStatus.h/.cpp   # WS2812B LED
│   │       └── mus4_ui.h
│   ├── mus4_log/                  # 日志与 JSON 工具
│   │   └── src/
│   │       ├── Mus4Log.h/.cpp     # 日志路由（Serial / Web）
│   │       ├── JsonUtil.h/.cpp    # JSON 字符串转义辅助
│   │       └── mus4_log.h
│   ├── mus4_i2c/                  # I2C 传感器
│   │   └── src/
│   │       ├── I2CBusTools.h/.cpp # I2C 底层读写与设备探测
│   │       ├── Sensors.h/.cpp     # INA219 / MPU6050 读取与扫描
│   │       └── mus4_i2c.h
│   ├── mus4_rc/                   # RC PWM 输入
│   │   └── src/
│   │       ├── RcPwmCapture.h/.cpp# RC PWM 输入捕获：中断、脉冲验证
│   │       ├── RcFilter.h/.cpp    # 6 通道滑动窗口中值滤波与防抖
│   │       └── mus4_rc.h
│   ├── mus4_control/              # 控制融合与转向
│   │   └── src/
│   │       ├── ControlMixer.h/.cpp# 驾驶模式切换、RC/Pilot 控制融合
│   │       ├── DriftAssist.h/.cpp # 漂移辅助
│   │       ├── SteeringControl.h/.cpp # 转向 PID 平滑与故障安全
│   │       ├── SteeringCalibration.h/.cpp # 转向通道交互式标定
│   │       └── mus4_control.h
│   ├── mus4_safety/               # 安全与执行器输出
│   │   └── src/
│   │       ├── SafetyState.h/.cpp # Park 状态机、紧急制动 FSM
│   │       ├── ActuatorOutput.h/.cpp # PWM 执行器输出：舵机/电调映射与驱动
│   │       └── mus4_safety.h
│   ├── mus4_command/              # 命令处理
│   │   └── src/
│   │       ├── CommandParser.h/.cpp     # 串口命令解析、校验和
│   │       ├── CommandDispatcher.h/.cpp # 命令分发（本地/无线/OTA/Wi-Fi STA）
│   │       ├── LocalCommands.h/.cpp     # 本地串口行处理（Pilot 数据帧解析）
│   │       ├── SerialLineReader.h/.cpp  # Serial/Serial1 行缓冲读取
│   │       ├── WirelessConsole.h/.cpp   # 无线命令权限与命令分类
│   │       └── mus4_command.h
│   ├── mus4_auth/                 # 身份识别（Serial2 通路）
│   │   └── src/
│   │       ├── AuthService.h/.cpp       # eFuse 芯片 ID 身份识别（CMD:READ_HW_ID/UID/WRITE_UID/CLEAR_UID）
│   │       └── mus4_auth.h
│   ├── mus4_wifi/                 # Wi-Fi 运行时、STA、OTA、身份
│   │   └── src/
│   │       ├── WifiManager.h/.cpp  # Wi-Fi runtime 状态机（AP/STA/mDNS/DNS/TCP Console）
│   │       ├── WifiStaConfig.h/.cpp# STA 配置持久化与状态管理
│   │       ├── WifiIdentity.h/.cpp # AP SSID / mDNS 主机名校验
│   │       ├── WifiOta.h/.cpp      # OTA 窗口、ArduinoOTA 生命周期、Park 保护
│   │       └── mus4_wifi.h
│   ├── mus4_web/                  # Web 控制台与遥测
│   │   └── src/
│   │       ├── WebConsoleAssets.h  # Web Console HTML/JS/CSS（PROGMEM）
│   │       ├── WebConsoleServer.h/.cpp  # HTTP route、API handler、OTA upload
│   │       ├── WebLogBuffer.h/.cpp # Web 日志 ring buffer
│   │       ├── WebTelemetry.h/.cpp # WebSocket 遥测、数据采样与推送
│   │       └── mus4_web.h
│   ├── mus4_diag/                 # 诊断与 BLE 游戏手柄
│   │   └── src/
│   │       ├── Diagnostics.h/.cpp  # 降级检测、BENCH/STRESS/REGRESS 诊断入口
│   │       ├── GamepadMode.h/.cpp  # BLE 游戏手柄输出
│   │       └── mus4_diag.h
│   └── ... 第三方库（FastLED、Adafruit_xxx、AsyncTCP、ESP_Async_WebServer 等）
├── config.yaml                    # arduino-cli.py 主配置
├── sketch.yaml                    # Arduino CLI 项目级默认配置
├── wslbuild.yaml                  # WSL 构建脚本覆盖配置
├── wslbuild.example.yaml          # WSL 配置模板
├── ArduFlux.json                  # ArduFlux IDE 配置文件
├── .mus4_ota_target               # HTTP OTA 默认目标主机（本地文件，不提交）
├── WirelessSecrets.h              # 本地 Wi-Fi STA 凭据（本地文件，不提交）
├── arduino-cli.py                 # 跨平台构建/上传/监控 Python 主入口
├── arduino-cli-wsl.ps1            # Windows WSL 加速构建与 OTA 上传包装脚本
├── build_wsl.ps1                  # WSL 构建辅助脚本
├── wireless_console_policy.py     # Wi-Fi/TCP/Web Console 权限策略的 Python 镜像
├── tests/                         # Python 单元/集成测试
│   ├── test_arduino_cli.py        # 串口选择、OTA 工具链、编译命令的单元测试
│   ├── test_firmware_feature_flags.py  # 源码结构断言（82 个 test 函数）
│   ├── test_train_tub_driver.py   # Tub 训练工具测试
│   ├── test_mus4_pilot_infer.py   # Pilot 推理控制器测试
│   └── test_wireless_console_policy.py # 无线权限策略、Web Log Buffer、网络状态格式化测试
├── tools/                         # 模型与数据采集工具
│   ├── train_tub_driver.py        # Tub JSON 行为克隆训练工具
│   └── mus4_pilot_infer.py        # Pilot 推理控制器
├── examples/                      # 独立示例 Sketch
│   ├── getcurrent/                # INA219 电流读取示例
│   ├── testIIC/                   # I2C 扫描与通信测试
│   └── smart_provisioning/        # Wi-Fi 配网示例（AP + Web Server）
├── docs/                          # 项目文档（中文为主）
│   ├── Arch/architecture.md       # 固件主循环、状态机、数据流架构（部分参数以 FirmwareConfig.h 为准）
│   ├── Hardware/pin_definitions.md# 引脚定义（v2.3/v2.4.2，部分 PWM 频率描述待更新）
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
│   ├── Valid/                     # 验证指南
│   ├── workflow/                  # 工作流与并行开发说明
│   └── superpowers/specs/         # 扩展规范（按需查阅）
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

> **完整命令矩阵、参数说明、HTTP OTA 稳定性机制、WSL 排障指引见 [`CLAUDE.md`](CLAUDE.md) §Commands。** 以下仅列最常用入口。

### Python 依赖

```bash
pip install pyyaml pyserial pytest
```

### 常用命令速查

```powershell
# WSL 仅编译（修改固件后首选验证）
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino

# WSL 编译 + HTTP OTA 上传
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost <设备IP> -Sketch MUS4_FW.ino

# 原生编译
python arduino-cli.py -c --sketch MUS4_FW.ino
```

关键配置：默认 FQBN `esp32:esp32:esp32:PartitionScheme=min_spiffs`，波特率 `115200`，构建输出 `build/`（原生）或 `build_wsl/`（WSL），本地库目录 `libraries/`。

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
- `test_firmware_feature_flags.py`：基于 `pytest`，包含 **82 个 test 函数**。它读取固件源码并验证：模块是否正确拆分、符号是否存在于预期文件、Web Console HTML/JS/CSS 结构、Wi-Fi 状态机行为、编译开关状态等。**修改固件源码（尤其是 Web Console UI 或库文件路径）后必须同步更新此测试并确保通过。** 当前该文件对 Web Console CSS 结构有较强的断言，若仅修改 UI 样式而未同步测试，会导致断言失败。
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

> **主循环时序、数据流、模块职责、中断约束详见 [`CLAUDE.md`](CLAUDE.md) §High-Level Architecture。** 以下仅列关键数据流要点。

核心数据流：RC PWM 中断捕获 → 中值滤波 → 控制融合（手动/半自动/全自动）→ Park/紧急制动安全层 → `ledc` PWM 输出（300Hz/14bit）。Serial1 上行 `T<t>S<s>` / `M<m>:P<p>` / `$IMU,...`，Serial2 处理 Linux 上位机 ping-pong/身份识别/配网协议。OTA 传输期间 Serial1 遥测暂停、WebSocket 清场。

---

## 5. 代码风格规范

> **完整命名约定、文件组织、注释语言、中断与安全编码规范见 [`CLAUDE.md`](CLAUDE.md) §C++ 编码规范。**

核心要点：头文件 `#pragma once`；常量 `ALL_CAPS`；类名 `PascalCase`；类方法 `camelCase`；自由函数 `snake_case`；私有成员下划线前缀；引脚/编译开关集中在 `FirmwareConfig.h`；新增模块以 `libraries/mus4_<domain>/` 本地库形式存在；硬件注释用中文，代码逻辑用英文。

## 6. 串口协议

> **下行/上行格式、校验和、ACK/NACK、Serial1 上行帧（T..S../M:P/$IMU）、Serial2 ping-pong/配网/身份识别协议详见 [`CLAUDE.md`](CLAUDE.md) §串口协议。**

## 7. 安全与关键注意事项

> **物理安全、无线权限分层、防御性编程详见 [`CLAUDE.md`](CLAUDE.md) §Safety-Critical Editing Notes 与 §Wi-Fi Console、Web Console 与 OTA。**

核心要点：该固件直接控制舵机与电调——修改输出/安全/模式融合时必须保留 PWM 限幅与失效安全；所有串口/Web/TCP 输入视为不可信边界；无线命令权限分层（公开/需认证/需认证+Park锁定）在 `wireless_console_policy.py` 与 `WirelessConsole.cpp` 中必须同步。

## 8. 配置与部署

> **编译开关、库优先级、配网系统部署详见 [`CLAUDE.md`](CLAUDE.md) §Configuration 与 §Firmware Behavior Reference。**

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
  - `build/`, `build_wsl/`, `build_verbose/`
  - `.arduino_cli_state.json`, `.tmp_serial_state_test.json`
  - `*.log`
  - `.cache/`, `.worktrees/`

---

## 10. 文档地图

| 文档 | 内容 |
|------|------|
| `docs/Arch/architecture.md` | 固件主循环、状态机、数据流架构（注意：部分时序参数基于历史版本，以 `FirmwareConfig.h` 为准） |
| `docs/Hardware/pin_definitions.md` | MUS4-v2.3/v2.4.2 引脚定义与连接图（注意：部分 PWM 输出频率描述为 50Hz，实际源码为 300Hz/14bit，以源码为准） |
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
| `docs/workflow/` | 工作流与并行开发说明（git worktree 等） |
| `docs/superpowers/specs/` | 扩展规范（按需查阅） |
| `CHANGELOG.md` | 版本发布记录 |
| `CLAUDE.md` | 面向 Claude Code 的详细行为参考与编辑安全备忘 |

---

## 11. 自动构建策略

- 完成固件源代码修改后，**自动调用** `mus4-fw-ota` Skill 进行 **WSL 编译并上传**。
- **必须首先使用 WSL 编译**（`.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`），只有 WSL 不可用时才回退到原生编译。
- OTA 目标优先读取项目根目录 `.mus4_ota_target` 第一行。
- **默认参数**：`-c -u -HttpOta -HttpOtaHost <目标地址>`（HTTP `/update` 端点，无 1KB/ACK 瓶颈）。
- 仅在设备固件不支持 HTTP `/update` 时回退到 `-Ota`。
- 不要自动执行 `git push`。
