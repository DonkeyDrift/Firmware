# MUS4_FW.ino 模块化拆分方案

## 背景

`MUS4_FW.ino` 是 MUS4 固件的主 sketch，当前同时承载 Arduino 生命周期入口、RC PWM 输入、Pilot 串口控制、Wi-Fi/TCP/Web Console、OTA、I2C 传感器、TUI、Buzzer、LED、转向标定、Drift Assist、Park/紧急制动和控制输出等职责。文件体积过大后，任何局部修改都容易牵连安全关键路径，增加审查、测试和回滚成本。

本方案目标是：在保持行为不变的前提下，把 `MUS4_FW.ino` 逐步拆分为职责清晰的 `.h/.cpp` 模块，让主 sketch 最终收敛为“全局装配 + `setup()` + `loop()` 调度层”。

## 设计原则

1. **先测试保护，再移动代码**：每个切片先补充或调整源码断言/策略测试，再做最小机械迁移。
2. **行为不变优先**：拆分阶段不做算法优化、不改协议、不改权限策略、不改 PWM 映射。
3. **低风险优先**：优先迁移纯工具、静态资源、配置、日志桥接，再处理无线/OTA，最后处理 ISR、Park、PWM 输出和控制混控。
4. **保留旧入口名**：迁移后尽量保留原函数名，减少调用点变化。
5. **短期允许 `extern` 桥接**：初期保留主要全局状态在 `MUS4_FW.ino`，模块通过 `extern` 或参数访问；后续再收敛为小状态结构。
6. **安全关键路径最后拆**：ISR、`volatile` 共享变量、Park/紧急制动、PWM 限幅、无线控制入口认证必须在验证充分后再迁移。

## 当前已完成进展

当前仓库已经完成了一批低风险拆分与类型收敛：

- `WebConsoleAssets.h`：承载 Web Console 页面资源和 HTTP OTA 更新页面资源。
- `StringPrint.h`：承载 `Print` 到 `String` 的桥接工具。
- `JsonUtil.h/.cpp`：承载 JSON 字符串追加工具。
- `I2CBusTools.h/.cpp`：承载低层 I2C 读写和探测工具。
- `LedStatus.h/.cpp`：承载 LED 状态输出工具。
- `FirmwareConfig.h`：承载固件配置、引脚、功能开关和常量。
- `Mus4Log.h/.cpp`：承载 MUS4 日志输出桥接。
- `SteeringCalibration.h/.cpp`：承载转向标定状态机、Preferences 读写和转向映射。
- `Sensors.h/.cpp`：承载 INA219/MPU6050 初始化、读取和 I2C 扫描逻辑。
- `GamepadMode.h/.cpp`：承载 BLE Gamepad 模式逻辑。
- `RcFilter.h/.cpp`：承载 RC 滤波、辅助通道稳定和滤波测试入口。
- `CommandParser.h/.cpp`：承载 Pilot 命令解析、范围校验、序列号和校验和处理。
- `CommandDispatcher.h/.cpp`：承载串口/无线命令分发中的通用命令调度。
- `LocalCommands.h/.cpp`：承载本地命令入口 `processLine()`，包括 `ANSI`、`NOANSI`、`FILTER_DEBUG` 和 Pilot 控制命令解析委托。
- `SerialLineReader.h/.cpp`：承载串口行缓冲读取逻辑。
- `SerialBufferTypes.h`：承载串口缓冲区类型。
- `Diagnostics.h/.cpp`：承载诊断、基准、回归和压力测试相关逻辑。
- `DriftAssist.h/.cpp`：承载 Drift Assist 控制计算。
- `SteeringControl.h/.cpp`：承载转向滤波/控制逻辑。
- `SharedTypes.h`：已统一使用 `ControlData`，删除了 `MUS4_FW.ino` 与 `CommandDispatcher.cpp` 中重复的 `struct_message`。

源码断言测试 `tests/test_firmware_feature_flags.py` 已改为聚合读取多个固件源码文件，避免代码从 `MUS4_FW.ino` 迁出后测试误报。

