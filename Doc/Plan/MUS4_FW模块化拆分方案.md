# MUS4_FW.ino 模块化拆分方案

## 优化意见与状态更新（修订稿）

> 本文档初稿撰写时拆分尚在进行中，当前项目（`v1.7.3`）已实质完成阶段 0～阶段 3 的低风险切片。`MUS4_FW.ino` 从原 `~3700` 行收敛到 `~2270` 行，新增并落地了 `WifiOta.h/.cpp`、`WifiIdentity.h/.cpp`、`WirelessConsole.h/.cpp`、`WifiStaConfig.h/.cpp` 等无线域模块。原方案中“下一阶段继续拆 `WifiStaConfig` 剩余 helper”的建议大多已落地，继续沿用旧阶段划分会造成计划与代码脱节。以下优化意见基于当前实际代码状态提出，核心思路从**“继续拆分”**转向**“收敛全局状态、收紧模块边界、降低 `.ino` 中的堆积”**。

### 1. 当前主要问题不再是模块数量，而是全局状态未收敛

当前代码已拆出 20 余对 `.h/.cpp`，但 `MUS4_FW.ino` 中仍有大量全局对象与 `static` helper：

- Wi-Fi runtime 状态（`wifiStaConnected`、`wifiOtaWindowOpen`、`wifiStaHandoffActive` 等约 15 个布尔/字符串变量）。
- Web server / WebSocket / route handler（约 650 行，覆盖 root、API、 captive portal、OTA upload、plot data）。
- TCP Console server / client 生命周期（`setupWifiConsole()`、`updateWifiConsole()` 等）。
- STA handoff、AP 重启、mDNS 启动/停止等状态机。
- RC PWM 捕获：MCPWM capture 初始化和双模式中断（传统 `attachInterrupt` + `mcpwm` capture）。
- 顶层混控、Park、紧急制动以及 `User_throttle`、`Pilot_throttle` 等输入缓冲。

这些代码仍通过 `extern` 或参数在模块间传递，形成隐式依赖网。下一步的优化重点应当是**把散落的 runtime 状态收进小型结构体**，而不是继续新增更多只包含 helper 的切片。

### 2. 建议调整模块边界（2.0 版）

| 区域 | 当前状态 | 优化方向 |
|------|----------|----------|
| **Web HTTP / Route** | 全部在 `MUS4_FW.ino` 中，约 650 行 | 抽出 `WebConsoleServer.h/.cpp`，统一注册 route、处理 captive portal、API 响应头和页面入口。不要把 WebSocket 混进去。 |
| **WebSocket Telemetry** | 全部在 `MUS4_FW.ino` 中 | 抽出 `WebTelemetry.h/.cpp`，ring buffer、数据点采样、`pushWifiWebSocketData()` 全部迁出。 |
| **Wi-Fi Runtime 状态机** | AP/STA handoff、mDNS、DNS captive、扫描分散在 `.ino` | 抽出 `WifiManager.h/.cpp`，聚合 AP 启停、STA handoff、扫描调度、mDNS 生命周期。`WifiStaConfig` 只保留配置读写与命令入口，不持有连接状态机。 |
| **TCP Console** | `setupWifiConsole()` / `updateWifiConsole()` 在 `.ino` | 并入 `WirelessConsole.h/.cpp` 或新建 `TcpConsole.h/.cpp`，与 Web Console 共用认证会话抽象。 |
| **RC 输入** | ISR + MCPWM capture + 通道解释全在 `.ino` | 非 ISR 部分迁入 `RcInput.h/.cpp`；MCPWM 与传统 ISR 迁入 `RcPwmCapture.h/.cpp`，并在 `setup()` 中只暴露 `rcPwmCaptureBegin()`。 |
| **安全与混控** | Park、紧急制动、模式切换、混控、PWM 写入仍在 `.ino` | 这是最后的安全关键切片，建议先引入 `ControlContext` 结构体（包含 `ControlData`、模式、Park 状态、紧急制动状态），再抽出 `SafetyState.h/.cpp` 和 `ControlMixer.h/.cpp`。 |
| **FirmwareApp 装配层** | 尚未引入 | 在 `loop()` 任务调度清晰后，再引入 `FirmwareApp.h/.cpp`，让 `.ino` 只剩 `app.begin()` / `app.update()`。不要提前做，否则会掩盖初始化顺序问题。 |

