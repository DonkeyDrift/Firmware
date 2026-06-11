# Web Console 曲线 CH3-CH6 尖峰跳变分析

## 结论

从当前代码链路看，CH3 到 CH6 的曲线尖峰不宜直接判定为“Web Console 绘图驱动程序错误”。Web Console 只是高频采样并绘制 `pwm_filtered[]` 中的通道值，尖峰更可能来自 RC PWM 采集/滤波层的瞬态值、通道失效默认值回落，或前端曲线把离散状态通道按连续模拟量连接后放大显示。

需要重点关注的是：CH3、CH5、CH6 在信号失效时会被强制改写为不同默认值，但 CH4 没有同等的显式失效默认值；如果某些辅助通道边沿丢失、接收机输出短暂中断或滤波窗口尚未填满，Web 曲线会把这些短暂回落画成尖峰。

## 数据链路

### 1. Web Console 采样的是滤波后的 RC 通道

`mus4.ino` 中 `sampleWifiWebData()` 每 `WIFI_WEB_DATA_INTERVAL_MS = 16` ms 采样一次数据，约 62.5 Hz：

```cpp
for (uint8_t i = 0; i < RC_CHANNEL_COUNT; i++) {
    point.rcChannels[i] = pwm_filtered[i];
}
```

随后 `/api/data` 输出：

```cpp
"ch3" -> point.rcChannels[CH_PARK]
"ch4" -> point.rcChannels[CH_MODE]
"ch5" -> point.rcChannels[CH_DRIFT]
"ch6" -> point.rcChannels[CH_DRIFT_SCALE]
```

前端曲线直接绘制这些键：

```js
drawSeries('ch3', '#d96bff', 900, 2100)
drawSeries('ch4', '#f472b6', 900, 2100)
drawSeries('ch5', '#a3e635', 900, 2100)
drawSeries('ch6', '#fb923c', 900, 2100)
```

因此，Web 曲线没有重新计算 CH3-CH6，也没有做额外滤波；曲线上的尖峰基本代表 `pwm_filtered[CH3..CH6]` 在某个采样点出现过瞬态变化。

### 2. 前端会把所有采样点连成折线

前端 `pollData()` 拉取 `/api/data?since=...` 返回的所有点，然后逐点 `addPoint()`，`drawSeries()` 对相邻点直接 `lineTo()`。如果某个通道只出现 1 个采样周期的短暂异常，画面会显示成细尖峰。

这对 CH3/CH4/CH5 这类开关/档位通道尤其明显：它们本质上更像离散状态，而不是连续模拟曲线。用折线连续连接 1000、1500、2000 附近的值，会让短暂抖动看起来像“尖峰”。

## RC PWM 采集与滤波现状

### 1. 中断层已有范围检查和突变确认

`handle_interrupt()` 对 falling edge 计算脉宽，只接受 `800..2200 µs`：

```cpp
if (width >= RC_PWM_MIN && width <= RC_PWM_MAX) {
    uint16_t prev = pwm_value[channel];
    int diff = abs((int)width - (int)prev);
    ...
}
```

并按变化幅度做确认：

- `diff <= 120`：直接接受。
- `diff <= 200`：需要一次候选确认。
- `diff > 200`：需要两次大变化确认。

这说明底层并非完全裸采样；驱动层已经有基础去尖峰逻辑。

### 2. 主循环层每 4 ms 做 5 点中值滤波

主循环中每 `RC_FILTER_UPDATE_INTERVAL = 4` ms 更新一次 `pwm_filtered[]`：

```cpp
pwm_filter_buf[ch][idx] = raw;
pwm_filter_idx[ch] = (idx + 1) % PWM_FILTER_SIZE;
uint16_t median = medianFilter(pwm_filter_buf[ch], PWM_FILTER_SIZE);
```

5 点中值滤波能压制单点和双点异常，但也有两个限制：

1. 初始化阶段 `pwm_filter_buf` 全为 0，前几个周期可能产生非真实中值，直到窗口被有效 PWM 填满。
2. 如果同一异常值连续出现 3 次以上，中值滤波会认为它是新状态并输出。

### 3. CH3、CH5、CH6 有失效默认值回写

滤波更新后，代码会对部分辅助通道做失效默认值处理：