## 推荐最终模块边界

### 1. 固件入口与装配层

保留：`MUS4_FW.ino`

职责：

- `setup()`。
- `loop()`。
- 全局对象装配。
- 模块初始化顺序。
- 主循环调度。

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

此项应放在后期执行，避免一次性改动初始化顺序和主循环时序。

### 2. 配置与共享类型

保留/维护：

- `FirmwareConfig.h`
- `SharedTypes.h`

职责：

- 固件特性宏默认值。
- 引脚、PWM、RC、Wi-Fi、I2C、刷新间隔等配置。
- `SensorData`、`ControlData` 等跨模块共享类型。

约束：

- 可被外部覆盖的宏继续使用 `#ifndef` 包裹。
- 不把所有全局变量塞进 `SharedTypes.h`。
- 不把需要编译期裁剪的宏强行改成 `constexpr`。

### 3. Web Console 与静态资源

已有：`WebConsoleAssets.h`

后续推荐：

- `WebConsoleServer.h/.cpp`：HTTP route、API handler、Web 页面入口。
- `WebTelemetry.h/.cpp`：WebSocket telemetry、曲线数据、ring buffer、采样节流。

约束：

- Web UI raw literal 只做机械搬迁时不得改内容。
- 修改 Web UI 行为前，先在 `tests/test_firmware_feature_flags.py` 增加源码断言。
- API 字段和前端安全门控必须保持兼容。

### 4. Wi-Fi、TCP Console 与 OTA

推荐模块：

- `WifiManager.h/.cpp`：AP/STA、mDNS、DNS captive、Wi-Fi 扫描、STA 配置状态。
- `WifiOta.h/.cpp`：OTA 窗口、TTL、ArduinoOTA callback、HTTP OTA guard。
- `WirelessConsole.h/.cpp`：TCP Console、认证、权限判断和无线命令调度。

约束：

- 无线入口视为不可信边界。
- `AUTH`、Park 锁定、OTA 窗口权限必须与 `wireless_console_policy.py` 保持同步。
- 修改无线权限时必须同步更新 `tests/test_wireless_console_policy.py`。
- OTA 窗口打开或 OTA 传输期间暂停 Serial1 遥测的行为必须保留。

### 5. 传感器与 I2C

已有：

- `I2CBusTools.h/.cpp`
- `Sensors.h/.cpp`

职责：

- INA219/MPU6050 初始化。
- 传感器读取。
- I2C 扫描和探测。
- `SensorData` 更新。

约束：

- 硬件对象优先由外部传入或通过现有全局对象桥接。
- 不在模块内重复创建硬件对象。
- I2C 错误路径保持现有降级行为。

### 6. 命令解析与分发

已有：

- `CommandParser.h/.cpp`
- `CommandDispatcher.h/.cpp`
- `LocalCommands.h/.cpp`
- `SerialLineReader.h/.cpp`
- `SerialBufferTypes.h`

职责：

- Pilot 命令格式解析。
- `Throttle:Steering[:Seq]` 和校验和处理。
- 本地命令处理。
- 通用命令分发。
- 串口行缓冲读取。

后续可继续收敛：

- 将无线命令分发中仍残留在 `MUS4_FW.ino` 的权限判断和命令入口迁入 `WirelessConsole`。
- 将 OTA 维护命令迁入 `WifiOta`。
- 将 Wi-Fi STA 配置命令迁入 `WifiManager` 或独立 `WifiStaConfig` 模块。

### 7. 转向标定、转向控制与 Drift Assist

已有：

- `SteeringCalibration.h/.cpp`
- `SteeringControl.h/.cpp`
- `DriftAssist.h/.cpp`

职责：

- Preferences 中的转向标定参数读写。
- 转向标定交互状态机。
- 转向滤波与控制计算。
- Drift Assist 使能、强度、补偿和平滑衰减。

约束：

- 标定 Preferences namespace/key 不可无测试保护地修改。
- Drift Assist 输出保持 `-100..100` 限幅。
- Park/解锁语义不能被 Drift Assist 绕过。

