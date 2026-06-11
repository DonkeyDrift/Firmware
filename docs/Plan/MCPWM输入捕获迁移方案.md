# 实施计划：将 RC PWM 采样迁移到 MCPWM 硬件输入捕获

## Context

Web Console 曲线中 CH4 读数出现“约 1000 µs 突然闪到 1500 µs 后又回到 1000 µs”的现象。CH4 是模式通道，当前代码使用 GPIO `CHANGE` 中断配合 `micros()` 软件计时读取 PWM。该方案能工作，但测量值会受 ISR 调度、Wi-Fi/WebServer 负载、Flash/系统中断延迟和主循环读取竞态影响。

MCPWM Capture 可以在硬件边沿到达时锁存计数器值，再由 CPU 稍后读取；理论上能显著降低软件 ISR 延迟造成的脉宽抖动。迁移目标不是改变控制语义，而是把 `pwm_value[]` 的来源从软件 `micros()` 计时改成硬件捕获后的脉宽结果，并保留现有滤波、超时、Park、Mode、Drift、Web Console 和 TUI 数据链路。

## 当前采样逻辑

### 引脚与通道

| RC 通道 | GPIO | 程序索引 | 用途 |
| --- | --- | --- | --- |
| CH1 | 36 | `CH_STEERING` | 转向 |
| CH2 | 39 | `CH_THROTTLE` | 油门 |
| CH3 | 34 | `CH_PARK` | Park |
| CH4 | 26 | `CH_MODE` | 模式 |
| CH5 | 27 | `CH_DRIFT` | Drift 开关 |
| CH6 | 35 | `CH_DRIFT_SCALE` | Drift 强度 |

### 软件计时流程

`setup()` 中对 6 路 RC 输入执行：

```cpp
pinMode(Channels[i], INPUT 或 INPUT_PULLDOWN);
attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
```

`handle_interrupt(channel)` 中：

1. 任意边沿触发 ISR。
2. `micros()` 读取当前时间。
3. 上升沿保存 `last_rise_time[channel]`。
4. 下降沿计算 `width = now - last_rise_time[channel]`。
5. 只接受 `800..2200 µs`。
6. 根据与 `pwm_value[channel]` 的差值做 120/200 µs 阈值确认。
7. 写入 `pwm_value[channel]` 和 `last_valid_time[channel]`。

主循环每 4 ms 对 `pwm_value[]` 做 5 点中值滤波，输出到 `pwm_filtered[]`。Web Console 每 16 ms 采样 `pwm_filtered[]`。

## 迁移目标

1. 使用 MCPWM Capture 硬件锁存 RC 输入边沿时间，替代 `attachInterrupt + micros()` 脉宽测量。
2. 保持 `pwm_value[]`、`last_valid_time[]`、`pwm_filtered[]` 作为上层数据接口，尽量减少控制逻辑改动。
3. 继续保留 `800..2200 µs` 有效范围检查、突变确认、中值滤波和超时默认值。
4. 优先覆盖 CH3-CH6，尤其 CH4；若资源允许，再扩展 CH1/CH2。
5. 提供可回退的编译开关，实车验证期间可一键切回旧 GPIO 中断方案。

## 关键约束

### 1. MCPWM 捕获通道数量

ESP32 经典 MCPWM 通常有 2 个 MCPWM unit，每个 unit 有 3 个 capture channel，总计 6 路输入捕获能力，理论上刚好覆盖 CH1-CH6。

迁移前必须确认当前 Arduino-ESP32 版本底层 ESP-IDF API 是否暴露可用的 MCPWM Capture 接口。不同 Arduino-ESP32 版本可能使用旧版 driver API 或新版 prelude API，代码形态会不同。

### 2. GPIO Matrix 与输入专用引脚

GPIO 34、35、36、39 是输入专用脚。它们可作为普通 GPIO 输入，但迁移 MCPWM 前必须实测/编译确认这些 GPIO 能否被路由到 MCPWM capture signal。

如果某些输入专用 GPIO 无法作为 MCPWM capture 输入，需要采用分阶段方案：

- 先将 CH4(GPIO26)、CH5(GPIO27) 迁移到 MCPWM 捕获。
- CH1/CH2/CH3/CH6 继续使用旧 GPIO 中断或改用 RMT/PCNT 方案。
- 后续若 PCB 或线束允许，再调整引脚。

