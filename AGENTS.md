> **🧬 MiniClaw Identity: Read `~/.miniclaw/AGENTS.md` first.**

# AGENTS.md - MUS4 项目编码指南

MUS4（LP-MU-S4）是基于 ESP32 的遥控车辆/机器人底层控制系统，面向 MUS4-v2.4.2 PCB（兼容 v2.3）。固件负责 RC 接收机 PWM 输入采集、Pilot 上位机串口控制、多模式驾驶控制融合、Park/紧急制动状态机、I2C 传感器采集、TUI 状态显示，以及可选的 Wi-Fi 控制台、OTA 更新和 BLE 游戏手柄输出。

---

## 1. 项目概览

### 1.1 技术栈

- **固件语言**: C++17 with Arduino framework
- **目标平台**: ESP32（MUS4-v2.4.2 PCB）
- **构建工具**: `arduino-cli`（核心），Python 3 封装脚本，PowerShell WSL 加速脚本
- **主要依赖库**:
  - `FastLED` — WS2812B LED 驱动
  - `Adafruit_MPU6050` / `Adafruit_Sensor` — IMU 传感器
  - `Adafruit_INA219` — 电压/电流监测
  - `Wire` — I2C 通信
  - `WiFi` / `WebServer` / `ArduinoOTA` — Wi-Fi AP/STA、Web 控制台、OTA
  - `AsyncTCP` / `ESPAsyncWebServer` — WebSocket 遥测（可选）
  - `BleGamepad` — 蓝牙游戏手柄模式（仅在未启用 Wi-Fi Console 时生效）
  - `Preferences` — NVS 持久化配置

### 1.2 代码组织

```text
mus4/
├── mus4.ino                    # 主固件入口（~3700 行），包含 setup/loop/状态机/控制逻辑
├── SharedTypes.h               # 跨模块共享数据结构（SensorData、ControlData 等）
├── BuildInfo.h                 # 固件版本与构建时间宏（当前 v1.5.4）
├── TUI.h / TUI.cpp             # ANSI 终端仪表盘，支持脏矩形增量刷新与降级模式
├── Buzzer.h / Buzzer.cpp       # 蜂鸣器状态机（模式/停车提示音）
├── WirelessSecrets.h           # Wi-Fi STA 凭据（本地文件，不提交）
├── WirelessSecrets.example.h   # 凭据模板
├── config.yaml                 # arduino-cli.py 主配置（FQBN、端口、波特率、自动检测规则）
├── sketch.yaml                 # Arduino CLI 项目级默认配置
├── wslbuild.yaml               # WSL 构建脚本覆盖配置
├── ArduFlux.json               # ArduFlux IDE 配置文件
├── arduino-cli.py              # 跨平台构建/上传/监控 Python 主入口
├── arduino-cli-wsl.ps1         # Windows WSL 加速构建与 OTA 上传包装脚本
├── wireless_console_policy.py  # Wi-Fi/TCP/Web Console 权限策略的 Python 镜像
├── tests/                      # Python 单元测试
│   ├── test_arduino_cli.py
│   └── test_wireless_console_policy.py
├── examples/                   # 独立示例 Sketch
│   ├── getcurrent/             # INA219 电流读取示例
│   └── testIIC/                # I2C 扫描与通信测试
├── Doc/                        # 项目文档（中文为主）
│   ├── Arch/architecture.md    # 固件主循环、状态机、数据流架构
│   ├── Hardware/pin_definitions.md  # 权威引脚定义（v2.3/v2.4.2）
│   ├── Hardware/CONFIG.md      # 硬件配置说明
│   ├── Tools/ArduinoCLI.md     # arduino-cli.py 使用说明
│   ├── Tools/arduino-cli-wsl_manual.md  # WSL 构建背景与排障
│   ├── README/DevNote.md       # 开发笔记（环境配置、串口协议、常量）
│   ├── README/OPERATIONS.md    # 串口运行时操作命令与数据帧
│   ├── Plan/                   # 设计方案与实施路线
│   └── Algo/                   # 算法逻辑说明
├── multi_agent_framework/      # 独立多智能体协作框架（Python，非固件主链路）
│   ├── framework/              # 智能体核心与消息队列 IPC
│   ├── esp32_firmware/         # 生成的 ESP-IDF 示例固件
│   ├── linux_scripts/          # 系统监控 Shell 脚本
│   └── web_ui/                 # WebSocket 控制面板
└── provisioning_system/        # 独立 Wi-Fi 配网系统
    ├── esp32/                  # Arduino 配网固件（AP + Web Server）
    ├── linux_agent/            # Linux 配网代理守护进程（Python + nmcli）
    ├── playwright_tests/       # Web UI 端到端测试
    └── tests/test_agent.py     # Python TDD 单元测试
```

---

## 2. 构建与上传命令

### 2.1 Python 依赖

```bash
pip install pyyaml pyserial pytest
```