### 8. RC 输入、ISR 与 PWM 捕获

推荐模块：

- `RcInput.h/.cpp`：非 ISR 的 RC 通道解释、timeout、滤波和模式/Park 通道处理。
- `RcPwmCapture.h/.cpp`：MCPWM capture 初始化与中断捕获。

约束：

- ISR 迁移放最后。
- `IRAM_ATTR` 不能丢失。
- 与 ISR 共享的数据继续使用 `volatile` 或等价保护。
- ISR 内不得引入 `String`、日志、动态分配或非 IRAM 安全调用。
- 硬件验证必须架空车轮或断开动力。

### 9. Park、安全状态机与控制混控

推荐模块：

- `ControlMixer.h/.cpp`：RC/Pilot/mode/Park 混控。
- `ActuatorOutput.h/.cpp`：PWM 输出限幅和 `ledc` 写入。
- `SafetyState.h/.cpp`：Park、紧急制动、失效安全状态机。

此区域是最后阶段，原因是它直接影响舵机、电调和安全状态。

约束：

- PWM 限幅必须保留。
- Pilot/RC 输入越界必须拒绝或夹紧，不能直接影响输出。
- Park 锁定必须覆盖油门输出。
- 失联、无效 PWM、OTA 期间等异常路径必须保持安全默认值。

## 分阶段实施计划

### 阶段 0：测试基线与源码聚合

状态：已基本完成。

内容：

1. 将 `tests/test_firmware_feature_flags.py` 改为聚合读取多个固件源码文件。
2. 保护关键编译开关：`ENABLE_WIFI_CONSOLE`、`ENABLE_WIFI_WEBSOCKET_TELEMETRY` 默认启用。
3. 保护诊断/启动自检开关默认关闭。
4. 保护 ISR、`volatile pwm_value[]`、`volatile rise_time[]`、`volatile last_valid_time[]` 等安全关键结构。
5. 保护 Web Console、OTA、权限策略和日志脱敏行为。

验证：

```powershell
pytest tests/test_firmware_feature_flags.py tests/test_wireless_console_policy.py
```

### 阶段 1：低耦合工具与资源拆分

状态：已完成主要切片。

内容：

1. 迁移 Web Console HTML/OTA HTML 到 `WebConsoleAssets.h`。
2. 迁移 `StringPrint`。
3. 迁移 JSON 工具。
4. 迁移低层 I2C 工具。
5. 迁移 LED 状态工具。
6. 迁移日志桥接。

验证：

```powershell
pytest tests/test_firmware_feature_flags.py tests/test_wireless_console_policy.py
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

### 阶段 2：命令、诊断、传感器和标定拆分

状态：已完成大部分切片。

内容：

1. 迁移 Pilot 命令解析到 `CommandParser`。
2. 迁移命令分发到 `CommandDispatcher`。
3. 迁移本地命令到 `LocalCommands`。
4. 迁移串口行读取到 `SerialLineReader`。
5. 迁移诊断、基准、回归和压力测试到 `Diagnostics`。
6. 迁移传感器逻辑到 `Sensors`。
7. 迁移转向标定到 `SteeringCalibration`。
8. 统一控制数据类型为 `ControlData`。

验证：

```powershell
pytest tests/
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

### 阶段 3：无线/Web/OTA 拆分

状态：待继续。

推荐顺序：

1. 抽出 Web route 注册和页面 handler 到 `WebConsoleServer.h/.cpp`。
2. 抽出 WebSocket telemetry 与曲线数据到 `WebTelemetry.h/.cpp`。
3. 抽出 OTA 窗口、TTL、HTTP OTA guard 和 ArduinoOTA callback 到 `WifiOta.h/.cpp`。
4. 抽出 AP/STA、mDNS、DNS captive 和 Wi-Fi 扫描到 `WifiManager.h/.cpp`。
5. 抽出 TCP Console 认证、权限判断和无线命令分发到 `WirelessConsole.h/.cpp`。

每个切片都必须同步检查：

- `wireless_console_policy.py`
- `tests/test_wireless_console_policy.py`
- `tests/test_firmware_feature_flags.py`

