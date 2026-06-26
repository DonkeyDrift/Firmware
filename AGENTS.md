<!-- AGENTS.md - 仓库根级 Agent 指南 -->
> **Agent 阅读说明**：本文件面向 AI 编码代理。当前仓库是 DonkeyDrift 项目的**多固件子项目容器**，根目录本身不放置固件源码、构建脚本或测试代码。具体实现、构建、测试与运维细节都在子项目内，阅读本文件后应进入对应子项目并遵循其 `AGENTS.md` / `CLAUDE.md`。

# 项目概览

- **仓库定位**：`C:\Dev\DDC\Firmware` 是 DonkeyDrift 的固件仓库根，职责是承载一个或多个独立的固件子项目。
- **当前在驻子项目**：
  - `MUS4_FW/` — MUS4（LP-MU-S4）基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件。
- **关键入口文件**：
  - [`README.md`](README.md) — 仓库根简介，仅列出在驻子项目。
  - [`CLAUDE.md`](CLAUDE.md) — 仓库根级工作约定（多固件容器、子项目隔离、`.gitignore` 分层等）。
  - [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) — MUS4 子项目面向代理的完整编码、构建、测试、安全指南。
  - [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) — MUS4 子项目面向 Claude Code 的权威工作手册。

> 若本文件与 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md)、[`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) 或源码冲突，**以子项目级文件和源码为准**。

## 仓库结构与约定

```text
C:/Dev/DDC/Firmware/
├── README.md                 # 仓库根 README
├── CLAUDE.md                 # 仓库根级 Agent/Claude 约定
├── AGENTS.md                 # 本文件
├── .gitignore                # 仅覆盖 OS / 编辑器级垃圾（见下文）
├── docs/superpowers/         # 根级唯一文档目录：实现计划与设计规格
└── MUS4_FW/                  # 当前唯一在驻固件子项目
    ├── MUS4_FW.ino           # MUS4 主 sketch
    ├── libraries/            # Arduino 本地库（含 mus4_* 自有模块与第三方库）
    ├── tests/                # Python 测试
    ├── tools/                # 训练 / 推理 / 辅助工具
    ├── examples/             # 独立示例 sketch
    ├── provisioning_system/  # 独立 Wi-Fi 配网子系统
    ├── multi_agent_framework/# 独立多智能体框架（Python）
    ├── docs/                 # MUS4 架构、硬件、工具、计划文档
    ├── AGENTS.md             # MUS4 子项目 Agent 指南
    ├── CLAUDE.md             # MUS4 子项目 Claude 指南
    ├── README.md             # MUS4 对外 README（英文）
    ├── CHANGELOG.md          # MUS4 版本发布记录
    ├── arduino-cli.py        # 跨平台构建/上传/监控 Python 入口
    ├── arduino-cli-wsl.ps1   # Windows + WSL 加速构建与 OTA 包装脚本
    ├── config.yaml           # arduino-cli.py 配置
    ├── sketch.yaml           # Arduino CLI 项目默认配置
    ├── wslbuild.yaml         # WSL 构建覆盖配置
    └── ...
```

### 根级隔离约定（来自 `CLAUDE.md`）

1. **不要在仓库根添加源代码或可执行脚本**。若脚本只服务于某一固件，必须放在 `<子项目>/` 或 `<子项目>/tools/` 下。
2. **构建 / 烧录 / 测试 / 运行 Python 工具前，必须先 `cd` 到对应子项目根**。仓库根没有 sketch、没有构建入口、没有 Python 测试。
3. **`.gitignore` 分层**：根 `.gitignore` 只覆盖 OS / 编辑器层面的全局垃圾（`.DS_Store`、IDE 元数据等）。子项目相关的构建产物、密钥模板、缓存等**必须**写入子项目自己的 `.gitignore`。
4. 新增其它硬件平台的固件时，应作为根目录下并列同级子目录引入，目录命名采用 `<硬件型号>_FW/` 形式（全大写硬件代号 + `_FW` 后缀），与 `MUS4_FW/` 保持一致。

# 构建与测试命令

## 仓库根没有构建入口

- 根目录不存在 `pyproject.toml`、`package.json`、`Cargo.toml`、`Makefile` 或可直接调用的构建脚本。
- 唯一有效的固件构建与烧录入口在 `MUS4_FW/` 内。

## MUS4 子项目构建入口

进入子项目后，优先使用 WSL 加速构建（推荐）：

```powershell
cd MUS4_FW

# 默认验证：仅编译
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino

# 清理后重新编译
.\arduino-cli-wsl.ps1 -Compile -Clean -Sketch MUS4_FW.ino

# 编译 + HTTP OTA 上传（推荐）
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost <设备IP> -Sketch MUS4_FW.ino

# 依赖检查
.\arduino-cli-wsl.ps1 -Check
```

原生构建入口：

```bash
cd MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino       # 仅编译
python arduino-cli.py -cu --sketch MUS4_FW.ino      # 编译 + 上传
python arduino-cli.py -cus --sketch MUS4_FW.ino     # 编译 + 上传 + 监控
```

完整命令矩阵、参数说明与排障指引见：
- [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) §2「构建与上传命令」
- [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) §Commands

## 测试命令

MUS4 子项目的测试全部在 `MUS4_FW/tests/` 内运行：

```bash
cd MUS4_FW
pytest tests/
pytest tests/test_firmware_feature_flags.py
pytest tests/test_wireless_console_policy.py
pytest tests/test_arduino_cli.py
```

独立子项目测试：

```bash
# 配网系统代理测试
cd MUS4_FW/provisioning_system
python tests/test_agent.py -v
```

> `provisioning_system/playwright_tests/` 当前 `npm test` 是占位脚本，会退出失败；不要把它当作验证命令。

# 代码组织与技术栈

## 当前唯一的固件子项目：MUS4

- **语言**：C++17 with Arduino framework
- **目标平台**：ESP32（MUS4-v2.4.2 / v2.3 PCB）
- **主 Sketch**：`MUS4_FW/MUS4_FW.ino`
- **自有模块**：`MUS4_FW/libraries/mus4_*/src/` 下的本地 Arduino 库，包括 `mus4_core`、`mus4_rc`、`mus4_control`、`mus4_safety`、`mus4_command`、`mus4_diag`、`mus4_i2c`、`mus4_log`、`mus4_ui`、`mus4_web`、`mus4_wifi`。
- **第三方 Arduino 依赖**：FastLED、Adafruit INA219、Adafruit MPU6050、AsyncTCP、ESPAsyncWebServer、BleGamepad 等，本地副本位于 `MUS4_FW/libraries/`。
- **Python 工具与测试**：`pyyaml`、`pyserial`、`pytest`。

> 具体模块职责、数据流、主循环时序、引脚定义、串口协议、Wi-Fi/OTA 生命周期、BLE Gamepad 映射等，见 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) §1、§4、§6、§8 与 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md)。

## 根级文档

- `docs/superpowers/` 是仓库根级唯一的额外文档目录，存放 superpowers 技能产出的跨切面 `plans/`（实现计划）与 `specs/`（设计稿）。当前内容均针对 MUS4 固件，引用前需对照 `MUS4_FW/` 源码验证。

# 代码风格与开发约定

## 根级约定

- **子项目优先**：新增业务代码、构建脚本、测试、文档，默认下沉到具体子项目，不要上提到仓库根。
- **`.gitignore` 分层**：根 `.gitignore` 保持精简，只忽略 OS / 编辑器垃圾；子项目特有忽略项放入子项目 `.gitignore`。
- **不要自动执行远端 Git 操作**：不要自动 `git push`、`git reset`、`git rebase` 等；任何远端操作需用户明确授权。

## MUS4 子项目约定

进入 `MUS4_FW/` 后，遵循该子项目的约定：

- 头文件使用 `#pragma once`。
- 常量 / 宏使用 `ALL_CAPS`；类名 `PascalCase`；类方法 `camelCase`；自由函数 `snake_case`；私有成员下划线前缀。
- 引脚定义、编译开关、时序常量集中放在 `libraries/mus4_core/src/FirmwareConfig.h`。
- 新增业务模块优先以 `libraries/mus4_<domain>/` 本地 Arduino 库形式存在，并提供同名聚合头文件 `mus4_<domain>.h`。
- 中断服务函数必须标注 `IRAM_ATTR`；与中断共享的变量使用 `volatile`。
- 注释语言：硬件相关注释使用中文；代码逻辑与公共 API 使用英文。

完整风格表见 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) §5。

