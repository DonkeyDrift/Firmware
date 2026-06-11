# MUS4_FW.ino 模块化拆分方案

## 优化意见与状态更新（3.0 修订稿）

> 本文档随 `v1.7.3` 迭代持续更新。当前项目已实质完成**全部计划内切片**（阶段 0～阶段 5）。`MUS4_FW.ino` 从原 `~3700` 行收敛到 **`~556` 行**，新增并落地了 `RcPwmCapture.h/.cpp`、`ControlMixer.h/.cpp`、`SafetyState.h/.cpp`、`ActuatorOutput.h/.cpp` 等安全关键模块。所有 pytest 断言（71 项）与 WSL 编译均保持通过，分区占用稳定在 **69.5%**。

### 1. 当前状态：全部安全关键切片已完成

`MUS4_FW.ino` 当前仅保留：

- `setup()` / `loop()` Arduino 生命周期入口
- 全局对象装配（`TUI`、`Buzzer`、传感器硬件对象、`wifiRuntime`/`otaRuntime` 等）
- RC 滤波与数据准备（`pwm_filtered`、超时检测、`rc_data` 更新）
- TUI 渲染调用与性能降级检测
- 任务调度（传感器读取 → 串口读取 → Wi-Fi 更新 → RC 滤波 → 安全状态 → 控制融合 → 执行器输出 → TUI）

**已完成迁移的模块清单（新增于本轮）**：

| 模块 | 职责 | 迁出代码量 |
|------|------|-----------|
| `RcPwmCapture.h/.cpp` | RC PWM 输入捕获：中断处理、MCPWM、脉冲验证、引脚初始化 | ~140 行 |
| `ControlMixer.h/.cpp` | 驾驶模式切换、RC/Pilot 控制融合、Drift Assist、LED 反馈 | ~80 行 |
| `SafetyState.h/.cpp` | Park 状态机、紧急制动 FSM | ~110 行 |
| `ActuatorOutput.h/.cpp` | PWM 执行器输出：舵机/电调映射、限幅、`ledcWriteChannel` | ~50 行 |

**清理的死代码**：`rise_time[]`（未使用）、`lastParkState`（未使用）、`adj()`（未使用）、`MOTOR_OFFSET_V`/`SERVO_OFFSET_V`（未使用）、`throttleWave[]`/`steeringWave[]`/`waveIndex`（未使用）、`counter`（未使用）。

### 2. 模块边界状态总览（3.0 版）

| 区域 | 状态 | 说明 |
|------|------|------|
| **Web HTTP / Route** | ✅ 已完成 | `WebConsoleServer.h/.cpp` 承载 HTTP route、API handler、captive portal、OTA upload。 |
| **WebSocket Telemetry** | ✅ 已完成 | `WebTelemetry.h/.cpp` 承载 ring buffer、数据采样、`pushWifiWebSocketData()`。 |
| **Wi-Fi Runtime 状态机** | ✅ 已完成 | `WifiManager.h/.cpp` 承载 AP/STA handoff、mDNS、DNS captive、TCP Console。全局状态收敛到 `WifiRuntimeState` / `OtaRuntimeState`。 |
| **TCP Console** | ✅ 已完成 | `updateWifiConsole()` 已迁入 `WifiManager.cpp`。 |
| **RC 输入** | ✅ 已完成 | `RcPwmCapture.h/.cpp` 承载 ISR、MCPWM capture、引脚初始化。`acceptRcPulse()` 脉冲验证已迁入。 |
| **控制融合** | ✅ 已完成 | `ControlMixer.h/.cpp` 承载 `mode_change()`、三大模式分支、`updateControlOutput()`。 |
| **安全状态机** | ✅ 已完成 | `SafetyState.h/.cpp` 承载 `park_change()`、`emergencyStop()`、`EmergencyStopState` 枚举。 |
| **执行器输出** | ✅ 已完成 | `ActuatorOutput.h/.cpp` 承载 PWM 常量、`setupActuatorOutput()`、`updateActuatorOutput()`。`SERVO_MID_V`/`SERVO_RANGE_V` 保持外部链接供 `Diagnostics.cpp` 引用。 |
| **FirmwareApp 装配层** | ⏳ 尚未引入 | 当前初始化顺序仍有隐式依赖，建议在模块边界彻底稳定后再封装 `app.begin()` / `app.update()`。 |

