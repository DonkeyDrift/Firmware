# MUS4_FW 源码目录整理方案

> **目标**：把根目录平铺的 30+ 对 `.h/.cpp` 重新组织，使根目录更直观、模块边界更清晰，并符合 Arduino / arduino-cli 的编译规范。**本轮不动 `examples/`、`libraries/`、`tools/`、`tests/`、`docs/`、`multi_agent_framework/`、`provisioning_system/`，也暂不动 `arduino-cli.py` / `arduino-cli-wsl.ps1` / `config.yaml` / `wslbuild.yaml` 中的源码路径假设。**

---

## 1. 现状与问题

### 1.1 根目录文件清单

`MUS4_FW.ino` 同目录平铺了 **29 对 `.h/.cpp`（58 个文件）** + 9 个纯头 / 配置 / 共享类型（合计 67 个源文件）：

| 类别 | 文件 | 说明 |
|------|------|------|
| Sketch 入口 | `MUS4_FW.ino` | 主入口，**保留在根目录** |
| 全局配置与共享类型 | `FirmwareConfig.h`, `BuildInfo.h`, `SharedTypes.h`, `RuntimeState.h`, `SerialBufferTypes.h`, `WifiConsoleTypes.h`, `WebConsoleAssets.h`, `WirelessSecrets.example.h` | 8 个 |
| 工具/UI/日志 | `StringPrint.h`（纯头）, `JsonUtil.h/.cpp`, `I2CBusTools.h/.cpp`, `Mus4Log.h/.cpp`, `LedStatus.h/.cpp`, `Buzzer.h/.cpp`, `TUI.h/.cpp` | 1 个纯头 + 6 对 |
| 串口命令 | `SerialLineReader.h/.cpp`, `CommandParser.h/.cpp`, `CommandDispatcher.h/.cpp`, `LocalCommands.h/.cpp`, `WirelessConsole.h/.cpp` | 5 对 |
| RC/控制/安全/执行 | `RcPwmCapture.h/.cpp`, `RcFilter.h/.cpp`, `ControlMixer.h/.cpp`, `SafetyState.h/.cpp`, `ActuatorOutput.h/.cpp`, `DriftAssist.h/.cpp`, `SteeringControl.h/.cpp`, `SteeringCalibration.h/.cpp` | 8 对 |
| 传感器 | `Sensors.h/.cpp` | 1 对 |
| Wi-Fi/Web | `WifiManager.h/.cpp`, `WifiOta.h/.cpp`, `WifiStaConfig.h/.cpp`, `WifiIdentity.h/.cpp`, `WebConsoleServer.h/.cpp`, `WebTelemetry.h/.cpp`, `WebLogBuffer.h/.cpp` | 7 对 |
| 诊断 | `Diagnostics.h/.cpp`, `GamepadMode.h/.cpp` | 2 对 |
| 构建脚本与配置 | `arduino-cli.py`, `arduino-cli-wsl.ps1`, `build_wsl.ps1`, `config.yaml`, `sketch.yaml`, `wslbuild.yaml`, `ArduFlux.json`, `README.md`, `README.zh-CN.md`, `LICENSE`, `CHANGELOG.md`, `AGENTS.md`, `CLAUDE.md`, `.gitignore` | 留在根目录 |

> **问题**：
> 1. 根目录**同时承担"Sketch 入口目录"和"模块源码目录"两个角色**，67 个 `.h/.cpp` 与构建脚本、配置、文档混杂在一起，IDE/编辑器左侧文件树一眼看不出模块归属。
> 2. 命名虽然有 `Rc*` / `Wifi*` / `Web*` 前缀隐式分组，但**没有物理隔离**，阅读、Review、检索都很累。
> 3. 与 Arduino 官方"主 sketch 入口应保持精简"的建议（详见 `src/` 文件夹规范）不符。

### 1.2 已有的拆分工作

- `docs/Plan/MUS4_FW模块化拆分方案.md` 已经按**逻辑模块**完成 8 轮切片（`RcPwmCapture`、`ControlMixer`、`SafetyState`、`ActuatorOutput` 等），但**仅做逻辑拆分，未做物理目录整理**。
- `tests/test_firmware_feature_flags.py` 维护着 50+ 个文件路径常量（`PROJECT_ROOT / "X.h"`），是当前唯一的"位置真相"。

