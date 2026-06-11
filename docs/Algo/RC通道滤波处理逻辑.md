# RC CH1-CH6 通道滤波处理逻辑

## 1. 通道定义

当前固件统一按 6 路 RC PWM 输入处理：

| 通道 | GPIO | 索引常量 | 业务含义 | 滤波策略 |
| --- | --- | --- | --- | --- |
| CH1 | GPIO36 | `CH_STEERING` | 转向 | ISR 突变确认 + 5 点中值滤波 |
| CH2 | GPIO39 | `CH_THROTTLE` | 油门 | ISR 突变确认 + 5 点中值滤波 |
| CH3 | GPIO34 | `CH_PARK` | Park 长按锁定/解锁 | ISR 突变确认 + 5 点中值滤波 + 辅助通道稳定确认 |
| CH4 | GPIO26 | `CH_MODE` | 驾驶模式 | ISR 突变确认 + 5 点中值滤波 + 辅助通道稳定确认 |
| CH5 | GPIO27 | `CH_DRIFT` | Drift Assist 开关 | ISR 突变确认 + 5 点中值滤波 + 辅助通道稳定确认 |
| CH6 | GPIO35 | `CH_DRIFT_SCALE` | Drift Assist 强度比例 | ISR 突变确认 + 5 点中值滤波 + 辅助通道稳定确认 |

所有通道的有效 PWM 范围为：

```cpp
#define RC_PWM_MIN 800
#define RC_PWM_MAX 2200
```

信号超时判定为：

```cpp
#define RC_SIGNAL_TIMEOUT 1000000UL
```

即某通道超过 1 秒没有更新有效脉宽时，视为失效。

## 2. 总体处理链路

每个通道从 GPIO 输入到业务使用，经过以下阶段：

```text
GPIO 边沿中断
  -> 上升沿记录时间
  -> 下降沿计算脉宽
  -> ISR 内有效范围检查
  -> ISR 内突变确认
  -> 写入 pwm_value[] / last_valid_time[]
  -> 主循环临界区快照
  -> 5 点中值滤波
  -> CH3-CH6 辅助通道稳定确认
  -> 超时处理
  -> rc_data / car_output / Web Console / TUI 使用
```

`pwm_value[]` 是 ISR 写入的原始有效脉宽；`pwm_filtered[]` 是主循环输出的滤波值，Web Console 和 TUI 显示读取的是后者。

## 3. GPIO 边沿采样

初始化时，每个 RC 输入通过 `attachInterrupt(..., CHANGE)` 绑定中断：

```cpp
attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
```

中断入口为 `handle_interrupt(channel)`：

1. 调用 `micros()` 获取当前时间。
2. 若距离上一次边沿不足 `100 µs`，直接丢弃，抑制毛刺边沿。
3. 上升沿记录 `last_rise_time[channel]`。
4. 下降沿计算：

```cpp
width = now - last_rise_time[channel]
```

5. 将 `width` 交给 `acceptRcPulse()` 做有效性与突变确认。

这一层解决的是“边沿级毛刺”和明显非法 PWM。

## 4. ISR 内突变确认

`acceptRcPulse(channel, width, now)` 对所有通道共用同一套规则。

### 4.1 范围检查

```cpp
if (width < RC_PWM_MIN || width > RC_PWM_MAX) return;
```

低于 `800 µs` 或高于 `2200 µs` 的脉宽直接丢弃，不更新 `pwm_value[]`，也不更新 `last_valid_time[]`。

### 4.2 小变化直接接受

```cpp
if (diff <= 120) {
    pwm_value[channel] = pulse;
    last_valid_time[channel] = now;
}
```

其中 `diff = abs(pulse - pwm_value[channel])`。

小于等于 `120 µs` 的变化被认为是正常抖动或缓慢变化，立即接受。

### 4.3 中等变化需要候选确认

```cpp
else if (diff <= 200) {
    if (abs(pulse - candidate_pwm[channel]) < 80) {
        pwm_value[channel] = pulse;
        last_valid_time[channel] = now;
    }
    candidate_pwm[channel] = pulse;
}
```

`120..200 µs` 的变化不会第一次就写入，必须与上一帧候选值接近（小于 `80 µs`）才接受。

目的：过滤单次中等幅度误采样。

### 4.4 大变化需要连续确认

