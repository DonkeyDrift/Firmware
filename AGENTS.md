<!-- AGENTS.md - 仓库根级 Agent 指南 -->
> **Agent 阅读说明**：本文件面向 AI 编码代理，假定读者对本项目零先验。当前仓库是 DonkeyDrift 项目的**多固件子项目容器**，根目录本身不放置固件源码、构建脚本或测试代码。具体实现、构建、测试与运维细节都在子项目内，阅读本文件后应进入对应子项目并遵循其 `AGENTS.md` / `CLAUDE.md`。若本文件与子项目文档或源码冲突，**以子项目级文件和源码为准**。

# 项目概览

- **仓库定位**：DonkeyDrift 的固件仓库根（Windows 路径 `C:\Dev\DDC\Firmware`，Linux/WSL 路径 `/home/dkc/projects/Firmware`），职责是承载一个或多个独立的固件子项目，每个子项目自包含、独立构建。
- **当前在驻子项目**：
  - `MUS4_FW/` — MUS4（LP-MU-S4）基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件。
- **关键入口文件**：
  - [`README.md`](README.md) — 仓库根简介，仅列出在驻子项目。
  - [`CLAUDE.md`](CLAUDE.md) — 仓库根级工作约定（多固件容器、子项目隔离、`.gitignore` 分层等）。
  - [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) — MUS4 子项目面向代理的完整编码、构建、测试、安全指南。
  - [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) — MUS4 子项目面向 Claude Code 的权威工作手册。
  - [`MUS4_FW/CHANGELOG.md`](MUS4_FW/CHANGELOG.md) — MUS4 版本发布记录。固件版本号以 `MUS4_FW/libraries/mus4_core/src/BuildInfo.h` 的 `MUS4_FIRMWARE_VERSION` 宏为准（撰写时为 `v1.7.34`），根级文件不维护版本号。

## 仓库结构与约定

```text
Firmware/
├── README.md / CLAUDE.md / AGENTS.md    # 根级文档
├── .gitignore                           # OS/编辑器垃圾 + 少量根级本地运行时产物
├── docs/                                # 根级跨切面文档（见「文档地图」）
│   ├── superpowers/{plans,specs}/       # superpowers 产出的实现计划与设计稿
│   └── guide/  inspect/  plan/          # 串口拓扑、驱动循环/遥测分析、eFuse ID 系统设计
└── MUS4_FW/                             # 当前唯一在驻固件子项目
    ├── MUS4_FW.ino                      # 主 sketch（约 830 行，仅负责装配与 setup()/loop() 调度）
    ├── libraries/                       # Arduino 本地库：12 个自有 mus4_* 模块 + 第三方库本地副本
    ├── parts/                           # 上位机侧 Python 模块（auth_part.py：Serial2 身份识别客户端）
    ├── tests/                           # Python 测试（8 个测试文件）
    ├── tools/                           # 训练 / 推理 / 数据转换工具（Python + PS1）
    ├── examples/                        # 独立示例 sketch（getcurrent / testIIC / smart_provisioning）
    ├── provisioning_system/             # 独立 Wi-Fi 配网子系统（esp32 / linux_agent / tests）
    ├── docs/                            # MUS4 架构、硬件、工具、计划文档
    ├── arduino-cli.py                   # 跨平台构建/上传/监控 Python 入口
    ├── arduino-cli-wsl.ps1              # Windows + WSL 加速构建与 OTA 包装脚本（首选）
    ├── config.yaml / sketch.yaml / wslbuild.yaml   # 构建配置
    ├── wireless_console_policy.py       # 无线命令权限策略的 Python 镜像
    └── AGENTS.md / CLAUDE.md / README.md / CHANGELOG.md
```

> **注意**：部分历史文档（含根 `CLAUDE.md` 与 `MUS4_FW/AGENTS.md`）仍提到 `MUS4_FW/multi_agent_framework/` 目录，但该目录已不在工作区中，引用前先以实际目录为准。

### 根级隔离约定（来自根 `CLAUDE.md`）