---

## 2. Arduino / arduino-cli 对源码布局的硬约束

在提方案前，必须先明确 Arduino 编译的硬约束，否则改了目录也会被 IDE 忽略。

### 2.1 编译单元与 include 路径规则

`arduino-cli compile <sketch>` 实际把以下来源都纳入编译：

1. **主 sketch 文件**（`<sketch>.ino`）
2. **sketch 目录及其所有递归子目录中**的 `.ino`、`.cpp`、`.c`、`.S`（**注**：仅在 IDE 2.x 处理 `.ino` 时递归；`.cpp/.c/.S` 在 1.x 时代不递归）
3. **`src/` 子目录**（IDE 2.x / arduino-cli 0.18+ 支持）中的 `.cpp`、`.c`、`.S`、`.h` —— **这是 Arduino 官方对"Sketch 内部多文件"的标准方案**
4. **`libraries/`（通过 `--libraries` 注入）** 下的标准 Arduino 库（含 `library.properties` / `src/` 子目录结构）

### 2.2 关键约束

- **直接放在 sketch 根目录的 `.cpp` 会被编译**，但**子目录里的 `.cpp` 在 Arduino 1.x / 老 arduino-cli 中不会被自动编译**。Arduino 2.x 仅 `src/` 这一层目录是"自动编译"白名单，**`src/` 之下的子目录不会被自动加入编译**（这点与 PlatformIO 的 `lib/` 行为不同）。
- **include 解析顺序**：`#include "X.h"` 优先相对当前 `.cpp/.h` 所在目录查找 → sketch 根目录 → `src/` → 用户库。
- `libraries/` 下的库若提供 `src/<LibraryName>.h`，主 sketch 可写 `#include <LibraryName.h>`；若不提供，则按"目录 + 文件"形式包含。

### 2.3 推论

要"既能分目录、又能被 Arduino 自动编译"且**改动最小**，只有两条路：

| 路径 | 适用 | 限制 |
|------|------|------|
| **A. `src/` 平铺** | 简单、保留 `#include "X.h"` 不变 | 无法再做物理子目录分组，否则不会被 Arduino 自动编译 |
| **B. 转 Arduino 库（`libraries/<Name>/`）** | 可任意子目录、可对外复用 | 需要写 `library.properties`、改 `#include` 写法（`#include <Name/Header.h>`） |

---

## 3. 三个候选方案

### 方案 A：`src/` 平铺（最小改动）

把根目录的 29 对 `.h/.cpp`（58 个文件） + 9 个纯头/配置 全部移入 `src/`，根目录仅保留：

```
MUS4_FW/                                    # Sketch 根 = arduino-cli 入口
├── MUS4_FW.ino                             # 主 sketch
├── src/                                    # 所有固件源码集中
│   ├── FirmwareConfig.h
│   ├── BuildInfo.h
│   ├── SharedTypes.h
│   ├── RuntimeState.h
│   ├── ...其余 30 对 .h/.cpp
│   └── WirelessSecrets.example.h
├── examples/                               # 不动
├── libraries/                              # 不动
├── tools/                                  # 不动
├── tests/                                  # 不动（仅需更新 PROJECT_ROOT 路径）
├── docs/                                   # 不动
├── multi_agent_framework/                  # 不动
├── provisioning_system/                    # 不动
├── arduino-cli.py / *.ps1 / *.yaml         # 不动
└── README.md / CHANGELOG.md / ...          # 不动
```

**优点**：
- 改动面最小。所有 `#include "X.h"` 不变（Arduino 把 `src/` 加进 include path）。
- 根目录文件数从 ~90+ 减少到 ~25。
- 符合 Arduino IDE 2.x / arduino-cli 0.18+ 的官方推荐做法。

**缺点**：
- `src/` 内部仍是平铺 67 个文件；分组的"意义"靠**文件名前缀**（`Rc*` / `Wifi*` / `Web*`）传达，没有物理隔离。
- 真正想做"按子目录分组"还是做不到。

**适配度**：对当前问题"根目录太乱"**已经足够**；对"模块边界更清晰"**部分缓解**。