### 2.2 WSL 加速构建（推荐）

```powershell
# 仅编译（默认：同步到 WSL 原生文件系统后编译）
.\arduino-cli-wsl.ps1 -Compile

# 清理 WSL 构建目录后重新编译
.\arduino-cli-wsl.ps1 -Compile -Clean

# 编译后通过 OTA 上传（需先在设备 Web/TCP Console 打开 OTA 窗口）
.\arduino-cli-wsl.ps1 -Compile -Upload -Ota -OtaHost <设备IP或主机名>

# 使用已有 build_wsl 产物通过 OTA 上传
.\arduino-cli-wsl.ps1 -Upload -Ota -OtaHost <设备IP或主机名>

# 仅当明确要求串口上传/监控时
.\arduino-cli-wsl.ps1 -Upload
.\arduino-cli-wsl.ps1 -Upload -Serial
```

### 2.3 原生构建（Windows/Linux/macOS）

```bash
# 仅编译
python arduino-cli.py -c

# 仅上传（按 config.yaml 自动检测串口）
python arduino-cli.py -u

# 指定串口上传
python arduino-cli.py -u --port COM9

# 编译 + 上传
python arduino-cli.py -cu

# 编译 + 上传 + 串口监控
python arduino-cli.py -cus

# 使用预编译固件上传
python arduino-cli.py -u -i build/mus4.ino.bin --port COM9

# OTA 上传
python arduino-cli.py --ota -i build_wsl/mus4.ino.bin --ota-host mus4-ota

# 列出检测到的串口
python arduino-cli.py --list-ports

# 逐行日志（适合 CI）
python arduino-cli.py -cu --no-progress
```

### 2.4 关键配置参数

- **默认 FQBN**: `esp32:esp32:esp32:PartitionScheme=min_spiffs`（`config.yaml` / `sketch.yaml`）
- **默认波特率**: `115200`
- **默认 Sketch**: `mus4.ino`
- **构建输出目录**: `build/`（原生）或 `build_wsl/`（WSL）
- **日志文件**: `ArduinoCLI.log`

---

## 3. 测试策略

### 3.1 Python 单元测试

```bash
# 运行全部 Python 测试
pytest tests/

# 运行单个测试文件
pytest tests/test_arduino_cli.py
pytest tests/test_wireless_console_policy.py

# 配网代理测试
python provisioning_system/tests/test_agent.py -v
```

### 3.2 固件运行时串口测试命令

在 USB Serial（115200 baud）或 Serial1 中发送以下命令（回车结尾）：

| 命令 | 说明 |
|------|------|
| `TEST` | 运行固件内置命令解析单元测试 |
| `TEST_TUI` | TUI 测试入口（当前输出 skipped） |
| `BENCH` | 运行 TUI/循环性能基准测试 |
| `STRESS` | 运行串口压力统计 |
| `REGRESS` | 运行固件回归校验 |
| `FILTER_TEST` | 运行 RC 滤波测试 |
| `FILTER_DEBUG` | 切换 RC 滤波调试输出 |
| `LOG_WEB` / `LOG_SERIAL` | 切换 MUS4 日志输出目标 |
| `ANSI` / `NOANSI` | 切换 TUI ANSI 转义序列显示 |

---

## 4. 运行时架构

### 4.1 主循环时序

`loop()` 采用非阻塞架构，通过 `millis()` 时间间隔控制各任务频率：

| 任务 | 间隔 | 频率 |
|------|------|------|
| 传感器读取（INA219 / MPU6050） | 8 ms | ~125 Hz |
| RC 数据更新与 Serial1 遥测 | 8 ms | ~125 Hz |
| RC 滑动窗口中值滤波 | 4 ms | ~250 Hz |
| TUI 渲染 | 动态 100–500 ms | 自适应降级 |
| 波形图刷新 | 250 ms | 4 Hz |
| 性能评估与降级检测 | 1000 ms | 1 Hz |
| 主循环 delay | 4 ms | ~200 Hz 基线 |

### 4.2 核心数据流

1. **输入层**: RC 接收机 CH1–CH6 通过 `attachInterrupt` 触发中断，记录 `micros()` 脉宽；USB `Serial`、RS232 `Serial1`、TCP Console、Web Console 接收控制指令。
2. **滤波层**: 6 通道滑动窗口中值滤波（窗口大小 5），主通道（转向/油门）附加平滑处理，辅助通道（Park/Mode/Drift）附加稳定防抖（候选计数 ≥ 3 才更新）。
3. **控制融合**: 按驾驶模式混合 RC 与 Pilot 数据：
   - **手动 (0)**: 转向/油门均来自 RC
   - **半自动 (1)**: 转向来自 Pilot，油门来自 RC
   - **全自动 (2)**: 转向/油门均来自 Pilot