```cpp
pwm_filtered[CH_PARK] = parkValid ? pwm_filtered[CH_PARK] : 1500;
pwm_filtered[CH_DRIFT] = driftValid ? pwm_filtered[CH_DRIFT] : 1000;
pwm_filtered[CH_DRIFT_SCALE] = driftScaleValid ? pwm_filtered[CH_DRIFT_SCALE] : 1500;
```

这会导致两类可见跳变：

- 如果 CH3/CH5/CH6 短暂超时，曲线会瞬间跳到默认值。
- 信号恢复后，曲线又跳回真实 PWM 值。

其中 CH5 默认回落到 `1000`，如果实际遥控器保持在 `2000`，Web 曲线上会表现为一次很大的向下尖峰。

### 4. CH4 当前没有显式失效默认值回写

代码计算了：

```cpp
bool modeValid = (nowUs - last_valid_time[CH_MODE]) < RC_SIGNAL_TIMEOUT;
```

但后续没有类似：

```cpp
pwm_filtered[CH_MODE] = modeValid ? pwm_filtered[CH_MODE] : 1500;
```

`mode_change(modeValid)` 会拿到有效性参数，但 Web 曲线仍直接读取 `pwm_filtered[CH_MODE]`。这意味着 CH4 在失效或恢复时，显示曲线可能保留旧滤波值，或在窗口/原始值变化时出现与 CH3/CH5/CH6 不一致的跳变特征。

## 可能原因排序

### 高概率：辅助通道边沿短暂丢失或接收机输出抖动

CH3-CH6 是 Park、Mode、Drift 开关、Drift 强度等辅助通道，很多遥控器/接收机在开关档位变化、信号刷新不同步、接收机 failsafe 切换时，会出现短暂 PWM 变化。当前 Web 曲线采样频率较高，容易捕获这些瞬态。

### 高概率：失效默认值回落被曲线放大

CH3、CH5、CH6 的默认值分别是 1500、1000、1500。只要某个通道 `last_valid_time` 超过 1 秒未更新，Web 曲线就会看到默认值。信号恢复时会再次跳回实际值。

这不是 Web 绘图错误，而是安全/控制层状态回退被可视化出来。

### 中概率：滤波窗口初始化或恢复期瞬态

`pwm_filter_buf` 初始为 0。虽然无效信号路径会返回默认 1500，但在有效信号刚开始进入窗口时，5 点中值可能需要数个更新周期才完全稳定。上电、接收机刚连接、通道刚恢复时，Web 曲线可能出现起始尖峰。

### 中概率：开关通道用连续折线显示造成视觉尖峰

CH3/CH4/CH5 本质上多为档位/开关，折线图适合连续变量，但不适合离散状态。即使底层信号符合预期，档位切换时也会画成陡峭竖线；如果切换过程中经过中间脉宽，更像尖峰。

### 低到中概率：ISR 读写与主循环读取存在原子性风险

`pwm_value[]` 和 `last_valid_time[]` 由中断写入、主循环读取。ESP32 上 16/32 位读写通常可用，但当前主循环没有在临界区内成对读取 `pwm_value` 与 `last_valid_time`。理论上可能出现“值已更新但时间未同步”或“时间已更新但值仍旧”的短暂不一致。

这类问题通常表现为偶发瞬态，不一定是主要原因，但如果尖峰非常随机且与遥控器/接收机状态无关，应纳入排查。

## 是否是驱动程序问题

当前证据更支持以下判断：

1. Web Console 前端绘图逻辑没有明显把 CH3-CH6 算错；它只是直接画 `/api/data` 返回的 `ch3..ch6`。
2. `/api/data` 输出的是固件内部 `pwm_filtered[]` 快照，因此如果曲线有尖峰，尖峰来源在 RC 数据链路或状态回退，而不是单纯的浏览器绘图。
3. RC 驱动层已有范围检查、突变确认和 5 点中值滤波，不像完全失控的驱动错误。
4. 但 CH4 缺少与 CH3/CH5/CH6 一致的 Web 可视化失效默认值处理，这是一个值得修正或至少明确的显示一致性问题。

因此，结论是：更像“RC 辅助通道瞬态 + 当前显示策略暴露瞬态”，不是明确的 Web 驱动 bug；但 RC 采样一致性和 CH4 失效显示策略仍有改进空间。