# 安全注意事项

## 根级安全

- 仓库根不存放敏感信息。任何含真实凭据或目标地址的文件（如 `WirelessSecrets.h`、`.mus4_ota_target`）应位于对应子项目内，并已由子项目 `.gitignore` 排除。
- 不要在根目录留下临时凭据、私钥或设备地址。

## MUS4 子项目安全（关键）

MUS4 直接控制舵机和电调，属于安全关键固件：

- 修改输出映射、Park、紧急制动、模式融合或无线控制入口时，必须保留 PWM 限幅与失效安全路径。
- 所有串口 / Web Console / TCP Console 输入视为不可信边界，控制命令必须经过认证、权限检查和范围校验后才能影响输出。
- 无线命令权限分层策略在 `MUS4_FW/wireless_console_policy.py` 与 `MUS4_FW/libraries/mus4_command/src/WirelessConsole.cpp` 中必须保持同步；修改后同步更新 `tests/test_wireless_console_policy.py`。
- `WirelessSecrets.h`、`.mus4_ota_target`、`ArduFlux.json` 等本地文件不应纳入版本控制。

详细安全说明见 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) §7。

# 文档地图

| 文件 | 作用 |
|------|------|
| [`README.md`](README.md) | 仓库根简介，列在驻子项目 |
| [`CLAUDE.md`](CLAUDE.md) | 仓库根级 Claude Code 约定 |
| [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) | MUS4 子项目面向 AI 代理的完整指南 |
| [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) | MUS4 子项目面向 Claude Code 的权威手册 |
| [`MUS4_FW/README.md`](MUS4_FW/README.md) | MUS4 对外 README（英文） |
| [`MUS4_FW/CHANGELOG.md`](MUS4_FW/CHANGELOG.md) | MUS4 版本发布记录 |
| [`MUS4_FW/docs/`](MUS4_FW/docs/) | MUS4 架构、硬件、工具、计划文档 |
| [`docs/superpowers/`](docs/superpowers/) | 根级实现计划与设计规格 |

# 快速检查清单

开始处理任务前：

1. 确认当前工作目录；如需修改固件或运行构建 / 测试，**先 `cd MUS4_FW`**。
2. 阅读 [`MUS4_FW/AGENTS.md`](MUS4_FW/AGENTS.md) 与 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) 的相关章节。
3. 检查修改是否涉及安全关键路径；若是，保留 PWM 限幅、权限校验与失效安全。
4. 修改 `wireless_console_policy.py` 或 Web Console UI 后，运行对应 `pytest`。
5. 修改固件源码后，优先通过 WSL 编译验证：`.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`。
