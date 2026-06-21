# MUS4_FW 源码目录整理方案

> **目标**：把根目录平铺的 30+ 对 `.h/.cpp` 重新组织，使根目录更直观、模块边界更清晰，并符合 Arduino / arduino-cli 的编译规范。
>
> **2026-06-17 修订**：原方案 A（"`src/` 平铺"）在 `arduino-cli 1.4.1` 实际编译中**不可行**——`src/` 在官方规范中是给"自包含库"用的，不是给 sketch 源码用的。已通过 `python arduino-cli.py -c` 实测：移到 `src/` 后 67 个 `.cpp` 全部不被自动编译、`FirmwareConfig.h` 也找不到。本文档保留原方案作为"被否定的方案"参考，并补充经实测的可行路径。

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
> 3. 与 Arduino 官方"Sketch 源码应集中在 sketch 根"（不能放子目录）这一约束相符，但**不直观**。

### 1.2 已有的拆分工作

- `docs/Plan/MUS4_FW模块化拆分方案.md` 已经按**逻辑模块**完成 8 轮切片（`RcPwmCapture`、`ControlMixer`、`SafetyState`、`ActuatorOutput` 等），但**仅做逻辑拆分，未做物理目录整理**。
- `tests/test_firmware_feature_flags.py` 维护着 50+ 个文件路径常量（`PROJECT_ROOT / "X.h"`），是当前唯一的"位置真相"。

---

## 2. Arduino / arduino-cli 对源码布局的硬约束

在提方案前，必须先明确 Arduino 编译的硬约束，否则改了目录也会被 IDE 忽略。

### 2.1 编译单元与 include 路径规则

`arduino-cli compile <sketch>` 实际把以下来源都纳入编译：