1. **不要在仓库根添加源代码或可执行脚本**。若脚本只服务于某一固件，必须放在 `<子项目>/` 或 `<子项目>/tools/` 下。
2. **构建 / 烧录 / 测试 / 运行 Python 工具前，必须先 `cd` 到对应子项目根**。仓库根没有 sketch、没有构建入口、没有 Python 测试。
3. **`.gitignore` 分层**：根 `.gitignore` 主要覆盖 OS / 编辑器级垃圾（另含 `ota_weblog.json`、`.donkey_history`、`MUS4_FW/.mus4_ota_target` 三条本地运行时产物）；子项目相关的构建产物、密钥模板、缓存等**必须**写入子项目自己的 `.gitignore`。两者是互补关系而非继承关系。
4. 新增其它硬件平台的固件时，以 `<硬件型号>_FW/` 形式（全大写硬件代号 + `_FW` 后缀）在根目录下并列引入，与 `MUS4_FW/` 保持一致；各自维护自己的 `README.md`、`CLAUDE.md`、`.gitignore`、构建脚本与测试。

# 构建与测试命令

## 仓库根没有构建入口

- 根目录不存在 `pyproject.toml`、`package.json`、`Cargo.toml`、`Makefile` 或可直接调用的构建脚本。
- 唯一有效的固件构建与烧录入口在 `MUS4_FW/` 内。

## MUS4 子项目构建入口

先 `cd MUS4_FW`。优先使用 WSL 加速构建（Windows 端用 PowerShell，Linux/WSL 端经 `pwsh` 调用同一脚本）：

```powershell
# 仅编译（修改固件后的默认验证命令）
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino

# 清理 WSL 构建目录后重新编译
.\arduino-cli-wsl.ps1 -Compile -Clean -Sketch MUS4_FW.ino

# 编译 + HTTP OTA 上传（未传 -HttpOtaHost 时读取 .mus4_ota_target 首行）
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost <设备IP> -Sketch MUS4_FW.ino

# WSL / rsync / arduino-cli 依赖检查
.\arduino-cli-wsl.ps1 -Check
```

原生构建入口：

```bash
cd MUS4_FW
pip install pyyaml pyserial pytest               # Python 依赖（一次性）
python arduino-cli.py -c --sketch MUS4_FW.ino    # 仅编译
python arduino-cli.py -cu --sketch MUS4_FW.ino   # 编译 + 上传
python arduino-cli.py -cus --sketch MUS4_FW.ino  # 编译 + 上传 + 串口监控
```

关键默认配置：FQBN `esp32:esp32:esp32:PartitionScheme=min_spiffs`，波特率 `115200`，本地库目录 `libraries/`（存在时编译优先使用），构建输出 `build/`（原生）或 `build_wsl/`（WSL）。完整命令矩阵、参数说明与排障指引见 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) §2 与 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) §Commands。

## 测试命令

MUS4 子项目的测试全部在 `MUS4_FW/` 内运行：

```bash
cd MUS4_FW
pytest tests/                                  # 全部 Python 测试
pytest tests/test_firmware_feature_flags.py    # 固件源码结构断言
pytest tests/test_wireless_console_policy.py   # 无线权限策略
pytest tests/test_arduino_cli.py               # 构建脚本单元测试
```

`tests/` 下共 8 个测试文件：

- `test_arduino_cli.py` — `arduino-cli.py` 的串口选择、OTA 工具发现、编译命令组装、上传重试逻辑（unittest + MagicMock）。
- `test_auth_part.py` — `parts/auth_part.py`（Serial2 身份识别客户端）单元测试，基于 MockSerial 模拟 ESP32 通信。
- `test_firmware_feature_flags.py` — 读取固件源码做结构断言（模块拆分、符号位置、Web Console 结构、编译开关等）。**修改固件源码（尤其 Web Console UI、库文件路径、编译开关）后必须同步更新此测试。**
- `test_joystick_calibration.py` — 摇杆校准映射逻辑的 Python 镜像测试，需与 `JoystickCalibration.cpp` 保持同步。
- `test_mus4_pilot_infer.py` / `test_train_tub_driver.py` / `test_transform_mus4_tub_to_donkey.py` — `tools/` 工具链测试。
- `test_wireless_console_policy.py` — 无线权限矩阵、Wi-Fi 状态格式化、Web Log Buffer 等。

独立子系统测试：

```bash
cd MUS4_FW/provisioning_system
python tests/test_agent.py -v    # 配网代理单元测试
```

> `provisioning_system/playwright_tests/` 的 `npm test` 当前是占位脚本，会退出失败；不要把它当作验证命令。