## 建议验证步骤

### 1. 同时观察 `/api/data` 原始 JSON

在尖峰出现时，确认 `/api/data?since=...` 中 `ch3..ch6` 是否真的出现异常值。如果 JSON 中已有异常值，则问题在固件数据链路；如果 JSON 正常但画面异常，才是前端绘图问题。

### 2. 使用 `FILTER_DEBUG` 扩展到 CH3-CH6

当前调试输出只在 `ch == CH_THROTTLE` 时打印。建议临时扩展为可选打印 CH3-CH6 的 raw/median/valid，例如记录：

- `channel`
- `raw = pwm_value[channel]`
- `median = pwm_filtered[channel]`
- `valid = nowUs - last_valid_time[channel] < RC_SIGNAL_TIMEOUT`
- `last_valid_age_us`

这样可判断尖峰是 raw 输入异常、median 输出异常，还是 valid 失效回退。

### 3. 对比串口 `pwm_value[]` 原始输出

代码尾部已有循环打印 `pwm_value[i]` 的逻辑。可在串口日志中对比原始值与 Web 曲线，确认尖峰是否源自中断采样原始值。

### 4. 静态固定输入测试

保持遥控器不操作，分别测试：

- CH3 固定 Park 档。
- CH4 固定模式档。
- CH5 固定 Drift 开关档。
- CH6 固定旋钮位置。

如果固定不动仍出现尖峰，优先怀疑接线、电平质量、接收机输出稳定性、ISR 原子性或失效回退。若只在拨动开关/旋钮时出现，则更可能是遥控器通道的正常过渡或 Web 折线显示方式。

## 建议改进方向

### 1. 让 Web 数据携带通道有效性

建议 `/api/data` 增加 `v3..v6` 或 `chValid` 字段，前端能区分真实 PWM 与失效默认值。这样曲线可以在无效段断线，而不是画到默认值形成尖峰。

### 2. CH4 增加一致的失效显示策略

如果 CH4 失效时控制逻辑已有安全处理，Web 显示也应明确：要么保留最后有效值并标记 invalid，要么回落到 1500。当前 CH4 与 CH3/CH5/CH6 行为不一致，容易误判。

### 3. 前端对离散通道改为阶梯图或状态条

CH3/CH4/CH5 适合画成状态条或阶梯线，CH6 适合连续曲线。这样可以降低视觉尖峰误判。

### 4. 初始化滤波缓冲区

首次收到某通道有效 PWM 时，可用该值填满对应 `pwm_filter_buf[ch]`，避免窗口从 0 过渡到真实值时出现启动瞬态。

### 5. 成对读取中断共享数据

在主循环拷贝 `pwm_value[]` 与 `last_valid_time[]` 时，可考虑短临界区快照，再在临界区外滤波，减少 ISR 与主循环之间的瞬态不一致。

## 优先处理建议

1. 先加观测：记录 CH3-CH6 的 raw/filtered/valid/age，并确认尖峰是否来自 JSON。
2. 再改显示：Web 曲线对 invalid 通道断线或标记，不直接画默认值。
3. 最后评估驱动层：如果 raw 本身有尖峰，再检查接收机输出、电平质量、中断确认阈值和临界区快照。

---

## 附录 A：当前 PWM 采样逻辑 vs MCPWM 输入捕获

### 当前方案：GPIO 边沿中断 + `micros()` 软件计时

**初始化流程（mus4.ino ~2436 行）：**

```cpp
pinMode(Channels[i], INPUT_PULLDOWN);
attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
```

**ISR 流程：**

1. `CHANGE` 触发（上升沿 + 下降沿都进中断）。
2. `micros()` 记录当前时刻。
3. 上升沿保存 `last_rise_time[channel]`。
4. 下降沿计算 `width = now - last_rise_time[channel]`。
5. 范围检查（800-2200 µs） + 变化幅度确认（120/200 阈值）。

**主要优点：**

- 代码简单，6 通道都用同一模式，无需配置外设寄存器。
- GPIO 中断在 ESP32 上足够处理 50 Hz RC PWM（每个脉冲只有两个边沿）。
- `micros()` 在 ESP32 Arduino 中通常是硬件定时器驱动，单芯片调用开销很低。

**主要缺点（也是可能产生尖峰的真实原因）：**