### 3. 全局状态收敛成果

本轮切片前的全局状态收敛基础（`WifiRuntimeState` / `OtaRuntimeState`）已支撑了无线域全部模块。本轮新增切片继续沿用 `extern` 桥接策略，但将原本散落在 `.ino` 中的安全关键状态（`emergencyStopState`、`parkBtnPressed`、`lastCarMode`、`carOutputModeLast` 等）分别收拢到各自模块的 `static` 变量中：

- `SafetyState.cpp`：`emergencyStopState`、`parkBtnPressed`、`parkActionTaken` 等
- `ControlMixer.cpp`：`lastCarMode`、`carOutputModeLast`
- `RcPwmCapture.cpp`：`candidate_pwm`、`large_change_count`、`last_large_pwm`（原为 `acceptRcPulse` 的 static 局部变量）

**收益**：`MUS4_FW.ino` 中的全局变量声明从数十个减少到仅剩硬件对象、跨模块数据（`ControlData`、传感器数据、滤波状态）和时序标记。

### 4. `loop()` 调度现状

当前 `loop()` 仍通过多个 `if (millis() - lastX >= intervalX)` 分支驱动任务。虽然各任务的业务实现已迁出，但调度代码仍留在 `.ino` 中。未来引入 `FirmwareApp` 时，可将此调度表化：

```cpp
struct Task {
    const char* name;
    uint32_t intervalMs;
    uint32_t lastRunMs;
    void (*callback)();
    bool enabled;
};
```

> **注意**：这不是为了引入复杂 RTOS，而是把时序策略从业务代码中剥离。应在 `FirmwareApp` 阶段再做，避免当前改动面过大。

---

## 背景

`MUS4_FW.ino` 是 MUS4 固件的主 sketch，原同时承载 Arduino 生命周期入口、RC PWM 输入、Pilot 串口控制、Wi-Fi/TCP/Web Console、OTA、I2C 传感器、TUI、Buzzer、LED、转向标定、Drift Assist、Park/紧急制动和控制输出等职责。通过本轮及前几轮拆分，主 sketch 已从 ~3700 行收敛到 ~556 行，职责清晰化为"全局装配 + `setup()` + `loop()` 调度层"。

## 设计原则

1. **先测试保护，再移动代码**：每个切片先补充或调整源码断言/策略测试，再做最小机械迁移。
2. **行为不变优先**：拆分阶段不做算法优化、不改协议、不改权限策略、不改 PWM 映射。
3. **低风险优先**：优先迁移纯工具、静态资源、配置、日志桥接，再处理无线/OTA，最后处理 ISR、Park、PWM 输出和控制混控。
4. **保留旧入口名**：迁移后尽量保留原函数名，减少调用点变化。
5. **短期允许 `extern` 桥接**：初期保留主要全局状态在 `MUS4_FW.ino`，模块通过 `extern` 或参数访问；后续再收敛为小状态结构。
6. **安全关键路径最后拆**：ISR、`volatile` 共享变量、Park/紧急制动、PWM 限幅、无线控制入口认证必须在验证充分后再迁移。

## 当前已完成进展

### 早期已完成（v1.5.x ~ v1.6.x）