1. **主 sketch 文件**（`<sketch>.ino`）
2. **sketch 根目录** 中的 `.ino`、`.cpp`、`.c`、`.S`、`.h`（其中 `.h` 仅在被 `.cpp/.ino` 引用时参与编译）
3. **`src/` 子目录**：根据 [arduino-cli 1.5 官方 sketch 规范](https://arduino.github.io/arduino-cli/1.5/sketch-specification/#src-subfolder)，`src/` 用于**打包自包含的第三方库**，**不是 sketch 源码目录**：
   > "The contents of the `src` subfolder are compiled recursively. ... It can be used to **bundle libraries** with the sketch in order to make it a self-contained project. **Arduino language files under the `src` folder are not supported.**"
4. **`libraries/`（通过 `--libraries` 注入）** 下的标准 Arduino 库（含 `library.properties` / `src/` 子目录结构）

### 2.2 关键约束（**实测后修正**）

> **2026-06-17 实测结论**（`arduino-cli 1.4.1` / `esp32:esp32 3.3.10-cn` / `Commit: e39419312`）：
>
> 把 67 个 `.h/.cpp` 移到 `src/` 后，运行 `arduino-cli compile --verbose MUS4_FW.ino`：
> 1. **`.cpp` 文件不会被自动编译** —— verbose 输出只出现 `MUS4_FW.ino.cpp`，`src/*.cpp` 都没进入编译命令。
> 2. **`src/` 不会被自动加入 include path** —— 编译命令仅有 `-IC:\Dev\DDC\MUS4_FW`，主 sketch 报 `FirmwareConfig.h: No such file or directory`。
> 3. arduino-cli 会把 `src/` 复制到 temp 目录，但仅作为"扫描第三方库"的尝试，并非作为 sketch 源码识别。
> 4. 用 `--build-property "build.extra_flags=-Isrc"` 强行把 `src/` 加进 include path，**会被传递给所有库（FastLED 等）编译**，污染第三方库源码路径。
>
> 因此**`src/` 在 arduino-cli 1.4.1 下不是 sketch 源码目录**，不能用于本次整理。

### 2.3 实际可行的"既能分目录、又能被 Arduino 自动编译"路径

经实测，下面是当前 arduino-cli 实际支持的几种组织形式：

| 路径 | 适用 | 改动面 | 实测可用 |
|------|------|--------|----------|
| **A. 转 Arduino 库（`libraries/<Name>/`）** | 模块自包含、对外可复用 | 需要写 `library.properties`、拆公共/私有头、改 `#include` 写法（`#include <Name/Header.h>`） | ✅ 官方支持 |
| **B. 保持根目录平铺** | 改动最小 | 维持现状，"根目录乱"问题不解决 | ✅ 当前状态 |
| **C. `src/` 平铺** | ~~"Arduino 官方推荐"~~ | ~~零 #include 改动~~ | ❌ **实测不可行**（本计划 1.0 误判） |
| **D. `build.extra_flags=-Isrc`** | 理论可加 include path | 污染所有库编译，**FastLED 等报错** | ❌ **实测不可行** |

---

## 3. 修正后的方案

### 3.1 方案对比

| 方案 | 改动面 | 根目录整洁度 | 模块边界清晰度 | 推荐度 |
|------|--------|--------------|----------------|--------|
| A. 转 Arduino 库 | 中 | 高 | 高 | ⭐⭐⭐（最规范） |
| B. 保持平铺 | 0 | 低 | 中 | ⭐（不改） |
| ~~C. `src/` 平铺~~ | ~~小~~ | ~~高~~ | ~~中~~ | ❌（已否） |

### 3.2 方案 A：转 Arduino 库（推荐）

**核心思路**：把**自包含、对外可复用**的模块改造成 `libraries/mus4_<feature>/` 下的标准 Arduino 库。

#### 3.2.1 拟划分（9 个库）

```
libraries/
├── mus4_core/              # FirmwareConfig + BuildInfo + SharedTypes + RuntimeState + SerialBufferTypes + StringPrint + WirelessSecrets
├── mus4_log/               # Mus4Log + JsonUtil
├── mus4_ui/                # TUI + Buzzer + LedStatus
├── mus4_i2c/               # I2CBusTools + Sensors
├── mus4_rc/                # RcPwmCapture + RcFilter
├── mus4_control/           # ControlMixer + DriftAssist + SteeringControl + SteeringCalibration
├── mus4_safety/            # SafetyState + ActuatorOutput
├── mus4_wifi/              # WifiManager + WifiOta + WifiStaConfig + WifiIdentity
├── mus4_web/               # WebConsoleServer + WebTelemetry + WebLogBuffer + WebConsoleAssets + WifiConsoleTypes
├── mus4_command/           # CommandParser + CommandDispatcher + LocalCommands + WirelessConsole + SerialLineReader
└── mus4_diag/              # Diagnostics + GamepadMode
```

每个 `libraries/mus4_<feature>/` 标准结构：

```
libraries/mus4_<feature>/
├── library.properties      # name / version / author / depends
├── keywords.txt            # IDE 关键词
└── src/
    ├── mus4_<feature>.h    # 公共头（重导出或聚合）
    ├── <OriginalName>.h    # 原模块头
    └── <OriginalName>.cpp  # 原模块实现
```

#### 3.2.2 `#include` 改写

主 sketch 与模块之间：

```cpp
// 旧
#include "RcPwmCapture.h"
#include "RcFilter.h"
// 新
#include <mus4_rc.h>   // mus4_rc.h 重导出 RcPwmCapture.h + RcFilter.h
```

`mus4_*.h` 头内：

```cpp
// mus4_rc.h
#pragma once
#include "RcPwmCapture.h"
#include "RcFilter.h"
```

#### 3.2.3 关键问题

1. **跨库类型依赖**（`SharedTypes` / `FirmwareConfig` / `RuntimeState` 被多个库引用）→ 必须建 `mus4_core` 库作为最底层。
2. **当前 `extern` 桥接 + 全局 `static` 变量**（如 `ControlMixer.cpp` 的 `lastCarMode`）需要保留或改造为库内 `static`。
3. **库的 `src/` 不再是 sketch 的 `src/`**——这恰好符合"先分类后解耦"的演进。
4. **每个库需要单独的 `library.properties`**，9 个库 × 1 个文件 = 9 个新元数据文件。
5. **测试断言**：现有 `tests/test_firmware_feature_flags.py` 的 `FIRMWARE_SOURCE_PATHS` 列表需要按新库重新组织。

#### 3.2.4 验证步骤

- 跑 `pytest tests/` 全绿。
- 跑 `python arduino-cli.py -c --sketch MUS4_FW.ino`，编译通过，分区占用与基线一致。
- 实机或串口 smoke test：`STATUS` / `PING` 等命令正常返回。

### 3.3 方案 B：保持平铺（零改动）

接受当前根目录"乱"的事实。优点是零风险；缺点是"问题 1、2"持续存在。

### 3.4 方案 X：先做"语义前缀"再决定物理分组

不动物理位置，但**在文件名前加分组前缀**（让 IDE 侧栏按前缀排序时自然聚类）：

```
Rc_RcPwmCapture.h / .cpp
Rc_RcFilter.h / .cpp
Ctrl_ControlMixer.h / .cpp
Ctrl_SafetyState.h / .cpp
Ctrl_ActuatorOutput.h / .cpp
Wifi_WifiManager.h / .cpp
Web_WebConsoleServer.h / .cpp
Cmd_CommandParser.h / .cpp
...
```

优点：零 build 风险，IDE 侧栏按名称排序时一目了然。  
缺点：rename 成本不低（67 个文件 + 所有引用点），且分组边界后期调整时很痛。

---

## 4. 实施建议

1. **如果根目录"乱"是痛点** → 走方案 A（9 个 Arduino 库）。改动面中、长期收益最高。
2. **如果只是 IDE 侧栏看着烦** → 走方案 X（语义前缀）。改动面小、立即可生效。
3. **如果可以接受现状** → 走方案 B（不动）。

---

## 5. 已否定的方案：原 §3 方案 A `src/` 平铺

> **保留此节作为踩坑记录**。原计划（1.0 版）的核心假设是"arduino-cli 0.18+ 自动编译 `src/` 下的 sketch 源码"，经 2026-06-17 实测证实不成立，**已完整回滚**（67 个 `git mv` 已恢复原位，tests/AGENTS.md/CHANGELOG.md 全部 `git checkout --` 还原，`git status` 干净）。具体实误原因见 §2.2。

### 5.1 原方案 A 设想（已否）

把 67 个 `.h/.cpp` 全部 `git mv` 到 `src/`，根目录仅保留 `MUS4_FW.ino` / `WirelessSecrets.h` / 构建脚本。`#include "X.h"` 零修改，build 脚本零修改。

### 5.2 实测失败表现

```
$ arduino-cli compile --verbose --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs \
    --libraries libraries MUS4_FW.ino
...
-IC:\Dev\DDC\MUS4_FW  ← 仅有项目根，没有 -I src
...\sketch\MUS4_FW.ino.cpp -o nul  ← 只编译 .ino，src/*.cpp 没出现
C:\Dev\DDC\MUS4_FW\MUS4_FW.ino:26:10: fatal error: FirmwareConfig.h: No such file or directory
   26 | #include "FirmwareConfig.h"
```

加 `--build-property "build.extra_flags=-Isrc"` 后：

```
libraries/FastLED/src/FastLED.h:76: ...error: This platform isn't recognized by FastLED... yet.
libraries/FastLED/src/platforms/avr/clockless_trinket.h:6:10: fatal error: avr/interrupt.h: No such file
```

`build.extra_flags` 被全局应用，污染第三方库。

### 5.3 教训

- 在写方案前**必须实际跑一次 `arduino-cli compile` 验证 `src/` 行为**，而不是只看官方文档的"理论支持"。
- 官方文档说"`src/` 编译递归"指的是 `src/` 下的"**库**"（带 `library.properties`），不是普通 sketch 源码。
- 类似踩坑应在 `docs/Plan/MUS4_FW源码目录整理方案.md` 留档，避免后续重复。

---

## 6. 已确认的开放问题（待用户拍板）

1. **是否要继续推进物理整理？** 若要，走方案 A（Arduino 库）或方案 X（语义前缀）。
2. **若选方案 A**：9 个库的划分边界是否需要调整？是否要拆得更细（如 `mus4_command` 拆为 `mus4_parser` / `mus4_dispatcher`）？
3. **若选方案 A**：`mus4_core` 库与 `libraries/` 同级，与 sketch 入口的依赖方向是否要解耦（参考 PlatformIO `libdeps`）？
4. **CHANGELOG 是否需要补一条"否定了 src/ 方案"的踩坑记录**？

---

## 7. 文档与日志

- 选定方案后：
  - 在 `CHANGELOG.md` 增加对应条目。
  - 在 `AGENTS.md` §1.3 "代码组织"段落同步。
  - 在 `tests/test_firmware_feature_flags.py` 的 `FIRMWARE_SOURCE_PATHS` 列表按新位置重写。
  - 在 `README.md` / `README.zh-CN.md` "项目结构"段落同步。
- 不论选哪个方案，本文档 §5 都应作为"踩坑记录"保留。