```cpp
else {
    if (abs(pulse - last_large_pwm[channel]) < 100) {
        large_change_count[channel]++;
        if (large_change_count[channel] >= 2) {
            pwm_value[channel] = pulse;
            last_valid_time[channel] = now;
            large_change_count[channel] = 0;
        }
    } else {
        large_change_count[channel] = 0;
    }
    last_large_pwm[channel] = pulse;
}
```

大于 `200 µs` 的跳变需要连续多次观测到相近值才接受。

这层让真实档位切换仍能生效，但单个异常脉冲不会立即污染 `pwm_value[]`。

## 5. 主循环快照

ISR 会异步更新 `pwm_value[]` 和 `last_valid_time[]`。主循环读取前先进入短临界区，把所有通道复制到局部快照：

```cpp
noInterrupts();
for (int i = 0; i < RC_CHANNEL_COUNT; i++) {
    pwmSnapshot[i] = pwm_value[i];
    lastValidSnapshot[i] = last_valid_time[i];
}
interrupts();
```

后续滤波只使用快照，避免出现“脉宽值已更新但时间戳还未同步”或相反的半更新状态。

这一步对 Web Console 曲线稳定性影响明显。

## 6. 5 点中值滤波

主循环每 `RC_FILTER_UPDATE_INTERVAL` 更新一次滤波值。当前文档对应代码中使用 5 点窗口：

```cpp
#define PWM_FILTER_SIZE 5
```

每个通道各自维护：

- `pwm_filter_buf[channel][5]`
- `pwm_filter_idx[channel]`
- `pwm_filter_initialized[channel]`

首次收到有效 raw 值时，会用该 raw 值填满窗口：

```cpp
if (!pwm_filter_initialized[ch]) {
    for (int i = 0; i < PWM_FILTER_SIZE; i++) pwm_filter_buf[ch][i] = raw;
    pwm_filter_idx[ch] = 0;
    pwm_filter_initialized[ch] = true;
}
```

这样避免初始全 0 缓冲区导致启动阶段中值异常。

之后每次写入当前 raw：

```cpp
pwm_filter_buf[ch][idx] = raw;
pwm_filter_idx[ch] = (idx + 1) % PWM_FILTER_SIZE;
```

再排序取中位数：

```cpp
median = medianFilter(pwm_filter_buf[ch], PWM_FILTER_SIZE)
```

5 点中值滤波能压制：

- 1 个异常点。
- 2 个异常点。

如果同一异常连续出现 3 次，窗口中位数会切到新值。因此 CH3-CH6 又额外增加了辅助通道稳定确认。

## 7. CH3-CH6 辅助通道稳定确认

`isAuxiliaryRcChannel()` 定义辅助通道：

```cpp
CH_PARK
CH_MODE
CH_DRIFT
CH_DRIFT_SCALE
```

即 CH3、CH4、CH5、CH6 都会经过 `stabilizeAuxiliaryPWM()`。

### 7.1 有效信号首次初始化

第一次收到有效中值时：

```cpp
aux_stable_pwm[ch] = value;
aux_candidate_pwm[ch] = value;
aux_candidate_count[ch] = 0;
aux_stable_initialized[ch] = true;
```

之后 Web/TUI/控制逻辑都优先看到这个稳定值。

### 7.2 小变化直接跟随

```cpp
if (diff <= 80) {
    aux_stable_pwm[ch] = value;
    aux_candidate_pwm[ch] = value;
    aux_candidate_count[ch] = 0;
    return value;
}
```

稳定值和新中值差异小于等于 `80 µs` 时直接更新。

这适合 CH6 旋钮缓慢变化，也能吸收接收机正常小抖动。

### 7.3 大变化需要 3 次确认

当新中值和稳定值差异大于 `80 µs` 时，不会立即输出，而是进入候选确认：

```cpp
if (abs(value - aux_candidate_pwm[ch]) <= 80) {
    aux_candidate_count[ch]++;
} else {
    aux_candidate_pwm[ch] = value;
    aux_candidate_count[ch] = 1;
}
```

只有候选值连续达到 3 次，才切换稳定值：

```cpp
if (aux_candidate_count[ch] >= 3) {
    aux_stable_pwm[ch] = value;
    aux_candidate_count[ch] = 0;
    return value;
}
```

如果只是单帧或双帧尖峰，函数返回旧的 `aux_stable_pwm[ch]`。