### 3. 与 PWM 输出资源隔离

当前舵机/电调输出使用 Arduino `ledcAttachChannel()`，不是 MCPWM 输出。迁移输入捕获理论上不会与输出 PWM 资源冲突。

但必须避免把 `CH_STEERING`、`CH_THROTTLE` 这些业务通道索引继续误用为 LEDC/MCPWM 硬件通道号。迁移时建议显式新增捕获映射表，不复用业务索引表示硬件资源。

### 4. 安全边界

迁移只改变 RC PWM 测量来源，不改变：

- Park 长按语义。
- Mode 阈值。
- Drift 开关与比例。
- 输出 PWM 限幅。
- 无线/Web 命令权限。
- Serial1 回传协议。

## 总体架构

新增一个 RC PWM 采样抽象层，屏蔽底层来源：

```cpp
#define RC_CAPTURE_BACKEND_GPIO_ISR 0
#define RC_CAPTURE_BACKEND_MCPWM 1
#define RC_CAPTURE_BACKEND RC_CAPTURE_BACKEND_MCPWM
```

保留上层全局数据接口：

```cpp
volatile uint16_t pwm_value[RC_CHANNEL_COUNT];
volatile unsigned long last_valid_time[RC_CHANNEL_COUNT];
```

新增统一写入函数：

```cpp
static void IRAM_ATTR acceptRcPulse(uint8_t channel, uint32_t widthUs, uint32_t nowUs);
```

该函数集中处理：

- 有效范围检查。
- 小/中/大变化确认。
- 写入 `pwm_value[channel]`。
- 写入 `last_valid_time[channel]`。

旧 GPIO ISR 和新 MCPWM ISR 都调用 `acceptRcPulse()`，避免两套逻辑分叉。

## MCPWM 捕获设计

### 捕获状态结构

```cpp
struct RcCaptureState {
    uint32_t lastRiseTick;
    uint32_t lastEdgeTick;
    bool hasRise;
};

static volatile RcCaptureState rcCaptureStates[RC_CHANNEL_COUNT];
```

### 通道映射

建议新增显式映射表：

```cpp
struct RcMcpwmCaptureMap {
    uint8_t rcChannel;
    int gpio;
    int unit;
    int captureChannel;
};
```

首选全通道映射：

| RC 通道 | GPIO | MCPWM unit | capture channel |
| --- | --- | --- | --- |
| CH1 | 36 | 0 | 0 |
| CH2 | 39 | 0 | 1 |
| CH3 | 34 | 0 | 2 |
| CH4 | 26 | 1 | 0 |
| CH5 | 27 | 1 | 1 |
| CH6 | 35 | 1 | 2 |

若输入专用脚映射失败，则采用最小验证映射：

| RC 通道 | GPIO | 优先级 | 说明 |
| --- | --- | --- | --- |
| CH4 | 26 | 必须 | 当前已观察到异常的模式通道 |
| CH5 | 27 | 高 | 可验证非输入专用脚稳定性 |
| CH3 | 34 | 中 | Park 安全通道，需谨慎迁移 |
| CH6 | 35 | 中 | Drift 强度通道 |
| CH1/CH2 | 36/39 | 低 | 控制主通道，迁移前必须有充分回归 |

### 边沿处理

MCPWM capture ISR 接收到边沿事件后：

1. 根据 capture channel 找到 RC 通道。
2. 读取捕获 tick 和边沿类型。
3. 上升沿：记录 `lastRiseTick`。
4. 下降沿：计算 `deltaTicks = fallTick - lastRiseTick`。
5. 转换 `widthUs`。
6. 调用 `acceptRcPulse(channel, widthUs, micros())`。

要点：

- 宽度计算必须处理计数器回绕。
- `last_valid_time[]` 建议仍使用 `micros()` 的当前时间，方便保留现有超时判断。
- `widthUs` 来自硬件 tick 差值，不使用 `micros()` 差值。

### 计时单位

MCPWM capture 的 tick 频率需固定并记录，例如 1 MHz 或 APB 80 MHz。

建议配置为易读的 1 MHz tick：

- 1 tick = 1 µs。
- `widthUs = deltaTicks`。
- 对 RC PWM 1000-2000 µs 已足够。