- `WebConsoleAssets.h`：Web Console 页面资源和 HTTP OTA 更新页面资源。
- `StringPrint.h`：`Print` 到 `String` 的桥接工具。
- `JsonUtil.h/.cpp`：JSON 字符串追加工具。
- `I2CBusTools.h/.cpp`：低层 I2C 读写和探测工具。
- `LedStatus.h/.cpp`：LED 状态输出工具。
- `FirmwareConfig.h`：固件配置、引脚、功能开关和常量。
- `Mus4Log.h/.cpp`：MUS4 日志输出桥接。
- `SteeringCalibration.h/.cpp`：转向标定状态机、Preferences 读写和转向映射。
- `Sensors.h/.cpp`：INA219/MPU6050 初始化、读取和 I2C 扫描逻辑。
- `GamepadMode.h/.cpp`：BLE Gamepad 模式逻辑。
- `RcFilter.h/.cpp`：RC 滤波、辅助通道稳定和滤波测试入口。
- `CommandParser.h/.cpp`：Pilot 命令解析、范围校验、序列号和校验和处理。
- `CommandDispatcher.h/.cpp`：串口/无线命令分发中的通用命令调度。
- `LocalCommands.h/.cpp`：本地命令入口 `processLine()`。
- `SerialLineReader.h/.cpp`：串口行缓冲读取逻辑。
- `SerialBufferTypes.h`：串口缓冲区类型。
- `Diagnostics.h/.cpp`：诊断、基准、回归和压力测试相关逻辑。
- `DriftAssist.h/.cpp`：Drift Assist 控制计算。
- `SteeringControl.h/.cpp`：转向滤波/控制逻辑。
- `WirelessConsole.h/.cpp`：无线命令分类、权限判断、Park 锁定判断、OTA 命令分类和无线日志脱敏。
- `WifiStaConfig.h/.cpp`：STA 命令入口、状态输出、凭据复制校验、STA IP 文本、错误状态、延迟应用调度、Preferences 保存/加载/清除。
- `WifiIdentity.h/.cpp`：AP SSID/mDNS hostname 校验、AP SSID 复制和 mDNS host/url 文本生成。
- `WifiOta.h/.cpp`：OTA 窗口管理、ArduinoOTA callback、HTTP OTA guard、OTA 状态输出和维护命令入口。
- `SharedTypes.h`：统一使用 `ControlData`，删除重复 `struct_message`。
- `RuntimeState.h`：`WifiRuntimeState` / `OtaRuntimeState` 聚合状态结构体，替代 15+ 个散列全局变量。

### 近期已完成（v1.7.3，切片 1～8）

- `WebLogBuffer.h/.cpp`：Web 日志 ring buffer 和 `appendWebLog` 桥接。
- `WebConsoleServer.h/.cpp`：HTTP route、API handler、captive portal、OTA upload、`printWirelessStatus()`。
- `WebTelemetry.h/.cpp`：WebSocket telemetry 服务器、事件处理、`pushWifiWebSocketData()`、数据采样推送。
- `WifiManager.h/.cpp`：Wi-Fi runtime 状态机，包括 AP/STA 启停、mDNS 生命周期、DNS captive、STA handoff、TCP Console 生命周期。
- **`RcPwmCapture.h/.cpp`**（切片 5）：RC PWM 输入捕获。中断处理、脉冲验证、MCPWM capture、引脚初始化。
- **`ControlMixer.h/.cpp`**（切片 6）：驾驶模式切换、`mode_change()`、三大模式融合分支、`updateControlOutput()`、Drift Assist 应用、LED 状态反馈。
- **`SafetyState.h/.cpp`**（切片 7）：Park 状态机 `park_change()`、紧急制动 FSM `emergencyStop()`、相关枚举与状态变量。
- **`ActuatorOutput.h/.cpp`**（切片 8）：PWM 执行器输出常量、`setupActuatorOutput()`、`updateActuatorOutput()`、舵机/电调限幅与驱动。

源码断言测试 `tests/test_firmware_feature_flags.py`（71 项）已持续更新，覆盖所有新增模块。每次切片按"先断言、再迁移、再 `pytest tests/`、WSL 编译"的闭环验证。

## 推荐最终模块边界

### 1. 固件入口与装配层

保留：`MUS4_FW.ino`

当前职责（已大幅收敛）：

- `setup()`：模块初始化顺序。
- `loop()`：任务调度（传感器 → 串口 → Wi-Fi → RC 滤波 → 安全状态 → 控制融合 → 执行器输出 → TUI）。
- 全局对象装配（`TUI`、`Buzzer`、传感器、Wi-Fi runtime 等）。
- RC 滤波与数据准备（`pwm_filtered`、超时检测、`rc_data` 更新）。
- TUI 渲染调用与性能降级检测。

后期可选新增：`FirmwareApp.h/.cpp`

目标形态：

```cpp
void setup()
{
    app.begin();
}

void loop()
{
    app.update();
}
```