### 7.4 无效信号时保持稳定值

辅助通道无效时：

```cpp
return aux_stable_initialized[ch] ? aux_stable_pwm[ch] : value;
```

这避免短暂失效把默认值写到 Web 曲线中形成单点尖峰。

控制安全不依赖这个显示保持值，而是由 `parkValid`、`modeValid`、`driftValid`、`driftScaleValid` 这些有效性布尔值继续约束。

## 8. CH1 转向滤波逻辑

CH1 是连续控制通道，不走辅助稳定确认。

处理链路：

1. GPIO ISR 采样脉宽。
2. `acceptRcPulse()` 做范围检查和突变确认。
3. 主循环快照。
4. 5 点中值滤波。
5. 若 `steeringValid == true`：

```cpp
rc_data.steering = pwm_filtered[CH_STEERING];
```

6. 若超时：

```cpp
rc_data.steering = RC_STEERING_MID;
```

CH1 保留连续响应，不增加 3 次稳定确认，避免转向手感变钝。

## 9. CH2 油门滤波逻辑

CH2 与 CH1 类似，也是连续控制通道，不走辅助稳定确认。

处理链路：

1. GPIO ISR 采样脉宽。
2. `acceptRcPulse()` 做范围检查和突变确认。
3. 主循环快照。
4. 5 点中值滤波。
5. 若 `throttleValid == true`：

```cpp
rc_data.throttle = pwm_filtered[CH_THROTTLE];
```

6. 若超时：

```cpp
rc_data.throttle = RC_THROTTLE_MID;
```

CH2 不使用辅助通道稳定确认，避免油门响应被额外延迟；最终油门输出仍会受到 Park/紧急制动状态机覆盖。

## 10. CH3 Park 滤波逻辑

CH3 是 Park 长按通道，走辅助稳定确认。

处理链路：

1. GPIO ISR 采样。
2. `acceptRcPulse()` 做范围检查和突变确认。
3. 主循环快照。
4. 5 点中值滤波。
5. `stabilizeAuxiliaryPWM()` 做 3 次候选确认。
6. `park_change()` 使用稳定后的 `pwm_filtered[CH_PARK]` 判断是否按下：

```cpp
isPressed = pwm_filtered[CH_PARK] > 1500
```

CH3 短暂失效时，如果已经初始化稳定值，不再直接写入 1500；如果尚未初始化，则使用 1500 作为默认显示/中立值。

Park 的最终状态切换仍由长按状态机决定，因此单个采样跳变不会直接切换锁定状态。

## 11. CH4 Mode 滤波逻辑

CH4 是模式档位通道，走辅助稳定确认。

处理链路：

1. GPIO ISR 采样。
2. `acceptRcPulse()` 做范围检查和突变确认。
3. 主循环快照。
4. 5 点中值滤波。
5. `stabilizeAuxiliaryPWM()` 做 3 次候选确认。
6. `mode_change(modeValid)` 在信号有效时使用：

```cpp
rc_data.mode = pwm_filtered[CH_MODE];
```

阈值：

```cpp
<= 1250 -> CAR_MODE_MANUAL
>= 1750 -> CAR_MODE_FULL_AUTO
其他 -> CAR_MODE_SEMI_AUTO
```

如果 CH4 无效，`mode_change()` 直接返回，不改变当前模式。

因此 CH4 的 Web 曲线和实际模式切换都基于稳定后的 `pwm_filtered[CH_MODE]`。

## 12. CH5 Drift 开关滤波逻辑

CH5 是 Drift Assist 开关通道，走辅助稳定确认。

处理链路：

1. GPIO ISR 采样。
2. `acceptRcPulse()` 做范围检查和突变确认。
3. 主循环快照。
4. 5 点中值滤波。
5. `stabilizeAuxiliaryPWM()` 做 3 次候选确认。
6. `update_drift_assist_control(driftValid, driftScaleValid)` 中使用：

```cpp
enabled = driftValid && pwm_filtered[CH_DRIFT] > 1500
```

如果 CH5 无效，Drift Assist 被关闭；如果只是短暂失效且已有稳定值，Web 曲线仍保持上次稳定值，不画默认值尖峰。

## 13. CH6 Drift 强度滤波逻辑

CH6 是 Drift Assist 强度旋钮，走辅助稳定确认。

处理链路：