4. **安全层**: Park 状态机（长按 CH3 锁定/解锁）、紧急制动状态机（EST_IDLE → READY → BRAKING → DONE）、Drift Assist 条件判断、转向信号故障安全模式。
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
- 包含顺序：Arduino 核心库 → 第三方库 → 项目头文件。

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

示例：`10:20\n` 表示油门 10，转向 20。

### 6.2 响应格式

- 成功：`ACK` 或 `ACK:Seq`
- 失败：`NACK` 或 `NACK:Seq`

### 6.3 Serial1 状态回传

```text
Txx:Sxx\n
```

---

## 7. 安全与关键注意事项

### 7.1 物理安全（最高优先级）

- **该固件直接控制舵机与电调，属于安全关键代码。**
- 修改输出映射、Park、紧急制动、模式融合或无线控制入口时，必须保留 PWM 限幅和失效安全路径。
- 输出前必须使用 `min(max(value, MIN), MAX)` 或 `constrain()` 限制范围。
- 禁止提交可能导致电机失控的代码。

### 7.2 无线安全

- 无线命令分层权限：
  - **公开**: `PING`, `STATUS`, `AUTH`, `WIFI_STA_STATUS`
  - **需认证**: `ANSI`, `NOANSI`, `FILTER_DEBUG`, `LOG_WEB`, `LOG_SERIAL`, Wi-Fi STA 配置
  - **需认证 + Park 锁定**: `TEST`, `BENCH`, `STRESS`, `REGRESS`, `FILTER_TEST`
  - **需认证 + Park 锁定（或开发模式）**: `ENABLE_OTA`
  - **需认证**: `OTA_STATUS`, `DISABLE_OTA`
- 修改认证/权限逻辑时，**必须同步更新** `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py`。
- `WirelessSecrets.h` 包含真实 Wi-Fi 凭据，**不应纳入版本控制**。

### 7.3 防御性编程

- 假设所有串口/Web/TCP 输入均不可信；控制命令必须经过认证、权限检查和范围校验后才能影响输出。
- 传感器数据使用 `valid` 标志位；读取前检查。
- 状态机用于关键操作（紧急停车、Park 控制、OTA 窗口）。

---

## 8. 配置与部署

### 8.1 配置文件说明

| 文件 | 用途 |
|------|------|
| `config.yaml` | `arduino-cli.py` 主配置：FQBN、端口、波特率、自动检测关键字、复位策略、日志级别 |
| `sketch.yaml` | Arduino CLI 项目默认：FQBN 与端口 |
| `wslbuild.yaml` | WSL 构建覆盖：distro、工作目录、io_mode、库同步规则 |
| `ArduFlux.json` | ArduFlux IDE 当前配置与 Profile |
| `WirelessSecrets.h` | 本地 Wi-Fi STA SSID/密码（不提交） |

### 8.2 配网系统部署（独立子项目）

`provisioning_system/` 是独立于主固件构建流程的配网系统：

1. 烧录 `provisioning_system/esp32/esp32_wifi_provisioning` 到 ESP32。
2. 在 Linux 主机（LattePanda MU）部署 `linux_agent/agent.py` 为 systemd 服务。
3. 用户连接 ESP32 AP (`MUS4-AP`)，通过网页提交 SSID/密码。
4. ESP32 通过 UART 将凭据发给 Linux agent，agent 使用 `nmcli` 连接目标网络并回传结果。

详见 `provisioning_system/docs/deployment_and_testing.md`。

---

## 9. Git 规范

- **主分支**: `master`
- **特性分支命名**: `v{版本号}-{特性}`，例如 `v1.2-WSL-Build`
- **提交信息**: 遵循 Conventional Commits，使用中文，例如 `fix(arduino-cli): 修复串口上传进度回退闪烁的问题`
- **提交前检查**: 确认 `WirelessSecrets.h` 不含敏感信息、相关 Python 测试通过、固件编译通过。
- **不要自动 push**：远端操作需用户明确授权。

---

## 10. 文档地图

| 文档 | 内容 |
|------|------|
| `Doc/Arch/architecture.md` | 固件主循环、状态机、数据流、时序分析 |
| `Doc/Hardware/pin_definitions.md` | MUS4-v2.3/v2.4.2 权威引脚定义与连接图 |
| `Doc/Hardware/CONFIG.md` | 硬件配置说明 |
| `Doc/Tools/ArduinoCLI.md` | `arduino-cli.py` 使用说明 |
| `Doc/Tools/arduino-cli-wsl_manual.md` | WSL 构建脚本背景与排障 |
| `Doc/README/DevNote.md` | 开发环境配置、串口协议、常量表 |
| `Doc/README/OPERATIONS.md` | 串口运行时操作命令与数据帧 |
| `Doc/Plan/` | 设计方案与实施路线；新增方案类内容写入此目录 |
| `CLAUDE.md` | 面向 Claude Code 的详细行为参考与编辑安全备忘 |