### 方案 B：转 Arduino 库（最规范，但最重）

把**自包含、对外可复用**的模块改造成 `libraries/mus4_<feature>/` 下的标准 Arduino 库：

```
libraries/
├── mus4_rc_input/        # RcPwmCapture + RcFilter
│   ├── library.properties
│   └── src/mus4_rc_input.h
├── mus4_control/         # ControlMixer + DriftAssist + SteeringControl + SafetyState + ActuatorOutput
├── mus4_wifi/            # WifiManager + WifiOta + WifiStaConfig + WifiIdentity
├── mus4_web_console/     # WebConsoleServer + WebTelemetry + WebLogBuffer + WebConsoleAssets
├── mus4_command/         # CommandParser + CommandDispatcher + LocalCommands + WirelessConsole + SerialLineReader
├── mus4_sensors/         # Sensors + I2CBusTools
├── mus4_ui/              # TUI + Buzzer + LedStatus + Mus4Log
└── ...                   # 原有第三方库保持原位
```

`MUS4_FW.ino` 与 `src/` 内的配置/共享类型改为：

```cpp
#include "FirmwareConfig.h"
#include <mus4_rc_input.h>
#include <mus4_control.h>
#include <mus4_sensors.h>
#include <mus4_wifi.h>
#include <mus4_web_console.h>
#include <mus4_command.h>
#include <mus4_ui.h>
```

**优点**：
- 完全符合 Arduino 库规范；每个库有 `library.properties`、明确的公共/私有头、关键词 `keywords.txt`。
- 真正的物理子目录隔离，模块边界最清晰。
- 这些模块未来若需要给其它固件复用，**直接拷贝 `libraries/mus4_xxx/` 即可**。
- IDE 侧栏会按"库"分组显示，更直观。

**缺点**：
- **改动面大**：每个库要写 `library.properties`、设计公共/私有头（当前很多 `.h` 没有 `_impl.h` 切分）、改几十处 `#include` 写法。
- 库与 `src/` 之间的依赖循环需要拆分（例：`Sensors` 依赖 `SharedTypes`，而 `SharedTypes` 又依赖 `FirmwareConfig` —— 当前通过 `extern` 桥接勉强绕开；改为库后需要明确"纯类型库 / 配置库 / 实现库"的分层）。
- `arduino-cli.py` 当前 `--libraries libraries` 是单一路径；改成多个库后无需改命令，但本地库的发现/构建顺序需要在测试中验证。

**适配度**：对"模块边界更清晰 + 符合 Arduino 规范"**最匹配**，但**实施成本最高**，且与 `docs/Plan/MUS4_FW模块化拆分方案.md` 的"先测试保护、再机械迁移"原则相冲突。

### 方案 C：混合（推荐）

**A 的物理整理 + B 的命名约束**，不引入 `library.properties`，但通过 `src/` 内的**子目录**+**显式 `build_src.py`（可选）**实现分组。具体落地：

```
src/                                   # 所有固件源码
├── app/                               # 跨模块装配与 sketch 入口辅助
│   ├── FirmwareConfig.h               # 配置中心
│   ├── BuildInfo.h
│   ├── SharedTypes.h
│   ├── RuntimeState.h
│   └── ...
├── common/                            # 工具/UI/日志/控制台输出
│   ├── StringPrint.h
│   ├── JsonUtil.h/.cpp
│   ├── I2CBusTools.h/.cpp
│   ├── Mus4Log.h/.cpp
│   ├── LedStatus.h/.cpp
│   ├── Buzzer.h/.cpp
│   ├── TUI.h/.cpp
│   └── ...
├── command/                           # 串口/Pilot 命令解析与分发
│   ├── SerialLineReader.h/.cpp
│   ├── CommandParser.h/.cpp
│   ├── CommandDispatcher.h/.cpp
│   ├── LocalCommands.h/.cpp
│   ├── WirelessConsole.h/.cpp
│   ├── WifiConsoleTypes.h
│   └── SerialBufferTypes.h
├── rc/                                # RC 输入 / 滤波
│   ├── RcPwmCapture.h/.cpp
│   └── RcFilter.h/.cpp
├── control/                           # 控制融合 / 安全 / 执行
│   ├── ControlMixer.h/.cpp
│   ├── SafetyState.h/.cpp
│   ├── ActuatorOutput.h/.cpp
│   ├── DriftAssist.h/.cpp
│   ├── SteeringControl.h/.cpp
│   ├── SteeringCalibration.h/.cpp
│   └── WebConsoleAssets.h             # Web 静态资源体积大
├── sensors/                           # I2C/INA219/MPU6050
│   ├── Sensors.h/.cpp
│   └── ...
├── wifi/                              # Wi-Fi 状态机 / OTA / STA / 身份
│   ├── WifiManager.h/.cpp
│   ├── WifiOta.h/.cpp
│   ├── WifiStaConfig.h/.cpp
│   ├── WifiIdentity.h/.cpp
│   └── WirelessSecrets.example.h
├── web/                               # Web Console / WebSocket / 日志
│   ├── WebConsoleServer.h/.cpp
│   ├── WebTelemetry.h/.cpp
│   └── WebLogBuffer.h/.cpp
└── diag/                              # 诊断 / 压测 / BLE
    ├── Diagnostics.h/.cpp
    └── GamepadMode.h/.cpp
```