1. **中断延迟抖动**：如果 Wi-Fi、I2S、Flash 等更高优先级的中断正在执行，GPIO 中断可能延迟几十到数百微秒才响应。`micros()` 的采样时刻会因此偏移，产生额外脉宽误差。
2. **软件去尖峰依赖历史值**：`diff <= 120` 这类阈值是在 ISR 内计算的，若前一个值本身就是误差值，后续判断也会偏移。
3. **无硬件级脉宽锁存**：ISR 响应时间包含了中断入口开销，这期间 PWM 信号可能已经变化。

### MCPWM 输入捕获方案

ESP32 有 MCPWM 模块，支持硬件级输入捕获（Capture）。

**工作原理：**

- MCPWM 外设直接监听 GPIO 边沿，硬件捕获时刻由内部时钟锁存，不依赖 CPU 响应速度。
- 边沿触发时硬件自动把计数器写入捕获寄存器，CPU 可以稍后读取，脉宽计算精度接近时钟周期（通常 80 MHz 即 12.5 ns 粒度）。
- 支持多通道、可滤波、可产生中断但不要求即时响应。

**对尖峰问题的理论改善：**

- 硬件锁存脉宽，不再受 ISR 延迟抖动影响，原始采样值本身更稳定。
- 即使 CPU 满载或 Wi-Fi 流量大，脉宽测量精度基本不变。
- 减少软件 `micros()` 与真实边沿之间的时间偏移误差。

**但也有代价：**

1. **引脚路由限制**：MCPWM 输入捕获有特定 GPIO matrix 路由规则，不是所有 CH3/CH5/CH6 引脚（34、35、27、26）都能直接映射到 MCPWM 捕获通道。现有 PCB 引脚可能需要重映射或飞线验证。
2. **Arduino 生态支持**：Arduino-ESP32 中 MCPWM 输入捕获的封装层较少，通常需要直接调用 ESP-IDF API，代码复杂度显著上升。
3. **多路通道冲突**：MCPWM 捕获通道数有限（每个 MCPWM 单元 3 个捕获通道，两个单元共 6 个），如果未来还要用 MCPWM 做 PWM 输出驱动，需要仔细分配资源。
4. **改动评估成本高**：当前软件滤波和失效保护逻辑已经在 `pwm_filtered[]` 层完成，即使底层换硬件捕获，上层策略仍需保留，收益不一定线性增加。

### 更实际的替代方案

在不换 MCPWM 的前提下，更可落地的改进点：

1. **增加 PCNT（脉冲计数器）作为补充**：ESP32 PCNT 外设可专门用于脉冲计数和脉宽测量，比 MCPWM 更轻量，引脚路由也更灵活。PCNT 阈值/过零中断仍由硬件触发，但脉宽仍需软件辅助计时。
2. **GPIO 滤波与上拉/下拉优化**：CH3/CH5/CH6 是仅输入引脚，无内部上拉，接收机输出电平质量和线长会直接影响边沿检测。可以验证外部 1k 上拉是否改善边沿抖动。
3. **临界区快照**：主循环读取 `pwm_value[]` / `last_valid_time[]` 前先 `noInterrupts()` 快照，再恢复中断，减少 ISR 与主循环的竞态。
4. **延长滤波窗口或增加额外确认**：将 5 点中值滤波扩为 7 点，或在滤波输出后再增加 2 帧确认，进一步压制残留瞬态。

### 最终建议

如果尖峰确实是软件 ISR 抖动造成的，MCPWM 硬件捕获会有明显改善；但如果尖峰来自接收机本身的输出抖动、信号质量或失效默认值回退，换 MCPWM 也无法根治。

建议先通过增加调试观测确认 raw 采样本身是否有高频抖动：

1. **先验证**：在串口打 raw 脉宽的分布、方差，确认是否真的存在 > 50 µs 的瞬态跳变，且跳变与遥控器操作无关。
2. **再对比**：如果 raw 确实乱跳，先试 PCNT 或仅改临界区快照等低成本方案；如果 raw 已经稳定但 Web 曲线仍尖，问题在显示层或滤波策略。
3. **最后再考虑 MCPWM**：只有当验证确认问题根源在 ISR 延迟抖动，且临界区快照/PCNT 仍不够时，再评估硬件捕获重构成本。