> **建议时机**：当前各模块边界已稳定，但 `loop()` 中的调度代码仍与业务时序紧密耦合。建议在充分实机验证后再引入 `FirmwareApp`，避免掩盖初始化顺序问题。

### 2. 配置与共享类型

保留/维护：

- `FirmwareConfig.h`
- `SharedTypes.h`

职责：固件特性宏默认值、引脚/PWM/RC/Wi-Fi/I2C/刷新间隔等配置、`SensorData`、`ControlData` 等跨模块共享类型。

### 3. Web Console 与静态资源

已有：

- `WebConsoleAssets.h`
- `WebConsoleServer.h/.cpp`
- `WebTelemetry.h/.cpp`
- `WebLogBuffer.h/.cpp`

### 4. Wi-Fi、TCP Console 与 OTA

已有：

- `WirelessConsole.h/.cpp`
- `WifiStaConfig.h/.cpp`
- `WifiIdentity.h/.cpp`
- `WifiOta.h/.cpp`
- `WifiManager.h/.cpp`
- `RuntimeState.h`

### 5. 传感器与 I2C

已有：

- `I2CBusTools.h/.cpp`
- `Sensors.h/.cpp`

### 6. 命令解析与分发

已有：

- `CommandParser.h/.cpp`
- `CommandDispatcher.h/.cpp`
- `LocalCommands.h/.cpp`
- `SerialLineReader.h/.cpp`
- `SerialBufferTypes.h`

### 7. 转向标定、转向控制与 Drift Assist

已有：

- `SteeringCalibration.h/.cpp`
- `SteeringControl.h/.cpp`
- `DriftAssist.h/.cpp`

### 8. RC 输入、ISR 与 PWM 捕获

已有：

- `RcFilter.h/.cpp`
- `RcPwmCapture.h/.cpp`

约束：ISR 已迁移完成；`IRAM_ATTR`、`volatile` 保护均保留；`setupRcPwmCapture()` 统一初始化。

### 9. 安全状态机、控制混控与执行器输出

已有：

- `SafetyState.h/.cpp`：Park 状态机、紧急制动 FSM。
- `ControlMixer.h/.cpp`：RC/Pilot/mode/Park 混控、Drift Assist。
- `ActuatorOutput.h/.cpp`：PWM 输出限幅和 `ledc` 写入。

约束：PWM 限幅保留；Park 锁定覆盖油门；失联/无效 PWM/OTA 期间安全默认值保留。

---

## 分阶段实施计划（已完结）

### 阶段 0：测试基线与源码聚合

状态：**已完成**。`tests/test_firmware_feature_flags.py` 已改为聚合读取多个固件源码文件，保护关键编译开关、ISR 结构、Web Console/OTA/权限策略。

### 阶段 1：低耦合工具与资源拆分

状态：**已完成**。`WebConsoleAssets.h`、`StringPrint`、`JsonUtil`、`I2CBusTools`、`LedStatus`、`Mus4Log` 等已迁出。

### 阶段 2：命令、诊断、传感器和标定拆分

状态：**已完成**。`CommandParser`、`CommandDispatcher`、`LocalCommands`、`SerialLineReader`、`Diagnostics`、`Sensors`、`SteeringCalibration`、`SharedTypes` 已迁出。

### 阶段 3：无线/Web/OTA 拆分

状态：**已完成**。`WirelessConsole`、`WifiStaConfig`、`WifiIdentity`、`WifiOta`、`RuntimeState`、`WebLogBuffer`、`WebConsoleServer`、`WebTelemetry`、`WifiManager` 已迁出。

### 阶段 4：RC 输入与控制辅助拆分

状态：**已完成**。

- `RcPwmCapture.h/.cpp`：ISR、MCPWM capture、引脚初始化、脉冲验证已迁出。
- `RcFilter.h/.cpp`：滤波逻辑此前已完成。

### 阶段 5：Park、PWM 输出和控制混控拆分

状态：**已完成**。