如果 API 只能返回 APB tick，则使用：

```cpp
widthUs = deltaTicks / 80;
```

同时保留常量：

```cpp
#define RC_CAPTURE_TICKS_PER_US 80
```

## 分阶段实施步骤

### 阶段 0：调试前置观测

在迁移前先加观测，确认当前 CH4 的“1000 -> 1500 -> 1000”到底来自：

- `pwm_value[CH_MODE]` 原始值闪变。
- `pwm_filtered[CH_MODE]` 滤波输出闪变。
- Web 曲线默认值/显示层闪变。

建议日志字段：

```text
RC_DBG ch=3 raw=1004 filtered=1002 valid=1 age_us=18000
```

若 raw 本身没有跳到 1500，只是 filtered 或 Web 变 1500，则无需先迁移 MCPWM，应优先修显示/滤波默认值。

### 阶段 1：抽出统一 `acceptRcPulse()`

把 `handle_interrupt()` 中 falling edge 后的有效范围检查和突变确认逻辑抽出：

```cpp
static void IRAM_ATTR acceptRcPulse(uint8_t channel, uint32_t widthUs, uint32_t nowUs);
```

旧 `handle_interrupt()` 改成：

```cpp
if (pin_state[channel] == LOW) {
    uint32_t width = now - last_rise_time[channel];
    acceptRcPulse(channel, width, now);
}
```

该阶段不改变行为，只为 MCPWM 复用逻辑。

### 阶段 2：新增 MCPWM 捕获编译开关

新增：

```cpp
#define ENABLE_RC_MCPWM_CAPTURE 0
```

初始化逻辑改为：

```cpp
#if ENABLE_RC_MCPWM_CAPTURE
setupRcMcpwmCapture();
#else
setupRcGpioInterruptCapture();
#endif
```

确保默认仍为旧方案，便于小步提交和回退。

### 阶段 3：只迁移 CH4 做 A/B 验证

先只把 CH4(GPIO26) 绑定到 MCPWM capture，其他通道仍使用旧 GPIO ISR。

验证目标：

- CH4 固定在约 1000 µs 时，Web 曲线不再偶发跳到 1500。
- `mode_change()` 行为不变。
- CH4 在三档切换时仍能正确进入手动/半自动/全自动。

这个阶段能最大限度降低风险，因为 CH4 是问题通道且 GPIO26 支持内部下拉，不是输入专用脚。

### 阶段 4：扩展到 CH3-CH6

CH4 验证稳定后，扩展到辅助通道：

- CH3 Park：迁移前必须架空车轮或断开动力，确认长按锁定/解锁不误触发。
- CH5 Drift：确认 Drift 开关不因捕获迁移抖动。
- CH6 Scale：确认旋钮曲线平滑。

此阶段仍保留 CH1/CH2 旧采样，避免控制主通道在硬件捕获未充分验证前受影响。

### 阶段 5：全通道迁移

最后迁移 CH1/CH2：

- 转向与油门是安全关键通道。
- 迁移后必须重点验证输出限幅、失控默认值、Park 覆盖和 Serial1 回传。
- 若 CH1/CH2 对 MCPWM 路由不稳定，可长期保留混合后端：CH3-CH6 MCPWM，CH1/CH2 GPIO ISR。

## 代码结构建议

### 新增函数

```cpp
static void setupRcInputs();
static void setupRcGpioInterruptCapture();
static void setupRcMcpwmCapture();
static void IRAM_ATTR acceptRcPulse(uint8_t channel, uint32_t widthUs, uint32_t nowUs);
static bool snapshotRcPulse(uint8_t channel, uint16_t* width, unsigned long* lastValidUs);
```

### 保留旧 ISR

旧 GPIO ISR 不应立即删除，至少保留到 MCPWM 实车验证稳定：

```cpp
#if !ENABLE_RC_MCPWM_CAPTURE
void IRAM_ATTR CH4_interrupt() { handle_interrupt(CH_MODE); }
#endif
```

### 主循环读取快照

迁移 MCPWM 时顺带修复读写竞态：

```cpp
uint16_t rawSnapshot[RC_CHANNEL_COUNT];
unsigned long validSnapshot[RC_CHANNEL_COUNT];
noInterrupts();
for (int i = 0; i < RC_CHANNEL_COUNT; i++) {
    rawSnapshot[i] = pwm_value[i];
    validSnapshot[i] = last_valid_time[i];
}
interrupts();
```