1. GPIO ISR 采样。
2. `acceptRcPulse()` 做范围检查和突变确认。
3. 主循环快照。
4. 5 点中值滤波。
5. `stabilizeAuxiliaryPWM()` 做 3 次候选确认。
6. `update_drift_assist_control()` 中使用：

```cpp
scalePwm = constrain(pwm_filtered[CH_DRIFT_SCALE], 1000, 2000)
drift_assist_scale = (scalePwm - 1000) / 500.0f
```

CH6 和 CH3/CH4/CH5 不同，它可能是连续旋钮，所以 `<=80 µs` 的小变化会直接跟随；大变化才需要 3 次确认。

如果 CH6 无效，控制层回到默认比例：

```cpp
drift_assist_scale = 1.0f
```

但显示层保持上次稳定值，避免短暂失效造成曲线尖峰。

## 14. 超时与默认值策略

### CH1/CH2

CH1/CH2 是连续控制通道，超时时业务数据直接回中：

- CH1 -> `RC_STEERING_MID`
- CH2 -> `RC_THROTTLE_MID`

### CH3/CH4/CH5/CH6

辅助通道超时后，控制逻辑优先看有效性布尔值：

- CH3：Park 长按状态机继续使用稳定值，但真实切换依赖长按，不会因单点跳变立刻切换。
- CH4：`modeValid == false` 时不更新模式。
- CH5：`driftValid == false` 时 Drift Assist 关闭。
- CH6：`driftScaleValid == false` 时 Drift 比例回到 `1.0`。

显示层和 Web 曲线则尽量保持上次稳定 PWM，避免默认值单点尖峰。

## 15. Web Console 数据来源

Web Console 的 `/api/data` 直接采样 `pwm_filtered[]`：

```cpp
point.rcChannels[i] = pwm_filtered[i];
```

因此：

- CH1/CH2 曲线反映 5 点中值滤波结果。
- CH3/CH4/CH5/CH6 曲线反映 5 点中值滤波 + 辅助稳定确认结果。

Web 前端不再额外滤波，所有抗尖峰逻辑都在固件侧完成。

## 16. 设计取舍

### 为什么 CH1/CH2 不使用辅助稳定确认

CH1/CH2 是转向和油门，用户需要连续、低延迟响应。若大变化必须连续 3 次确认，会让快速打方向或油门变化变钝。

### 为什么 CH3-CH6 使用辅助稳定确认

CH3-CH5 主要是档位/开关类输入；CH6 虽是旋钮，但对瞬态跳变敏感，且 `80 µs` 内的小变化仍会直接跟随。因此对 CH3-CH6 做额外确认更适合当前业务语义。

### 为什么无效时保持显示稳定值

短暂无效更可能来自边沿漏采或接收机瞬态，而不是用户真实操作。保持显示稳定值能避免 Web 曲线误导；控制层仍通过有效性布尔值走安全路径。

## 17. 关键参数汇总

| 参数 | 当前值 | 作用 |
| --- | --- | --- |
| `RC_PWM_MIN` | 800 µs | ISR 接受最小 PWM |
| `RC_PWM_MAX` | 2200 µs | ISR 接受最大 PWM |
| `RC_SIGNAL_TIMEOUT` | 1,000,000 µs | 通道失效判定 |
| GPIO 边沿防抖 | 100 µs | 过滤过近边沿 |
| ISR 小变化阈值 | 120 µs | 直接接受 |
| ISR 中变化阈值 | 200 µs | 需要候选确认 |
| ISR 中变化候选窗口 | 80 µs | 判断候选是否一致 |
| ISR 大变化候选窗口 | 100 µs | 判断大变化是否连续 |
| `PWM_FILTER_SIZE` | 5 | 中值滤波窗口 |
| 辅助稳定小变化阈值 | 80 µs | CH3-CH6 直接跟随 |
| 辅助稳定确认次数 | 3 | CH3-CH6 大变化确认 |

## 18. 当前已知边界

- MCPWM Capture 实验代码仍保留，但默认关闭：

```cpp
#define ENABLE_RC_MCPWM_CAPTURE 0
```

- 当前实际运行路径仍是 GPIO 中断采样。
- CH3-CH6 的稳定器会带来数个滤波周期的档位切换延迟；这是为抑制单帧尖峰接受的权衡。
- 若未来需要更快响应 CH3/CH4 开关，可以按通道单独调整确认次数或阈值。