**但** —— 由于 Arduino 默认**不递归编译 `src/` 子目录**，需要同时做下面两件事之一：

- **C-1（推荐）**：在 `arduino-cli.py` / `arduino-cli-wsl.ps1` 增加 `build_src_files` 自动发现脚本，把 `src/**/*.cpp` 追加到编译命令。  
  - 优点：完全符合用户"想物理分组"诉求，物理隔离。  
  - 代价：构建脚本需要小改（明确写"不违反用户范围限制"——本轮**不做**，只列在方案里）。
- **C-2（退路）**：保持 `src/` 平铺，但**按子目录建立空 `.gitkeep` 占位 + 在文件名前加分组前缀**（如 `rc_RcPwmCapture.h`、`wifi_WifiManager.h`）。  
  - 优点：完全不动 Arduino 编译规则、不动 build 脚本。  
  - 代价：分组的"意义"靠前缀传达，依然不彻底。

---

## 4. 推荐：先 A，必要时再升级 C / B

考虑到用户的两个明确偏好（**只先出方案文档**、**只动 .h/.cpp 排布**），并结合 AGENTS.md 强调的"先测试保护、再机械迁移、低风险优先"原则，本轮**推荐方案 A** 作为落地，**方案 C** 列为"未来可选升级"，**方案 B** 列为"长期目标"。

理由：

1. **方案 A 已经解决核心痛点**：根目录不再平铺 67 个源文件，IDE 侧栏可读性立刻改善。
2. **方案 A 不违反 Arduino 规范**：完全使用 Arduino 2.x 官方 `src/` 约定，是 arduino-cli 0.18+ 文档化支持的布局。
3. **方案 A 几乎不需要改 `#include`**：现有 `#include "X.h"` 在 `src/` 模式下零修改，迁移时只要把文件挪位置即可。
4. **方案 A 不动 build 脚本**：`arduino-cli.py` / `arduino-cli-wsl.ps1` 完全不需要改（它们对 sketch 路径使用 `MUS4_FW.ino`，并不假设源码在哪个子目录）。
5. **方案 B 改动太大**：当前很多模块（如 `ControlMixer` ↔ `SafetyState` ↔ `ActuatorOutput`）有外部状态依赖，转库需要先做接口清理，与"先机械迁移、再行为不变"的现状不符。
6. **方案 C 的物理子目录**：需要小改 build 脚本（与"只动 .h/.cpp 排布"的明确偏好相冲突），作为未来选项保留。

---

## 5. 方案 A 详细落地清单

### 5.1 文件移动映射

**移到 `src/`**（67 个文件）：