### 3. 全局状态收敛是最高优先级优化

原方案允许“短期 `extern` 桥接”，这个短期已经过长。建议引入以下小型结构体，将 `MUS4_FW.ino` 中的全局变量逐步迁入：

```cpp
struct WifiRuntimeState {
    bool apAvailable;
    bool staConfigured;
    bool staConnected;
    bool staConnecting;
    bool staTimedOut;
    bool staApplyPending;
    bool apRestartPending;
    bool mdnsStarted;
    bool handoffActive;
    String currentApSsid;
    String staTargetSsid;
};

struct OtaRuntimeState {
    bool started;
    bool windowOpen;
    bool inProgress;
    bool parkGuardActive;
    unsigned long windowOpenedAt;
};

struct RcRuntimeState {
    volatile uint32_t pwmValues[RC_CHANNEL_COUNT];
    volatile uint32_t riseTimes[RC_CHANNEL_COUNT];
    volatile uint32_t lastValidTimes[RC_CHANNEL_COUNT];
    int userThrottle;
    int userSteering;
    bool filterInitialized;
};
```

**优化收益**：
- 消除大量 `extern bool xxx`；模块接口从“读/写全局变量”变成“传结构体指针/引用”。
- 方便单元测试：可以在宿主环境中实例化这些结构体并注入模块。
- 减少 `.ino` 中的声明噪音。

### 4. `loop()` 调度表化

当前 `loop()` 中通过多个 `if (millis() - lastX >= intervalX)` 分支驱动任务。建议优化为任务调度表：

```cpp
struct Task {
    const char* name;
    uint32_t intervalMs;
    uint32_t lastRunMs;
    void (*callback)();
    bool enabled;
};
```

或者维护一个 `TaskScheduler` 小型类。**这不是为了引入复杂 RTOS，而是为了把时序策略从业务代码中剥离**，使 `loop()` 从“业务实现”降级为“调度器”。

### 5. 同步更新测试断言与文档

优化意见不仅是代码层面的。每次收敛全局状态、迁出 Web handler 或调整模块边界时，必须：

1. 在 `tests/test_firmware_feature_flags.py` 中增加源码断言，确认目标函数/变量已从 `MUS4_FW.ino` 迁出。
2. 在 `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py` 中保持权限策略同步。
3. 更新本文档的“当前已完成进展”和“下一步建议”，避免计划与实际状态再次脱节。
4. 更新 `AGENTS.md` 的代码组织图，确保新加入的模块一目了然。

---

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
- `WirelessConsole.h/.cpp`：承载无线命令分类、权限判断、Park 锁定判断、OTA 命令分类和无线日志脱敏 helper。
- `WifiStaConfig.h/.cpp`：承载 Wi-Fi STA 命令入口、状态输出、SSID/密码复制校验、STA IP 文本、错误状态记录、延迟应用调度、STA SSID/密码持久化保存 helper，以及 STA 配置加载/清除/运行态清理 helper。
- `WifiIdentity.h/.cpp`：承载 AP SSID/mDNS hostname 校验、AP SSID 复制、mDNS host/url 文本生成 helper。
- `WifiOta.h/.cpp`：承载 OTA 窗口管理、ArduinoOTA callback、HTTP OTA guard、OTA 状态输出和维护命令入口。
- `SharedTypes.h`：已统一使用 `ControlData`，删除了 `MUS4_FW.ino` 与 `CommandDispatcher.cpp` 中重复的 `struct_message`。

源码断言测试 `tests/test_firmware_feature_flags.py` 已改为聚合读取多个固件源码文件，避免代码从 `MUS4_FW.ino` 迁出后测试误报。近期无线相关切片均按“先断言、再迁移、再 `pytest tests/`、WSL 编译、HTTP OTA”的闭环验证。

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

已有/推荐模块：