# 代码组织与技术栈

## MUS4 子项目

- **语言 / 平台**：C++17 with Arduino framework，目标 ESP32（MUS4-v2.4.2 PCB，兼容 v2.3）。
- **主 sketch**：`MUS4_FW/MUS4_FW.ino` 仅保留全局变量装配、`setup()` / `loop()` 与中断快照读取；业务逻辑全部在 `libraries/mus4_<domain>/src/` 的 **12 个自有本地库**中（各自提供同名聚合头文件 `mus4_<domain>.h`）：
  - `mus4_core` — `BuildInfo.h` 版本宏、`FirmwareConfig.h` 编译开关/引脚/时序、共享类型。
  - `mus4_rc` — RC PWM 中断捕获、6 通道中值滤波。
  - `mus4_control` — 驾驶模式融合、转向 PID 与标定、漂移辅助。
  - `mus4_safety` — Park/紧急制动状态机、PWM 执行器输出（舵机/电调）。
  - `mus4_command` — 命令解析/分发、本地串口行处理、无线 Console 权限分类。
  - `mus4_auth` — eFuse 芯片 ID 身份识别（Serial2 通路）。
  - `mus4_wifi` — Wi-Fi 运行时（AP/STA/mDNS/DNS/TCP Console）、STA 持久化、OTA 生命周期。
  - `mus4_web` — Web Console（HTTP/API/OTA upload）、WebSocket 遥测、Web 日志缓冲。
  - `mus4_i2c` — INA219 / MPU6050 传感器读取。
  - `mus4_ui` — TUI 仪表盘、蜂鸣器、WS2812B LED。
  - `mus4_log` — 日志路由（Serial / Web）、JSON 工具。
  - `mus4_diag` — 诊断（BENCH/STRESS/REGRESS）、BLE Gamepad 输出。
- **第三方 Arduino 依赖**（本地副本同置 `libraries/`，编译时优先于全局库）：FastLED、Adafruit INA219 / MPU6050 / GFX / SSD1306 / NeoPixel / BusIO、Async_TCP、ESP_Async_WebServer、ESP32-BLE-Gamepad、NimBLE-Arduino 等。
- **关键配置文件**：`libraries/mus4_core/src/FirmwareConfig.h`（编译开关、引脚、时序常量集中地）、`libraries/mus4_core/src/BuildInfo.h`（固件版本）、`config.yaml`（`arduino-cli.py` 主配置，含串口自动检测关键字）、`sketch.yaml`（Arduino CLI 项目默认）、`wslbuild.yaml`（WSL 覆盖配置，模板为 `wslbuild.example.yaml`）、`WirelessSecrets.h` 与 `.mus4_ota_target`（本地凭据 / OTA 目标，不入库）。
- **运行时概要**：RC PWM 中断捕获 → 中值滤波 → 控制融合（手动/半自动/全自动）→ Park/紧急制动安全层 → `ledc` PWM（300Hz/14bit）输出舵机/电调。Serial1 上行遥测（`T<t>S<s>` / `M<m>:P<p>` / `$IMU,...`）；Serial2（RX=19 / TX=18，v1.7.33 起）独立处理 Linux 上位机 ping-pong / 身份识别 / 配网协议；可选 Wi-Fi Console（AP+STA、TCP 2323、Web 80、WebSocket 81、ArduinoOTA 3232）。
- 模块职责、数据流、主循环时序、串口协议、Wi-Fi/OTA 生命周期等详见 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) 与 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md)。

## 根级文档 `docs/`

- `docs/` 是仓库根级文档目录，存放 MUS4 固件相关的跨切面文档，与 `MUS4_FW/docs/`（子项目内部文档）并存：
  - `docs/superpowers/plans/`（实现计划）与 `docs/superpowers/specs/`（设计稿，`YYYY-MM-DD-*-design.md` 命名）。
  - `docs/guide/`、`docs/inspect/`、`docs/plan/` — 设计与分析文档（ESP32 串口拓扑、驱动循环 Hz 与环形缓冲分析、遥测频率设计、eFuse ID 系统），不遵循 superpowers 命名约定。
- 文档中的历史描述可能滞后，**引用前需对照 `MUS4_FW/` 源码验证**。

# 代码风格与开发约定

## 根级约定