| 源（根目录） | 目标 |
|---|---|
| `BuildInfo.h` | `src/BuildInfo.h` |
| `FirmwareConfig.h` | `src/FirmwareConfig.h` |
| `SharedTypes.h` | `src/SharedTypes.h` |
| `RuntimeState.h` | `src/RuntimeState.h` |
| `SerialBufferTypes.h` | `src/SerialBufferTypes.h` |
| `StringPrint.h` | `src/StringPrint.h` |
| `WifiConsoleTypes.h` | `src/WifiConsoleTypes.h` |
| `WebConsoleAssets.h` | `src/WebConsoleAssets.h` |
| `WirelessSecrets.example.h` | `src/WirelessSecrets.example.h` |
| `JsonUtil.h`, `JsonUtil.cpp` | `src/JsonUtil.h`, `src/JsonUtil.cpp` |
| `I2CBusTools.h`, `I2CBusTools.cpp` | `src/I2CBusTools.h`, `src/I2CBusTools.cpp` |
| `LedStatus.h`, `LedStatus.cpp` | `src/LedStatus.h`, `src/LedStatus.cpp` |
| `Mus4Log.h`, `Mus4Log.cpp` | `src/Mus4Log.h`, `src/Mus4Log.cpp` |
| `Buzzer.h`, `Buzzer.cpp` | `src/Buzzer.h`, `src/Buzzer.cpp` |
| `TUI.h`, `TUI.cpp` | `src/TUI.h`, `src/TUI.cpp` |
| `SerialLineReader.h`, `SerialLineReader.cpp` | `src/SerialLineReader.h`, `src/SerialLineReader.cpp` |
| `CommandParser.h`, `CommandParser.cpp` | `src/CommandParser.h`, `src/CommandParser.cpp` |
| `CommandDispatcher.h`, `CommandDispatcher.cpp` | `src/CommandDispatcher.h`, `src/CommandDispatcher.cpp` |
| `LocalCommands.h`, `LocalCommands.cpp` | `src/LocalCommands.h`, `src/LocalCommands.cpp` |
| `WirelessConsole.h`, `WirelessConsole.cpp` | `src/WirelessConsole.h`, `src/WirelessConsole.cpp` |
| `RcPwmCapture.h`, `RcPwmCapture.cpp` | `src/RcPwmCapture.h`, `src/RcPwmCapture.cpp` |
| `RcFilter.h`, `RcFilter.cpp` | `src/RcFilter.h`, `src/RcFilter.cpp` |
| `ControlMixer.h`, `ControlMixer.cpp` | `src/ControlMixer.h`, `src/ControlMixer.cpp` |
| `SafetyState.h`, `SafetyState.cpp` | `src/SafetyState.h`, `src/SafetyState.cpp` |
| `ActuatorOutput.h`, `ActuatorOutput.cpp` | `src/ActuatorOutput.h`, `src/ActuatorOutput.cpp` |
| `DriftAssist.h`, `DriftAssist.cpp` | `src/DriftAssist.h`, `src/DriftAssist.cpp` |
| `SteeringControl.h`, `SteeringControl.cpp` | `src/SteeringControl.h`, `src/SteeringControl.cpp` |
| `SteeringCalibration.h`, `SteeringCalibration.cpp` | `src/SteeringCalibration.h`, `src/SteeringCalibration.cpp` |
| `Sensors.h`, `Sensors.cpp` | `src/Sensors.h`, `src/Sensors.cpp` |
| `WifiManager.h`, `WifiManager.cpp` | `src/WifiManager.h`, `src/WifiManager.cpp` |
| `WifiOta.h`, `WifiOta.cpp` | `src/WifiOta.h`, `src/WifiOta.cpp` |
| `WifiStaConfig.h`, `WifiStaConfig.cpp` | `src/WifiStaConfig.h`, `src/WifiStaConfig.cpp` |
| `WifiIdentity.h`, `WifiIdentity.cpp` | `src/WifiIdentity.h`, `src/WifiIdentity.cpp` |
| `WebConsoleServer.h`, `WebConsoleServer.cpp` | `src/WebConsoleServer.h`, `src/WebConsoleServer.cpp` |
| `WebTelemetry.h`, `WebTelemetry.cpp` | `src/WebTelemetry.h`, `src/WebTelemetry.cpp` |
| `WebLogBuffer.h`, `WebLogBuffer.cpp` | `src/WebLogBuffer.h`, `src/WebLogBuffer.cpp` |
| `Diagnostics.h`, `Diagnostics.cpp` | `src/Diagnostics.h`, `src/Diagnostics.cpp` |
| `GamepadMode.h`, `GamepadMode.cpp` | `src/GamepadMode.h`, `src/GamepadMode.cpp` |