额外手工验证：

- AP 可见。
- STA 状态可显示。
- Web Console 可打开。
- `/api/status` 字段完整。
- 未认证控制命令被拒绝。
- Park unlocked 时 OTA 被拒绝。
- Park locked + AUTH 后 OTA 可打开。
- OTA 传输期间 Serial1 telemetry 暂停。

### 阶段 4：RC 输入与控制辅助拆分

状态：待继续。

推荐顺序：

1. 将非 ISR 的 RC 通道解释和 timeout 逻辑迁入 `RcInput.h/.cpp`。
2. 将 MCPWM capture 初始化迁入 `RcPwmCapture.h/.cpp`，暂不急于迁移 ISR。
3. 在测试保护充分后迁移 ISR。
4. 收敛 Drift Assist、转向控制和 RC 输入之间的数据依赖。

验证：

```powershell
pytest tests/
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

硬件验证必须断开动力或架空车轮。

### 阶段 5：Park、PWM 输出和控制混控拆分

状态：最后执行。

推荐顺序：

1. 抽出 Park/紧急制动状态机。
2. 抽出控制混控：手动、半自动、全自动。
3. 抽出 PWM 输出限幅和 `ledc` 写入。
4. 最后考虑引入 `FirmwareApp`，让 `MUS4_FW.ino` 只保留生命周期入口。

验证：

```powershell
pytest tests/
.\arduino-cli-wsl.ps1 -Compile -CheckPartition -Sketch MUS4_FW.ino
```

实机验证必须覆盖：

- Park locked 默认安全输出。
- Park unlocked 后油门响应正常。
- RC 失联回到安全状态。
- Pilot 输入越界不会绕过限幅。
- OTA 窗口/传输期间不会产生非预期输出。

## 全局状态收敛策略

### 短期

- 保留 `TUI`、`Buzzer`、传感器对象、Web server、Wi-Fi 状态、`ControlData`、`SensorData`、RC `volatile` 数组等主要全局对象在 `MUS4_FW.ino`。
- 新模块通过参数或 `extern` 访问现有状态。
- 每次迁移只移动一个责任域。

### 中期

引入小型状态结构，而不是一次性创建巨大上下文：

- `RcState`
- `PilotState`
- `WifiRuntimeState`
- `OtaState`
- `SteeringControlState`
- `DriftAssistState`
- `SafetyState`

### 后期

在模块边界稳定后，再考虑 `FirmwareApp` 装配层：

- `begin()`：集中初始化模块。
- `update()`：集中调度循环任务。
- `.ino` 只保留 Arduino 标准入口。

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

涉及无线/Web/OTA 的切片增加手工验证；涉及 RC/ISR/Park/PWM 输出的切片增加断动力或架空车轮硬件验证。

## 风险与回滚策略

1. **单责任域单提交**：每次只拆一个模块或一个明确切片。
2. **失败即停止**：测试、编译或 OTA 任一失败，不继续拆下一个模块。
3. **保留旧函数名 wrapper**：减少调用点变化，便于回滚。
4. **高风险区域最后拆**：无线权限、OTA guard、ISR、Park、PWM 输出、控制混控不得提前大改。
5. **提交前检查敏感文件**：`WirelessSecrets.h` 可能包含真实凭据，通常不应纳入提交。
6. **不自动 push**：远端操作必须由用户明确授权。

## 下一步建议

当前低风险工具、命令、诊断、传感器和类型收敛已经完成较多。下一阶段建议进入无线/Web/OTA 拆分，但仍保持小步推进：

1. 先只抽 `WebConsoleServer` 中最薄的 route 注册或页面 handler。
2. 保持 handler 内部仍调用现有函数，避免同时重写权限逻辑。
3. 为迁移目标补充源码断言。
4. 运行完整 pytest、WSL 编译和 HTTP OTA。

如果继续降低风险，也可以先清理 `MUS4_FW.ino` 中确认未使用的旧 TUI 残留变量或 cursor helper，但删除前必须用源码搜索和编译验证确认没有调用点。