- `WirelessConsole.h/.cpp`：已承载无线命令分类、权限判断、Park 锁定判断、OTA 命令分类和无线日志脱敏 helper；后续可继续迁入 TCP Console 认证会话和无线命令主分发。
- `WifiStaConfig.h/.cpp`：已承载 STA 命令入口、状态输出、凭据复制校验、IP 文本、错误状态、延迟应用调度、STA Preferences 保存/加载/清除 helper。
- `WifiIdentity.h/.cpp`：已承载 AP SSID/mDNS hostname 校验、AP SSID 复制和 mDNS host/url 文本生成。
- `WifiOta.h/.cpp`：已承载 OTA 窗口、TTL、ArduinoOTA callback、HTTP OTA guard、OTA 状态输出和维护命令入口。
- `WifiManager.h/.cpp`：**后续重点新增模块**，承载 AP/STA 启停、mDNS 生命周期、DNS captive、Wi-Fi 扫描、STA handoff 和运行态状态机；应当将 `MUS4_FW.ino` 中约 300 行的 Wi-Fi runtime 状态迁出。
- `WebConsoleServer.h/.cpp`：后续承载 HTTP route、API handler、Web 页面入口。
- `WebTelemetry.h/.cpp`：后续承载 WebSocket telemetry、曲线数据、ring buffer、采样节流。

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

- 无线命令分类、权限判断、STA 配置命令入口和 OTA 命令分类已迁入 `WirelessConsole` / `WifiStaConfig` / `WifiOta`。
- 将无线命令主分发 `processWirelessConsoleLine()` 迁入 `WirelessConsole`，但不要同时改认证、Park guard 或命令响应文本。
- 将 TCP Console 生命周期（server setup/update）迁入 `WirelessConsole` 或独立 `TcpConsole`。
- 将 AP/STA handoff、连接状态更新迁入后续 `WifiManager`，而不是继续堆在 `WifiStaConfig` 中。

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

状态：已完成低风险 helper 切片；**剩余高风险/大体积切片亟待按优化意见推进**。

已完成：

1. `WirelessConsole.h/.cpp`：无线命令分类、权限判断、Park 锁定判断、OTA 命令分类、日志脱敏。
2. `WifiStaConfig.h/.cpp`：STA 命令入口、状态输出、SSID/密码复制校验、STA IP 文本、错误状态记录、延迟应用调度、SSID/密码 Preferences 保存/加载/清除。
3. `WifiIdentity.h/.cpp`：AP SSID/mDNS hostname 校验、AP SSID 复制、mDNS host/url 文本生成。
4. `WifiOta.h/.cpp`：OTA 窗口、TTL、ArduinoOTA callback、HTTP OTA guard、OTA 状态输出和维护命令入口。

后续推荐顺序（已按当前状态调整）：

1. **先收敛全局状态**：引入 `WifiRuntimeState` / `OtaRuntimeState` 结构体，替代 `MUS4_FW.ino` 中分散的 15+ 个 Wi-Fi/Ota 全局变量。这是最高优先级优化，不引入新模块也能显著降低 `.ino` 复杂度。
2. 抽出 AP/STA 启停、mDNS 生命周期、DNS captive、Wi-Fi 扫描和 STA handoff 到 `WifiManager.h/.cpp`。让 `WifiStaConfig` 回归“配置读写 + 命令入口”本职，不再持有连接状态机。
3. 抽出 Web route 注册和页面 handler 到 `WebConsoleServer.h/.cpp`。
4. 抽出 WebSocket telemetry 与曲线数据到 `WebTelemetry.h/.cpp`。
5. 最后迁移 TCP Console 认证会话和无线命令主分发 `processWirelessConsoleLine()` 到 `WirelessConsole.h/.cpp`。

> 注：原规划中“本轮设计：收敛 `WifiStaConfig` 清理/加载 helper”已随 `v1.7.x` 迭代落地，详见 `WifiStaConfig.cpp` 当前实现。下文不再复述该轮详细设计。

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

### 短期（已处于尾声，需尽快收敛）