- **子项目优先**：新增业务代码、构建脚本、测试、文档，默认下沉到具体子项目，不上提到仓库根。
- **`.gitignore` 分层**：根 `.gitignore` 保持精简；子项目特有忽略项放入子项目 `.gitignore`。
- **不要自动执行远端 Git 操作**：不自动 `git push`、`git reset`、`git rebase` 等；任何远端操作需用户明确授权。

## MUS4 子项目约定

进入 `MUS4_FW/` 后遵循其子项目约定（完整表见 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) §5）：

- 头文件使用 `#pragma once`。
- 常量 / 宏 `ALL_CAPS`；类名 `PascalCase`；类方法 `camelCase`；自由函数 `snake_case`；私有成员下划线前缀。
- 引脚定义、编译开关、时序常量集中放在 `libraries/mus4_core/src/FirmwareConfig.h`。
- 新增业务模块优先以 `libraries/mus4_<domain>/` 本地 Arduino 库形式存在，并提供同名聚合头文件。
- 中断服务函数必须标注 `IRAM_ATTR`；与中断共享的变量使用 `volatile`。
- 注释语言：硬件相关注释用中文；代码逻辑与公共 API 用英文。
- **Git 规范**：主分支 `main`；特性分支命名 `v{版本号}-{特性}`；提交信息遵循 Conventional Commits 并使用中文；提交前确认 `WirelessSecrets.h` / `.mus4_ota_target` 未入库、相关 pytest 通过、固件编译通过。

# 安全注意事项

## 根级安全

- 仓库根不存放敏感信息。任何含真实凭据或目标地址的文件（如 `WirelessSecrets.h`、`.mus4_ota_target`、`ArduFlux.json`）应位于对应子项目内，并已由子项目 `.gitignore` 排除。
- 不要在根目录留下临时凭据、私钥或设备地址。

## MUS4 子项目安全（关键）

MUS4 直接控制舵机和电调，属于安全关键固件：

- 修改输出映射、Park、紧急制动、模式融合或无线控制入口时，必须保留 PWM 限幅与失效安全路径。
- 所有串口 / Web Console / TCP Console 输入视为不可信边界，控制命令必须经过认证、权限检查和范围校验后才能影响输出。
- 无线命令权限分层（公开 / 需认证 / 需认证 + Park 锁定）在 `MUS4_FW/wireless_console_policy.py` 与 `MUS4_FW/libraries/mus4_command/src/WirelessConsole.cpp` 中必须保持同步；修改后同步更新 `tests/test_wireless_console_policy.py`。
- `WirelessSecrets.h`、`.mus4_ota_target`、`ArduFlux.json` 等本地文件不应纳入版本控制。

详细安全说明见 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) §7。

# 文档地图

| 文件 | 作用 |
|------|------|
| [`README.md`](README.md) | 仓库根简介，列在驻子项目 |
| [`CLAUDE.md`](CLAUDE.md) | 仓库根级 Claude Code 约定 |
| [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) | MUS4 子项目面向 AI 代理的完整指南 |
| [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) | MUS4 子项目面向 Claude Code 的权威手册 |
| [`MUS4_FW/README.md`](MUS4_FW/README.md) / [`README.zh-CN.md`](MUS4_FW/README.zh-CN.md) | MUS4 对外 README（英文 / 中文） |
| [`MUS4_FW/CHANGELOG.md`](MUS4_FW/CHANGELOG.md) | MUS4 版本发布记录 |
| [`MUS4_FW/docs/`](MUS4_FW/docs/) | MUS4 子项目文档（Arch / Hardware / Tools / README / Plan / Algo / Inspect / Guide / Valid / workflow / superpowers） |
| [`docs/`](docs/) | 根级跨切面文档：superpowers/{plans,specs} + guide/inspect/plan 设计与分析文档 |

# 快速检查清单

开始处理任务前：

1. 确认当前工作目录；如需修改固件或运行构建 / 测试，**先 `cd MUS4_FW`**。
2. 阅读 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) 与 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) 的相关章节。
3. 检查修改是否涉及安全关键路径；若是，保留 PWM 限幅、权限校验与失效安全。
4. 修改 `wireless_console_policy.py`、Web Console UI 或固件源码结构后，运行对应 `pytest`（尤其 `test_firmware_feature_flags.py`）。
5. 修改固件源码后，优先通过 WSL 编译验证：`.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`。