- `SafetyState.h/.cpp`：Park 状态机、紧急制动 FSM 已迁出。
- `ControlMixer.h/.cpp`：模式切换、控制融合、Drift Assist 已迁出。
- `ActuatorOutput.h/.cpp`：PWM 映射、限幅、`ledcWriteChannel` 已迁出。

---

## 全局状态收敛策略

### 短期（已完成）

- Wi-Fi / OTA runtime 状态已收敛到 `WifiRuntimeState` / `OtaRuntimeState`。
- 安全关键状态（`emergencyStopState`、`parkBtnPressed`、`lastCarMode`、`carOutputModeLast`）已收拢到各自模块的 `static` 变量中。
- 保留 `TUI`、`Buzzer`、传感器对象、`ControlData`、`SensorData`、RC `volatile` 数组等必要全局对象在 `MUS4_FW.ino`。

### 中期

可考虑进一步引入小型状态结构：

- `RcState`（可选）：将 `pwm_filter_buf`、`pwm_filter_idx` 等滤波状态从 `.ino` 收拢。
- `SteeringControlState`（可选）：当前已在 `SteeringControl.cpp` 中通过 `static` 管理。
- `DriftAssistState`（可选）：当前已在 `DriftAssist.cpp` 中管理。

### 后期

在模块边界稳定后，再考虑 `FirmwareApp` 装配层：

- `begin()`：集中初始化模块。
- `update()`：集中调度循环任务。
- `.ino` 只保留 Arduino 标准入口。

---

## 测试与验证矩阵

每个小切片至少运行：

```powershell
pytest tests/test_firmware_feature_flags.py tests/test_wireless_console_policy.py
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

每个阶段结束运行：

```powershell
pytest tests/
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

编译成功后，按当前项目约定自动 HTTP OTA 到测试主板：

```powershell
.\arduino-cli-wsl.ps1 -Upload -HttpOta -HttpOtaHost 192.168.3.157 -Sketch MUS4_FW.ino
```

---

## 风险与回滚策略

1. **单责任域单提交**：每次只拆一个模块或一个明确切片。
2. **失败即停止**：测试、编译或 OTA 任一失败，不继续拆下一个模块。
3. **保留旧函数名 wrapper**：减少调用点变化，便于回滚。
4. **高风险区域最后拆**：无线权限、OTA guard、ISR、Park、PWM 输出、控制混控已全部完成。
5. **提交前检查敏感文件**：`WirelessSecrets.h` 可能包含真实凭据，通常不应纳入提交。
6. **不自动 push**：远端操作必须由用户明确授权。

---

## 下一步建议（基于当前 `v1.7.3` 完结状态）

**全部计划内切片已完成**。`MUS4_FW.ino` 已从 ~3700 行收敛到 **~556 行**，安全关键路径（RC ISR、Park、控制混控、PWM 输出）均已迁出并通过编译/测试验证。后续工作从"继续拆分"转向"收敛、稳定与可选装配层"：

1. **`FirmwareApp` 装配层（可选，低优先级）**
   - 当 `loop()` 中只剩调度代码、各模块边界彻底稳定后，再封装 `FirmwareApp.begin()` / `FirmwareApp.update()`。
   - 不要提前做：当前初始化顺序仍有隐式依赖，提前封装会掩盖问题。

2. **剩余全局变量收拢（可选）**
   - RC 滤波状态（`pwm_filter_buf`、`pwm_filter_idx`、`pwm_filter_initialized` 等）仍留在 `.ino` 中，可考虑迁入 `RcFilter.cpp` 作为 `static`。
   - TUI 相关时序变量（`uiIntervalCurrent`、`lastUICycleDuration` 等）属于调度层，可保留在 `.ino`。

3. **文档与测试同步**
   - 保持 `tests/test_firmware_feature_flags.py` 的源码断言与当前代码一致。
   - 保持 `AGENTS.md` 的代码组织图和模块清单最新。
   - 保持 `wireless_console_policy.py` 与 `WirelessConsole.cpp` 的权限策略一致。

4. **实机验证建议**
   - Park locked 默认安全输出。
   - Park unlocked 后油门响应正常。
   - RC 失联回到安全状态。
   - Pilot 输入越界不会绕过限幅。
   - OTA 窗口/传输期间不会产生非预期输出。