- 保留 `TUI`、`Buzzer`、传感器对象、Web server、`ControlData`、`SensorData`、RC `volatile` 数组等必要全局对象在 `MUS4_FW.ino`。
- **Wi-Fi / OTA runtime 状态不应再继续以散列 `extern bool` 形式存在**。当前 `WifiStaConfig`、`WirelessConsole`、`WifiIdentity`、`WifiOta` 中仍有较多 `extern` 桥接，这是下一步首要清理目标。
- 新模块优先通过结构体引用或参数访问状态，仅在确实需要跨模块共享硬件对象时保留 `extern`。
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

## 下一步建议（基于当前 `v1.7.3` 状态）

当前低风险工具、命令、诊断、传感器、类型收敛，以及无线/STA/OTA helper 拆分已经完成。`MUS4_FW.ino` 仍有约 2270 行，下一步应从“继续新增模块”转向**“收敛全局状态、迁出 `.ino` 中堆积的 Web/Wi-Fi 逻辑、为最终安全切片做准备”**：

1. **优先收敛 Wi-Fi / OTA 全局状态（不新增模块也能看到成效）**
   - 在 `SharedTypes.h` 或新建 `RuntimeState.h` 中引入 `WifiRuntimeState` 和 `OtaRuntimeState`。
   - 将 `MUS4_FW.ino` 中 `wifiStaConnected`、`wifiOtaWindowOpen`、`wifiStaHandoffActive` 等 15+ 变量迁入结构体。
   - 修改 `WirelessConsole.cpp`、`WifiStaConfig.cpp`、`WifiOta.cpp` 的接口，由 `extern bool` 改为接收 `WifiRuntimeState&` / `OtaRuntimeState&`。
   - 此步骤做完后，`MUS4_FW.ino` 可减少约 50～80 行声明与 `extern` 噪音，并为后续 `WifiManager` 拆分打好基础。

2. **迁出 Web route 与 WebSocket telemetry（最大的 `.ino` 体积负担）**
   - 新建 `WebConsoleServer.h/.cpp`，将 `handleWifiWebRoot()`、`handleWifiWebStatus()`、`handleWifiWebCommand()`、`handleWifiWebUpdateUpload()` 等约 650 行 handler 整体迁出。
   - 新建 `WebTelemetry.h/.cpp`，将 ring buffer、数据点采样、`pushWifiWebSocketData()` 迁出。
   - `MUS4_FW.ino` 只保留 `server.on(...)` 的注册调用或全部委托给 `webConsoleServer.begin()`。

3. **拆分 `WifiManager`（承接 Wi-Fi runtime 状态机）**
   - 将 `setupWifiConsoleServices()`、`startWifiApServices()`、`applyWifiStaCredentials()`、`updateWifiSta()`、`startWifiStaHandoff()`、`finishWifiStaHandoff()` 等迁入 `WifiManager.h/.cpp`。
   - `WifiStaConfig` 只保留配置读写与 Web/API 命令入口，不再处理连接状态机。
   - `WifiIdentity` 继续独立，提供 AP SSID/mDNS 相关 helper。

4. **继续后置 RC/ISR、Park/PWM 安全关键切片**
   - 在前三步让 `.ino` 体积降到 1200 行以下、全局状态清晰后，再进入 `RcInput` / `RcPwmCapture` / `SafetyState` / `ControlMixer` / `ActuatorOutput` 拆分。
   - 这些切片必须配合断动力或架空车轮硬件验证，每次只迁一个责任域。

5. **最后引入 `FirmwareApp` 装配层**
   - 当 `loop()` 中只剩调度代码、各模块边界稳定后，再封装 `FirmwareApp.begin()` / `FirmwareApp.update()`。
   - 不要提前做：当前初始化顺序仍有较多隐式依赖，提前封装会掩盖问题。

6. **文档与测试同步**
   - 每次收敛全局状态后，更新 `tests/test_firmware_feature_flags.py` 的源码断言，确认目标变量/函数已从 `MUS4_FW.ino` 迁出。
   - 同步更新 `AGENTS.md` 的代码组织图和模块清单。
   - 保持 `wireless_console_policy.py` 与 `WirelessConsole.cpp` 的权限策略一致。