**保留在根目录**：

- `MUS4_FW.ino`（Sketch 入口）
- `WirelessSecrets.h`（**用户主目录凭据**，`/src` 会让它进入源码树，不安全；**保持根目录且 `.gitignore` 已排除**）
- `arduino-cli.py` / `arduino-cli-wsl.ps1` / `build_wsl.ps1` / `config.yaml` / `sketch.yaml` / `wslbuild.yaml` / `ArduFlux.json`
- `README.md` / `README.zh-CN.md` / `CHANGELOG.md` / `LICENSE` / `AGENTS.md` / `CLAUDE.md`
- `.gitignore`
- 整个 `examples/`、`libraries/`、`tools/`、`tests/`、`docs/`、`multi_agent_framework/`、`provisioning_system/`、`provisioning_system/` 子树

### 5.2 `#include` 路径变更

**所有 `#include "X.h"` 形式保持不变**。`arduino-cli compile` 会把 `src/` 加入 include path，文件按名查找仍然命中。

**唯一需要确认的边界情况**：
- `src/WebConsoleAssets.h` 内 `PROGMEM` HTML 体积较大，与其它 `.h` 不同；但 IDE/编译器视其为普通头。
- `WirelessSecrets.h` **必须在根目录**（不入 `src/`），且 Sketch 内 `WifiStaConfig.cpp` 仍写 `#include "WirelessSecrets.h"`。因为 `WirelessSecrets.h` 留根目录，include 路径解析顺序为"sketch 根 → src/"，能命中根目录的副本。

### 5.3 必须同步更新的文件