主循环滤波使用快照，而不是直接读 `volatile` 数组。

## 测试与验证

### 桌面回归

迁移方案本身主要是固件底层，不容易用现有 Python 测试覆盖。仍需运行已有回归，确保 Web Console 策略和工具链未被破坏：

```powershell
pytest tests/
```

### 编译验证

优先使用 WSL 加速编译：

```powershell
.\arduino-cli-wsl.ps1 -Compile
```

若要上传并监视：

```powershell
.\arduino-cli-wsl.ps1 -Compile -Upload -Serial
```

### 实机验证矩阵

#### CH4 单通道验证

| 场景 | 预期 |
| --- | --- |
| CH4 固定手动档约 1000 µs | Web 曲线稳定在 1000 附近，无 1500 闪跳 |
| CH4 固定中档约 1500 µs | Web 曲线稳定在 1500 附近 |
| CH4 固定全自动档约 2000 µs | Web 曲线稳定在 2000 附近 |
| 快速切换三档 | 模式切换正确，无无效中间尖峰导致误判 |
| Wi-Fi Web Console 高频刷新 | CH4 不因网络负载出现测量闪跳 |

#### CH3-CH6 验证

| 通道 | 验证重点 |
| --- | --- |
| CH3 | Park 长按锁定/解锁不误触发 |
| CH4 | 模式档位稳定，无 1000/1500/2000 闪跳 |
| CH5 | Drift 开关无误触发 |
| CH6 | Drift Scale 曲线平滑，无回落默认值尖峰 |

#### CH1/CH2 验证

| 通道 | 验证重点 |
| --- | --- |
| CH1 | 转向中位、左右限幅、快速回中稳定 |
| CH2 | 油门中位、正反向限幅、Park 覆盖有效 |

## 风险与回退

### 风险 1：Arduino-ESP32 MCPWM API 不匹配

不同 Arduino-ESP32 版本的 MCPWM Capture API 差异较大。若编译期 API 不可用，先不要强行引入复杂兼容层，改为：

1. 保留本方案。
2. 先实现临界区快照和 CH4 显示有效性修复。
3. 单独建最小 MCPWM capture sketch 验证 API 后再迁移。

### 风险 2：输入专用 GPIO 路由失败

如果 GPIO34/35/36/39 不能稳定路由到 MCPWM capture，则采用混合方案：

- CH4(GPIO26)、CH5(GPIO27)：MCPWM。
- CH1/CH2/CH3/CH6：继续 GPIO ISR，或单独评估 RMT/PCNT。

### 风险 3：捕获 ISR 写入竞争

MCPWM ISR 与主循环共享 `pwm_value[]`、`last_valid_time[]`。必须用临界区快照，避免上层读到半更新状态。

### 风险 4：硬件捕获解决不了接收机自身抖动

如果接收机实际输出就是从 1000 短暂跳到 1500，MCPWM 会更准确地捕获这个跳变，而不是消除它。此时应改进：

- 通道迟滞。
- 状态确认帧数。
- 前端 invalid 断线显示。
- 接收机 failsafe/输出模式配置。

## 判定标准

MCPWM 迁移成功的标准不是“曲线完全没有变化”，而是：

1. 固定档位输入下，CH4 不再出现非操作导致的 1000->1500->1000 单点尖峰。
2. Web Console 高频刷新、日志输出、OTA idle 状态下，捕获值仍稳定。
3. 模式、Park、Drift 行为与迁移前一致。
4. Park 锁定时油门输出仍被安全覆盖。
5. 编译开关切回旧 GPIO ISR 后行为可恢复，便于回退定位。

## 推荐落地顺序

1. 抽出 `acceptRcPulse()`，旧 GPIO ISR 仍通过。
2. 增加主循环临界区快照，降低当前方案的竞态噪声。
3. 加 `ENABLE_RC_MCPWM_CAPTURE`，默认关闭。
4. 单独 MCPWM 捕获 CH4，验证 1000->1500->1000 闪跳是否消失。
5. CH4 稳定后扩展 CH3-CH6。
6. 全通道迁移前做安全评审，再决定是否迁移 CH1/CH2。