| 文件 | 更新内容 |
|---|---|
| `tests/test_firmware_feature_flags.py` | 把 `PROJECT_ROOT / "X.h"` 全部改为 `PROJECT_ROOT / "src" / "X.h"`（约 50 处）。**自动保留 PROJECT_ROOT / "MUS4_FW.ino"`** |
| `tests/test_firmware_feature_flags.py` | 若有断言 "**所有 .h/.cpp 都在根目录**" 类的硬约束，需要删掉或改成"在 src/ 下" |
| `.gitignore` | 确认仍忽略 `build/` / `build_wsl/` / `WirelessSecrets.h` 等，**新增** `src/WirelessSecrets.h` 形式的占位（如果未来有人误把凭据放进去）—— 但目前 `WirelessSecrets.h` 在根目录，**无需新增** |
| `docs/Arch/architecture.md` | 把"模块清单"段落里的 `MUS4_FW.ino` 同级文件列表改为 `src/` 形式 |
| `docs/Hardware/pin_definitions.md` | 不变 |
| `docs/README/DevNote.md` / `OPERATIONS.md` | 检查是否有"打开某 .h"的路径叙述 |
| `CHANGELOG.md` | 增补一条"重构"条目 |
| `README.md` / `README.zh-CN.md` | 章节"项目结构"同步更新（若已写） |

### 5.4 不需要改的文件

- `arduino-cli.py` / `arduino-cli-wsl.ps1` / `build_wsl.ps1` —— 它们用 `MUS4_FW.ino` 作为 sketch 入口，编译单位仍是该文件；`src/` 是 arduino-cli 默认行为，无需配置。
- `config.yaml` / `sketch.yaml` / `wslbuild.yaml` —— 同上。
- `examples/smart_provisioning/*.ino` 等独立 sketch —— 它们不在 `MUS4_FW` sketch 内，不受影响。
- `multi_agent_framework/` / `provisioning_system/` / `tools/` —— 与固件编译无关。

### 5.5 验证步骤

按"先测试保护、再机械迁移"原则：

1. **迁移前**：
   - 跑 `pytest tests/test_firmware_feature_flags.py` 全绿（约 71+ 个 test function / 958+ 断言）。
   - 跑 `pytest tests/` 全绿。
   - 跑 `.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`，**基准编译通过** + 分区占用记录。
   - 可选：跑 `TEST` / `BENCH` 串口命令采集基线（仅供肉眼对比，不作为门禁）。

2. **迁移**（机械操作）：
   - `git mv` 67 个文件到 `src/`。
   - 更新 `tests/test_firmware_feature_flags.py` 的 `PROJECT_ROOT` 路径前缀。
   - 更新 `docs/Arch/architecture.md` / `DevNote.md` 中涉及的目录描述。
   - 提交。

3. **迁移后**：
   - 跑 `pytest tests/` 全绿。
   - 跑 `.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`，**编译通过 + 分区占用与基线一致**。
   - （可选）`.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta` 烧录并验证 Web Console 正常。

4. **回滚预案**：
   - 单个 commit 即可回滚（`git revert`）。
   - 若 `src/` 模式下 arduino-cli 出现 include 找不到，立即检查 `src/` 内是否有同名文件冲突、或者 `WirelessSecrets.h` 是否误移入。

### 5.6 风险与缓解

| 风险 | 缓解 |
|---|---|
| arduino-cli 0.18 之前的版本不支持 `src/` | 项目 `config.yaml` 已显式依赖 `arduino-cli`；`docs/Tools/arduino-cli-wsl_manual.md` 也明示最低版本。WSL 镜像里固定为最新稳定版。 |
| `WirelessSecrets.h` 被误移入 `src/` | 在 PR / commit 信息中强调；通过 `git mv` 显式保留在根目录。 |
| Arduino IDE 1.x 用户（如果有） | AGENTS.md 明确以 arduino-cli 为主、IDE 1.x 不在支持矩阵；不影响。 |
| `examples/smart_provisioning/` 被误带进 `src/` | 不在本次移动列表中，**单独保留在 `examples/`**。 |
| IDE 侧栏分组消失 | 编辑器对 `src/` 仍可建立虚拟工作集；不影响构建。 |
| 后续从 `src/` 再升级到方案 C / B | `src/` 是后续两种方案的**必要中间态**，不会浪费。 |

---

## 6. 未来升级路径（不在本轮执行）

### 6.1 升级到方案 C

若希望进一步做"物理子目录分组"：

1. 在 `src/` 下建立 `app/`、`common/`、`command/`、`rc/`、`control/`、`sensors/`、`wifi/`、`web/`、`diag/` 子目录。
2. `arduino-cli.py` 增加 `gather_srcs()` 工具函数：遍历 `src/**/*.cpp`，按发现顺序追加到编译命令；在 `arduino-cli-wsl.ps1` 增加对应同步逻辑。
3. 更新 `tests/test_firmware_feature_flags.py` 的 `FIRMWARE_SOURCE_PATHS` 为 `(PROJECT_ROOT / "src" / ...)` 形式。
4. 更新 `docs/Arch/architecture.md` / `AGENTS.md` 中的"模块清单"。

### 6.2 升级到方案 B

若希望最终做到"模块即 Arduino 库"：

1. 先按方案 C 落物理子目录。
2. 逐个模块设计"公共头" vs "私有实现"边界（`_impl.h` 拆分）。
3. 每个 `libraries/mus4_<feature>/` 写 `library.properties`（`name` / `version` / `author` / `depends`）。
4. 主 sketch 改 `#include <mus4_<feature>.h>`。
5. 解决跨库类型依赖（`SharedTypes.h` / `FirmwareConfig.h` / `RuntimeState.h` 需提升为 `mus4_core` 库或保留在 `src/`）。

---

## 7. 文档与日志

- 本轮迁移完成后，在 `CHANGELOG.md` 增加一条"重构 (refactor): 将固件源码统一移入 `src/`，符合 Arduino IDE 2.x / arduino-cli 规范"。
- 在 `AGENTS.md` §1.3 "代码组织"段落同步插入 `src/` 树状图。
- 在 `README.md` / `README.zh-CN.md` §"项目结构"段落同步。

---

## 8. 待用户确认的开放问题

1. **确认采用方案 A**：若倾向 C / B，请指明（会扩大本轮改动面）。
2. **`WirelessSecrets.h` 的归属**：本方案保持根目录；若你也希望它进 `src/`，需要额外处理 `.gitignore` 规则（`src/WirelessSecrets.h`）并写好占位模板。
3. **`docs/Arch/architecture.md` 等文档是否需要同步重写**：本方案默认仅做"路径替换"，不做架构性重写。
4. **是否需要保留原 `.h/.cpp` 在根目录一段时间作为软链接/重定向**：本方案默认一次性移动，不留软链接。
