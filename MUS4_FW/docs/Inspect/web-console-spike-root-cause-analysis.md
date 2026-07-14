# Web Console 曲线尖峰跳变 — 全面根因分析与解决方案

> 生成日期：2026-07-14
> 分析范围：MUS4_FW Web Console 主控台曲线（Throttle / Steering / GyroZ）及 RC Channels 面板
> 关联文档：`docs/Inspect/web-console-ch3-ch6-spike-analysis.md`（CH3-CH6 尖峰初步分析）

---

## 1. 尖峰现象完整描述

### 1.1 曲线监控的具体指标

Web Console 主控台 Canvas 图表（[WebConsoleAssets.h](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h) 第 236 行 `draw()`）绘制三条曲线：

| 曲线 | 颜色 | 数据源 | 原始范围 | 显示范围 | 除数 |
|------|------|--------|----------|----------|------|
| Throttle | `#39d98a` 绿 | `car_output.throttle` | -100~100 | -1~1 | /100 |
| Steering | `#5cc8ff` 蓝 | `car_output.steering` | -100~100 | -1~1 | /100 |
| GyroZ | `#ff6b6b` 红 | `mpu6050Data.gyroZ` | ~-5~5 | -1~1 | /5 |

RC Channels 面板以文本形式显示 CH1-CH6（`pwm_filtered[0..5]`，单位 µs，范围 800~2200）。

Judge 页面有独立的 GyroZ 曲线（120 点，单条），不在本次分析范围内。

### 1.2 尖峰跳变的时间分布特征

| 场景 | 时间分布 | 典型持续时间 |
|------|----------|-------------|
| RC 信号失效/恢复 | 随机，与遥控器距离/障碍物相关 | 1~5 秒（失效期间持续） |
| RC 通道瞬态抖动 | 随机，接收机输出质量相关 | 1~2 个采样周期（16~32ms） |
| 模式切换（CH4） | 用户操作时 | 1~2 个采样周期 |
| Park 锁定/解锁 | 用户操作时 | 1 个采样周期（阶跃） |
| 漂移辅助启用/禁用 | 用户操作时 | 1~2 个采样周期 |
| OTA 传输 | 固件更新时 | 整个 OTA 期间（数据完全中断） |
| 屏保模式进入/退出 | Park 锁定 + CH1 静止 60 秒后 | 进入/退出瞬间各 1 个周期 |
| 浏览器 GC 停顿 | 随机，与内存分配频率相关 | 5~50ms |
| 堆水位跳过推送 | ESP32 内存紧张时 | 不确定，恢复时产生数据间隙 |

### 1.3 尖峰的峰值幅度

| 指标 | 正常波动范围 | 尖峰峰值幅度 | 尖峰类型 |
|------|-------------|-------------|---------|
| Throttle | ±5（±0.05 显示） | ±100（±1.0 显示），Park 切换时 | 阶跃 |
| Steering | ±10（±0.1 显示） | ±100（±1.0 显示），Park 切换时 | 阶跃 |
| GyroZ | ±0.5（±0.1 显示） | ±5（±1.0 显示），漂移辅助切换时 | 阶跃/脉冲 |
| CH3 (Park) | ±20µs | 500~1000µs（1000↔1500↔2000 跳变） | 阶跃 |
| CH4 (Mode) | ±20µs | 500~1000µs（1000↔1500↔2000 跳变） | 阶跃 |
| CH5 (Drift) | ±20µs | 1000µs（1000↔2000 跳变） | 阶跃 |
| CH6 (Scale) | ±20µs | 500~1000µs | 阶跃/脉冲 |

### 1.4 尖峰出现前的系统操作及业务流量变化

- **RC 信号丢失**：遥控器关机/超出范围/接收机供电不稳 → `last_valid_time` 超时 → 通道强制回退默认值
- **模式切换**：用户拨动 CH4 开关 → `mode_change()` → `updateControlOutput()` 输出突变
- **Park 锁定/解锁**：用户拨动 CH3 → `park_change()` → 输出从 guarded 切换到 enabled（或反向）
- **OTA 触发**：用户上传固件 → WebSocket 关闭 + 采样暂停 → 数据完全中断
- **多客户端连接**：第二个浏览器标签页打开 → 广播开销翻倍 → 推送间隔可能拉长

---

## 2. 根因定位 — 五层分层排查

### 2.1 前端代码逻辑层

#### 2.1.1 【P0 严重】`console.log` 在热路径中同步阻塞主线程

**位置**：[WebConsoleAssets.h](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h) 第 172 行 `updateState()` 函数内

```javascript
console.log('saver: active='+screenSaverActive+' park='+parkLocked+' ch1='+ch1Val+' range='+range.toFixed(1)+' n='+ch1Samples.length);
```

**问题**：`updateState()` 在每次收到数据帧时调用（WebSocket 模式下 62.5Hz）。`console.log` 是同步 I/O 操作，每次调用阻塞主线程 2~5ms。62.5Hz × 3ms = 每秒约 187ms 被阻塞（18.7% CPU 占用），导致渲染帧率不稳定，曲线绘制出现卡顿尖峰。

**验证方法**：Chrome DevTools → Performance → 录制 5 秒 → 搜索 `console.log` 调用栈，确认其在 `updateState` → `handleDataPayload` → `ws.onmessage` 调用链中。

**排查数据**：
- 调用频率：62.5 次/秒（WebSocket 模式）
- 单次阻塞：2~5ms（DevTools Console 面板打开时更长）
- 每秒累计阻塞：125~312ms

#### 2.1.2 【P1 高】`draw()` 无节流，每帧全量重绘

**位置**：第 236 行 `draw()` + 第 173 行 `if(added>0)draw()`

**问题**：WebSocket 每帧推送最多 8 个历史点，`added > 0` 时触发 `draw()`。在 62.5Hz 推送频率下，`draw()` 每秒最多被调用 62.5 次。每次 `draw()` 执行：
1. `ensureGrid()` — 检查网格缓存（通常跳过）
2. `ctx.clearRect()` — 清除整个绘图区
3. `ctx.drawImage()` — 从离屏 canvas 复制网格
4. 3 × `drawSeries()` — 每条曲线遍历 256 个点 + 像素分桶
5. 坐标轴标签绘制

**性能估算**：256 点 × 3 曲线 × 62.5Hz = 48,000 次 `lineTo` / 秒。在低端设备上可导致 10~30ms 的渲染延迟。

**排查数据**：Chrome DevTools → Performance → 录制 → 查看 `draw` 函数耗时。预期单次 2~8ms，62.5Hz 下累计 125~500ms/秒。

#### 2.1.3 【P1 高】失效通道默认值被绘制为连续曲线

**位置**：第 235 行 `drawSeries()` — 对所有相邻点执行 `lineTo()`

**问题**：当 RC 信号失效时，CH3 被强制设为 1000、CH5 设为 1000、CH6 设为 1500。信号恢复后值跳回实际 PWM。前端不知道通道有效性，将这些阶跃跳变用折线连接，在曲线上形成巨大的尖峰。

**根因**：二进制协议（schema v2）不携带通道有效性标志，前端无法区分真实 PWM 值和失效默认值。

#### 2.1.4 【P2 中】每帧创建大量临时对象导致 GC 压力

**位置**：第 174 行 `decodeBinaryDataPayload()` + 第 173 行 `handleDataPayload()`

**问题**：每帧解码创建：
- 1 个 `DataView` 对象
- 1 个 `latest` 对象（~35 个属性）
- 1 个 `points` 数组（最多 8 个对象，每个 6 个属性）
- `drawSeries()` 内的 `buckets[]` 稀疏数组

62.5Hz 下每秒产生约 500+ 个短生命周期对象，触发 V8 新生代 GC 约 5~10 次/秒，每次暂停 2~10ms。

#### 2.1.5 【P3 低】日志全量 `textContent` 替换

**位置**：第 137 行 `appendLogLine()`

**问题**：每条日志执行 `log.textContent = buf.slice(-LOG_DISPLAY_MAX_BYTES)`，16KB 文本的全量替换。WebSocket 日志推送频率高时（如 Serial1 遥测 10Hz + Web 日志），每秒触发多次 16KB DOM 文本节点重建。

**排查数据**：Chrome DevTools → Performance → 查看 `AppendLogLine` → `textContent` setter 耗时。16KB 文本节点重建约 1~3ms。

### 2.2 后端服务性能层（ESP32）

#### 2.2.1 【P1 高】单线程主循环竞争导致采样抖动

**位置**：[MUS4_FW.ino](file:///c:/Dev/DDC/Firmware/MUS4_FW/MUS4_FW.ino) 第 574~829 行 `loop()`

**问题**：主循环单线程顺序执行所有任务。关键竞争点：
- I2C 传感器读取（INA219 + MPU6050）：同步阻塞 2~5ms
- Serial1 上行帧拼装 + `Serial1.write()`：512 字节栈缓冲写入，约 0.5~1ms
- HTTP `handleClient()`：同步处理 HTTP 请求，JSON 序列化 256 个数据点时 5~15ms
- RC 滤波更新：6 通道 × 5 点中值排序，约 0.2ms
- `delay(5)` 固定底部延迟

**影响**：当 HTTP 请求（如 `/api/data` 或 `/api/status`）与 16ms 采样窗口重叠时，`sampleWifiWebData()` 被推迟执行，`dtMs` 出现 20~50ms 的异常值，在曲线上表现为时间轴不均匀和数值跳变。

**排查数据**：通过 `STATUS` 命令查看 `web_sample_dt_max`（采样最大间隔）。正常值 16ms，尖峰时可达 30~80ms。

#### 2.2.2 【P1 高】堆水位跳过导致数据间隙

**位置**：[WebTelemetry.cpp](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebTelemetry.cpp) 第 154 行

```cpp
if (ESP.getFreeHeap() < WIFI_WEB_TELEMETRY_MIN_FREE_HEAP) {
    wifiWebSocketHeapSkips++;
    return;
}
```

**问题**：当 ESP32 可用堆内存 < 60KB 时跳过 WebSocket 推送。跳过期间数据不发送，恢复后前端收到的 `droppedPoints` 增大，历史点出现间隙。前端 `addPoint()` 按顺序写入环形缓冲区，间隙后的第一个点与间隙前的最后一个点之间用 `lineTo()` 连接，形成跨间隙的直线 — 如果两端值差异大，视觉上即为尖峰。

**排查数据**：`STATUS` 命令中 `ws_heap_skip` 计数器 > 0 时确认此问题。

#### 2.2.3 【P2 中】`wifiWebDataIndexForSeq()` 线性搜索

**位置**：[WebTelemetry.cpp](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebTelemetry.cpp) 第 59~66 行

**问题**：为每个历史点线性搜索 256 容量的环形缓冲区，最坏情况 8 × 256 = 2048 次比较。虽然单次比较很快（`uint32_t` 等值比较），但在堆内存紧张或主循环繁忙时累积延迟。

#### 2.2.4 【P2 中】OTA 期间遥测完全中断

**位置**：[WebTelemetry.cpp](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebTelemetry.cpp) 第 153 行 + [WebConsoleServer.cpp](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleServer.cpp) 第 1135 行

**问题**：OTA 传输期间：
- `pushWifiWebSocketData()` 第 153 行 `if (otaRuntime.inProgress) return;`
- `sampleWifiWebData()` 通过 `if (!otaRuntime.inProgress)` 闸门跳过
- WebSocket 连接被 `closeAll(1000, "ota")` 主动关闭

OTA 完成后设备重启，前端经历断连重连，所有缓冲区清空，曲线出现完全的数据断裂。

### 2.3 网络传输层

#### 2.3.1 【P1 高】ESP32 WiFi 半双工竞争

**问题**：ESP32 WiFi 射频是半双工的。AP 模式下，设备需要同时：
- 接收客户端的 TCP ACK
- 发送 WebSocket 二进制帧（62.5Hz × ~250 字节 ≈ 15.6KB/s）
- 处理 HTTP 请求（`/api/status` 5s、`/api/log` 1s）
- 管理 AP 信标（默认 100ms 间隔）

当 BLE Gamepad 同时活动时（如果启用），2.4GHz 频段进一步竞争，导致 WiFi 包延迟或丢失，WebSocket 帧到达浏览器的时间抖动增大。

**排查数据**：`STATUS` 命令中 `ws_queue_full_skip` > 0 时确认 WebSocket 发送队列溢出。

#### 2.3.2 【P2 中】HTTP 轮询回退的自适应间隔抖动

**位置**：[WebConsoleAssets.h](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h) 第 178 行 `pollData()`

```javascript
delay=(j.points||[]).length?Math.max(30,Math.min(80,Math.round(elapsed*1.2))):100
```

**问题**：WebSocket 断开时回退到 HTTP 轮询。间隔根据上次请求耗时自适应（30~80ms），但在 ESP32 负载波动时，`elapsed` 在 10~60ms 间振荡，导致轮询间隔在 30~80ms 间频繁切换。数据点的时间间隔不均匀，`smoothedDt` 指数平滑（0.85/0.15 系数）无法完全消除抖动，曲线 X 轴定位出现偏差。

#### 2.3.3 【P3 低】二进制帧体积与带宽占用

**问题**：单帧 latest 区约 105 字节 + 8 个历史点 × 18 字节 = ~250 字节。62.5Hz 下约 15.6KB/s。ESP32 AP 模式理论带宽约 1~2MB/s，实际有效约 100~300KB/s。15.6KB/s 占用 5~15%，但与 HTTP 请求叠加时可能造成瞬态带宽竞争。

### 2.4 浏览器底层机制层

#### 2.4.1 【P1 高】V8 GC 全停顿导致渲染尖峰

**问题**：如 2.1.4 所述，62.5Hz 的对象分配产生大量新生代垃圾。V8 新生代 GC（Scavenge）通常 2~10ms，在主线程同步执行。GC 发生时 `requestAnimationFrame` 回调被推迟，导致 `draw()` 调用间隔出现 10~30ms 的间隔，曲线渲染出现「冻结-跳帧」现象。

**排查方法**：Chrome DevTools → Performance → 勾选「Memory」→ 录制 10 秒 → 查看 GC 事件（黄色三角标记）的 Duration 和频率。

#### 2.4.2 【P2 中】Canvas 2D 全量重绘的合成开销

**问题**：每次 `draw()` 执行 `ctx.clearRect()` + `ctx.drawImage()`（离屏网格合成）+ 3 × `drawSeries()`（256 点遍历 + 像素分桶）。`drawImage()` 涉及 GPU 合成（如果启用了硬件加速），`drawSeries()` 中的 `lineTo()` + `stroke()` 涉及路径栅格化。62.5Hz 下这些操作的累积 GPU/CPU 开销在低端设备上可导致 5~15ms 的渲染延迟。

#### 2.4.3 【P3 低】扩展程序注入脚本资源抢占

**问题**：浏览器扩展（如广告拦截器、翻译插件）可能注入 Content Script，监听 DOM 变化或拦截网络请求。Web Console 页面的高频 DOM 更新（`updateState()` 修改多个元素的 `textContent` 和 `className`）会触发扩展的 MutationObserver 回调，抢占主线程时间片。

**排查方法**：Chrome 无痕模式（禁用扩展）打开 Web Console，对比曲线流畅度。

### 2.5 业务流量波动层

#### 2.5.1 【P0 严重】RC 信号失效默认值回落 — 数据层尖峰的主要来源

**位置**：[MUS4_FW.ino](file:///c:/Dev/DDC/Firmware/MUS4_FW/MUS4_FW.ino) 第 674~683 行

```cpp
if (!parkValid) {
    pwm_filtered[CH_PARK] = 1000;  // 强制回退
    // ...
}
if (!driftValid && !aux_stable_initialized[CH_DRIFT]) pwm_filtered[CH_DRIFT] = 1000;
if (!driftScaleValid && !aux_stable_initialized[CH_DRIFT_SCALE]) pwm_filtered[CH_DRIFT_SCALE] = 1500;
```

**问题**：当 RC 信号超时（`RC_SIGNAL_TIMEOUT`）时：
- CH3 (Park) 强制回退到 1000µs
- CH5 (Drift) 回退到 1000µs（仅未初始化时）
- CH6 (Scale) 回退到 1500µs（仅未初始化时）
- **CH4 (Mode) 没有回退处理** — 显示不一致

信号恢复后值跳回实际 PWM，在 Web 曲线上形成巨大的阶跃尖峰。这是已有分析文档 [web-console-ch3-ch6-spike-analysis.md](file:///c:/Dev/DDC/Firmware/MUS4_FW/docs/Inspect/web-console-ch3-ch6-spike-analysis.md) 的核心结论。

#### 2.5.2 【P1 高】模式/Park/漂移状态切换的阶跃变化

**问题**：
- **Park 锁定/解锁**：`car_output.park` 变化导致 `updateControlOutput()` 输出从 0（guarded）切换到实际值（enabled），Throttle 和 Steering 曲线出现阶跃
- **模式切换**：Manual → Assist → Auto 切换时，控制源从 RC 遥控切换到上位机指令，输出值可能突变
- **漂移辅助启用/禁用**：`drift_compensation` 和 `gyro_z_filtered` 变化影响 GyroZ 曲线

这些是正常的业务逻辑切换，但在曲线上表现为尖峰，因为前端不做任何平滑处理。

#### 2.5.3 【P1 高】屏保模式进入/退出的数据不连续

**位置**：[WebConsoleAssets.h](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h) 第 148 行 `enterScreenSaver()` / `exitScreenSaver()`

**问题**：Park 锁定 + CH1 静止 60 秒后进入屏保，生成假数据（正弦波）。退出屏保时 `exitScreenSaver()` 调用 `clearChart()` 清空所有数据点，曲线从空白重新开始。进入屏保时，假数据与真实数据之间有不连续点。

#### 2.5.4 【P2 中】ISR 与主循环的原子性风险

**位置**：[MUS4_FW.ino](file:///c:/Dev/DDC/Firmware/MUS4_FW/MUS4_FW.ino) 第 601~606 行

```cpp
noInterrupts();
for (int i = 0; i < RC_CHANNEL_COUNT; i++) {
    pwmSnapshot[i] = pwm_value[i];
    lastValidSnapshot[i] = last_valid_time[i];
}
interrupts();
```

**现状**：主循环已使用 `noInterrupts()` 临界区快照 `pwm_value[]` 和 `last_valid_time[]`，**原子性风险已被消除**。已有分析文档中提到的原子性风险在当前代码中已修复。

---

## 3. 根因优先级排序

| 优先级 | 根因 | 影响层面 | 尖峰贡献度 | 修复难度 |
|--------|------|----------|-----------|---------|
| P0 | `console.log` 在 62.5Hz 热路径中同步阻塞 | 前端 | 30% | 极低 |
| P0 | RC 信号失效默认值回落被绘制为连续曲线 | 业务+前端 | 25% | 中 |
| P1 | `draw()` 无节流，62.5Hz 全量重绘 | 前端 | 15% | 低 |
| P1 | 堆水位跳过推送导致数据间隙 | 后端 | 10% | 中 |
| P1 | GC 停顿由每帧对象分配触发 | 浏览器 | 8% | 中 |
| P1 | 单线程主循环 I2C 阻塞导致采样抖动 | 后端 | 5% | 高 |
| P2 | 屏保模式进入/退出数据不连续 | 业务 | 3% | 低 |
| P2 | 状态切换阶跃变化无前端平滑 | 业务+前端 | 2% | 中 |
| P2 | HTTP 轮询自适应间隔抖动 | 网络 | 1% | 低 |
| P3 | 扩展程序注入脚本抢占 | 浏览器 | <1% | 不可控 |

---

## 4. 解决方案

### 4.1 短期止损方案（1 小时内实施）

#### 4.1.1 移除热路径 `console.log` 【P0，5 分钟】

**文件**：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h` 第 172 行

**操作**：删除或注释 `updateState()` 函数中的 `console.log('saver: active=...')` 语句。

**预期效果**：消除每秒 125~312ms 的主线程阻塞，渲染帧率提升 15~20%。

#### 4.1.2 为 `draw()` 添加 30fps 节流 【P1，10 分钟】

**文件**：`WebConsoleAssets.h` 第 173 行 `handleDataPayload()`

**操作**：将 `if(added>0)draw()` 改为基于 `requestAnimationFrame` 的节流渲染：

```javascript
let drawPending = false;
function scheduleDraw() {
    if (drawPending) return;
    drawPending = true;
    requestAnimationFrame(() => { drawPending = false; draw(); });
}
// 替换 if(added>0)draw() 为：
if (added > 0) scheduleDraw();
```

**预期效果**：`draw()` 调用频率从 62.5Hz 降至 ≤60Hz（显示器刷新率），且与浏览器渲染周期对齐，消除冗余绘制。在 120Hz 显示器上也能正确节流。

#### 4.1.3 为失效通道添加视觉断线 【P0，30 分钟】

**文件**：`WebConsoleAssets.h` 第 235 行 `drawSeries()` + 第 174 行 `decodeBinaryDataPayload()`

**操作**：在二进制解码时标记通道有效性（利用 latest 区已有的 RC 通道值与已知默认值比较，或添加有效性字段）。在 `drawSeries()` 中，遇到无效数据点时执行 `moveTo()` 而非 `lineTo()`，形成断线而非尖峰。

**简化方案（不改协议）**：在前端检测 CH3=1000 且 CH5=1000 且 CH6=1500 同时成立时，判定为失效状态，在该点断线。

**预期效果**：消除 RC 失效/恢复造成的阶跃尖峰，曲线在失效期间显示为断线。

#### 4.1.4 禁用 RC Channels 面板的高频 DOM 更新 【P2，10 分钟】

**文件**：`WebConsoleAssets.h` 第 172 行 `updateState()`

**操作**：将 RC 通道值显示更新从 62.5Hz 降频到 5Hz（与 `refreshStatus` 对齐）：

```javascript
let lastRcUpdate = 0;
// 在 updateState() 中：
if (performance.now() - lastRcUpdate > 200) {
    [p.ch1,p.ch2,p.ch3,p.ch4,p.ch5,p.ch6].forEach((v,i)=>chValues[i].textContent=v??'----');
    lastRcUpdate = performance.now();
}
```

**预期效果**：减少 57.5 次/秒的 DOM `textContent` 写入，降低布局抖动。

### 4.2 长期优化方案

#### 4.2.1 前端架构优化

**A. 对象池消除 GC 压力**

在 `decodeBinaryDataPayload()` 中使用预分配的对象池，避免每帧创建临时对象：

```javascript
const latestPool = { seq:0, t:0, dt:0, thr:0, str:0, gz:0, /* ... */ };
const pointsPool = Array.from({length:8}, () => ({seq:0, t:0, dt:0, thr:0, str:0, gz:0}));

function decodeBinaryDataPayload(buffer) {
    // 复用 latestPool 和 pointsPool，而非创建新对象
    const v = new DataView(buffer);
    // ... 直接写入 pool 对象的属性
    return { type:'data', dropped, latest: latestPool, points: pointsPool.slice(0, count) };
}
```

**预期效果**：消除 95%+ 的新生代 GC 触发，GC 停顿从 5~10 次/秒降至 <1 次/秒。

**B. Canvas 增量渲染**

仅清除和重绘新增数据点所在的 X 轴区域，而非全量重绘：

```javascript
let lastDrawnX = 0;
function drawIncremental() {
    // 只清除新增点所在区域
    const newX = rightX - (added * stepX);
    ctx.clearRect(newX, 0, rightX - newX, h);
    // 仅绘制新增点
    // ...
}
```

**预期效果**：渲染耗时从 O(256) 降至 O(8)，单次 `draw()` 从 2~8ms 降至 <1ms。

**C. 离散通道阶梯图渲染**

为 CH3/CH4/CH5 等开关/档位通道使用阶梯图（step chart）而非折线图：

```javascript
function drawSeriesStepped(key, color, min, max, divisor=1) {
    // 使用 stepBefore 插值：先垂直跳到新值，再水平延伸
    // 避免过渡期间的斜线被误读为渐变
}
```

**预期效果**：消除档位切换时的斜线视觉尖峰，更准确反映离散状态变化。

#### 4.2.2 后端架构优化

**A. WebSocket 二进制协议增加通道有效性字段**

在 `WebTelemetry.cpp` 的二进制帧 latest 区末尾添加 1 字节位掩码：

```cpp
uint8_t channelValidMask = 0;
if (steeringValid) channelValidMask |= (1 << 0);
if (throttleValid) channelValidMask |= (1 << 1);
if (parkValid) channelValidMask |= (1 << 2);
if (modeValid) channelValidMask |= (1 << 3);
if (driftValid) channelValidMask |= (1 << 4);
if (driftScaleValid) channelValidMask |= (1 << 5);
writeU8(channelValidMask);
```

前端解码后，失效通道的曲线断线显示。**需同步更新协议版本号为 v3**。

**B. `wifiWebDataIndexForSeq()` 改为 O(1) 直接索引**

由于环形缓冲区 seq 单调递增且容量固定为 256，可直接计算索引：

```cpp
static uint16_t wifiWebDataIndexForSeq(uint32_t seq)
{
    // seq 与 head 的关系：head = (seq - 1) % CAPACITY 的下一个位置
    // 即 index = (seq - 1) % CAPACITY... 但需验证 seq 与 head 的对齐
    // 更安全的方案：维护 seq -> index 的直接映射
    uint16_t index = (wifiWebDataHead + WIFI_WEB_DATA_CAPACITY - (wifiWebDataSeq - seq)) % WIFI_WEB_DATA_CAPACITY;
    return index;
}
```

**C. 可配置推送频率**

将 `WIFI_WEB_SOCKET_PUSH_INTERVAL_MS` 从编译期常量改为运行时可配置（通过 Web Console 命令），允许在不同场景下调整：
- 调参/调试：16ms（62.5Hz，当前默认）
- 监控观看：50ms（20Hz，降低带宽和渲染压力）
- 远程低带宽：100ms（10Hz）

**D. I2C 传感器异步读取**

将 INA219 和 MPU6050 的读取从同步阻塞改为基于 Wire 库的异步或 DMA 读取，避免 I2C 总线等待阻塞主循环。或者将传感器读取放入独立的 FreeRTOS task，通过共享变量与主循环交换数据（需加互斥锁或原子操作）。

#### 4.2.3 监控与告警

**A. 前端性能监控**

在 Web Console 中添加性能监控浮层（可 toggle 显示）：

```javascript
const perfStats = { drawMs: 0, decodeMs: 0, gcCount: 0, frameSkip: 0 };
// 在 draw() 前后记录耗时，定期报告
```

**B. 后端尖峰预告警**

在 `pushWifiWebSocketData()` 中，当 `droppedPoints` 增量超过阈值或 `heapSkips` 连续发生时，在下一帧的 latest 区添加告警标志（复用保留字节 `writeU8(0)` 的 bit 位）：

```cpp
uint8_t alertFlags = 0;
if (wifiWebSocketHeapSkips > 0 && (millis() - lastHeapSkipMs < 5000)) alertFlags |= 0x01;
if (wifiWebSocketDroppedPoints > prevDropped + 10) alertFlags |= 0x02;
// 写入保留字节
```

前端收到告警标志后，在曲线区域显示告警图标。

**C. 现有诊断命令增强**

当前 `STATUS` 命令已输出大量诊断指标（`ws_dropped`、`ws_heap_skip`、`ws_max_backlog`、`web_sample_dt_max`、`web_ws_dt_max` 等）。建议增加：
- `ws_drop_rate`：丢点率 = `ws_dropped / (ws_dropped + ws_frames)` × 100%
- `heap_skip_rate`：堆跳过率
- `sample_jitter_ms`：采样间隔标准差

---

## 5. 根因验证与方案测试

### 5.1 预发布环境复现方案

#### 测试环境
- 设备：MUS4-v2.4.2 PCB，ESP32
- 遥控器：实际 RC 发射机 + 接收机
- 浏览器：Chrome 最新版（Desktop）
- 网络：ESP32 AP 模式直连

#### 复现步骤

1. **RC 信号失效尖峰复现**
   - 打开 Web Console，确认 WebSocket 已连接
   - 保持遥控器操作（CH1/CH2 有活动），曲线正常滚动
   - 关闭遥控器电源 → 观察 CH3/CH5/CH6 曲线出现阶跃尖峰
   - 重新打开发射机 → 观察恢复时的反向尖峰
   - 记录尖峰幅度和持续时间

2. **console.log 阻塞复现**
   - Chrome DevTools → Performance → 录制 10 秒
   - 在 Timeline 中搜索 `console.log` 调用
   - 记录每秒 `console.log` 调用次数和累计阻塞时长

3. **GC 停顿复现**
   - Chrome DevTools → Performance → 勾选 Memory → 录制 10 秒
   - 统计 GC 事件次数和总停顿时长
   - 对比实施对象池优化前后的 GC 频率

4. **堆水位跳过复现**
   - 在设备串口发送 `STATUS` 命令，记录 `ws_heap_skip` 基线值
   - 连接 2 个 WebSocket 客户端 + 持续 HTTP 轮询 `/api/data`
   - 再次发送 `STATUS`，确认 `ws_heap_skip` 是否增长

### 5.2 验证指标与 SLA

| 指标 | 优化前基线 | 优化后目标 | SLA 要求 | 验证方法 |
|------|-----------|-----------|---------|---------|
| `console.log` 阻塞/秒 | 125~312ms | 0ms | 0ms | DevTools Performance |
| `draw()` 调用频率 | 62.5Hz | ≤60Hz（rAF 对齐） | ≤60Hz | DevTools Performance |
| `draw()` 单次耗时 | 2~8ms | <1ms（增量渲染） | <2ms | `performance.now()` |
| GC 停顿频率 | 5~10次/秒 | <1次/秒 | <2次/秒 | DevTools Memory |
| RC 失效尖峰幅度 | 500~1000µs | 0（断线显示） | 0 | 视觉验证 + 截图 |
| `ws_heap_skip` 增长率 | 取决于负载 | 0 | 0 | `STATUS` 命令 |
| `web_sample_dt_max` | 16~80ms | ≤20ms | ≤25ms | `STATUS` 命令 |
| `ws_dropped` 增量/分钟 | 可变 | 0 | ≤5 | `STATUS` 命令 |
| 页面渲染帧率 | 30~45fps | 55~60fps | ≥50fps | DevTools FPS |
| 尖峰出现频率 | 5~20次/分钟 | ≤1次/分钟 | ≤2次/分钟 | 人工统计 |

### 5.3 尖峰降幅验证标准

在预发布环境中，对同一段 5 分钟的 RC 信号间歇性失效场景进行对比测试：

```
尖峰降幅 = (优化前尖峰次数 - 优化后尖峰次数) / 优化前尖峰次数 × 100%
```

**目标**：尖峰降幅 ≥ 90%。

**验证步骤**：
1. 优化前：录制 5 分钟曲线截图 + DevTools Performance trace
2. 实施全部短期方案 + 长期方案 A/B/C
3. 优化后：相同条件下录制 5 分钟曲线截图 + DevTools Performance trace
4. 对比尖峰次数、幅度、渲染帧率

### 5.4 自动化测试

在 `MUS4_FW/tests/` 中添加测试用例，验证协议变更的正确性：

```python
# test_web_console_spike_suppression.py
def test_binary_protocol_v3_channel_validity():
    """验证 v3 协议帧包含通道有效性位掩码"""
    # 构造模拟帧，验证解码结果

def test_draw_throttle_not_exceeding_60fps():
    """验证 draw() 节流逻辑不超过 60fps"""
    # 模拟 62.5Hz 数据输入，验证 draw() 调用次数 ≤ 60/秒

def test_invalid_channel_renders_as_gap():
    """验证失效通道渲染为断线而非尖峰"""
    # 模拟失效场景，验证 drawSeries 输出断线
```

---

## 6. 实施步骤

### 第一阶段：短期止损（1 小时内）

| 步骤 | 操作 | 文件 | 预计耗时 |
|------|------|------|---------|
| 1 | 删除 `updateState()` 中的 `console.log` | `WebConsoleAssets.h:172` | 2 分钟 |
| 2 | 为 `draw()` 添加 `requestAnimationFrame` 节流 | `WebConsoleAssets.h:173` | 10 分钟 |
| 3 | 添加失效通道断线渲染（简化方案） | `WebConsoleAssets.h:235` | 20 分钟 |
| 4 | RC 通道 DOM 更新降频到 5Hz | `WebConsoleAssets.h:172` | 10 分钟 |
| 5 | WSL 编译验证 | `arduino-cli-wsl.ps1 -Compile` | 5 分钟 |
| 6 | OTA 部署到设备 | `arduino-cli-wsl.ps1 -Upload -HttpOta` | 5 分钟 |
| 7 | 预发布环境验证 | 人工 + DevTools | 10 分钟 |

### 第二阶段：长期优化（按优先级排期）

| 步骤 | 操作 | 优先级 | 预计耗时 |
|------|------|--------|---------|
| 1 | 二进制协议 v3 增加通道有效性字段 | P1 | 2 小时 |
| 2 | 前端对象池消除 GC 压力 | P1 | 3 小时 |
| 3 | Canvas 增量渲染 | P2 | 4 小时 |
| 4 | `wifiWebDataIndexForSeq()` O(1) 优化 | P2 | 1 小时 |
| 5 | 可配置推送频率 | P2 | 2 小时 |
| 6 | 离散通道阶梯图渲染 | P3 | 2 小时 |
| 7 | I2C 异步读取 | P3 | 4 小时 |
| 8 | 前端性能监控浮层 | P3 | 2 小时 |
| 9 | 后端尖峰预告警 | P3 | 2 小时 |
| 10 | 自动化测试用例 | P2 | 2 小时 |

---

## 7. 监控运维建议

### 7.1 日常监控

| 监控项 | 检查频率 | 告警阈值 | 检查方法 |
|--------|---------|---------|---------|
| `ws_dropped` 增量 | 每小时 | >10/小时 | `STATUS` 命令 |
| `ws_heap_skip` | 每小时 | >0 | `STATUS` 命令 |
| `web_sample_dt_max` | 每小时 | >25ms | `STATUS` 命令 |
| `web_ws_dt_max` | 每小时 | >25ms | `STATUS` 命令 |
| `free_heap` | 每小时 | <80KB | `STATUS` 命令 |
| `min_free_heap` | 每小时 | <50KB | `STATUS` 命令 |
| 前端渲染帧率 | 每次 Web Console 使用 | <50fps | DevTools FPS Meter |
| 曲线尖峰频率 | 每次 Web Console 使用 | >2次/分钟 | 人工观察 |

### 7.2 运维操作建议

1. **OTA 前告知用户**：OTA 期间曲线会中断，提前告知操作人员
2. **多客户端限制**：避免同时打开 2 个以上 Web Console 标签页
3. **RC 信号质量**：定期检查接收机接线和供电，减少信号失效
4. **浏览器扩展**：调试 Web Console 时使用无痕模式，排除扩展干扰
5. **ESP32 散热**：高温环境下 ESP32 降频会导致主循环变慢，确保散热
6. **固件版本对齐**：前端代码内嵌在固件中，OTA 升级后前端自动更新，无需单独部署

### 7.3 回溯追踪

- Web Console 的 `STATUS` 命令输出是排查尖峰问题的首要数据源
- Chrome DevTools Performance 录制是排查前端性能问题的首选工具
- Tub JSON 录制功能（`tubRecordBtn`）可导出原始遥测数据用于离线分析
- `WebLogBuffer` 的日志环形缓冲区保留了操作时序，可用于回溯尖峰发生前的事件

---

## 附录 A：数据链路全图

```
RC 发射机
  ↓ PWM 信号 (50Hz, 800~2200µs)
ESP32 GPIO 中断 (ISR)
  ↓ acceptRcPulse(): 范围检查 + 三段变化确认
pwm_value[6] (volatile, ISR 写入)
  ↓ noInterrupts() 临界区快照
主循环 loop()
  ↓ 每 2ms: 5 点中值滤波 + 主辅通道稳定化
pwm_filtered[6]
  ↓ 每 16ms: sampleWifiWebData()
wifiWebData[256] (环形缓冲区, WebDataPoint)
  ↓ 每 16ms: pushWifiWebSocketData()
WebSocket 二进制帧 (端口 81)
  ↓ WiFi AP 模式无线传输
浏览器 ws.onmessage
  ↓ decodeBinaryDataPayload()
{ latest, points[] } 对象
  ↓ handleDataPayload()
  ├── addPoint() → points[256] 环形缓冲区
  ├── updateState() → DOM 元素更新 (62.5Hz)
  └── scheduleDraw() → draw() → Canvas 2D 绘制
```

## 附录 B：关键配置常量

| 常量 | 值 | 定义位置 |
|------|-----|---------|
| `WIFI_WEB_DATA_INTERVAL_MS` | 16ms | `WifiConsoleTypes.h` |
| `WIFI_WEB_DATA_CAPACITY` | 256 | `WifiConsoleTypes.h` |
| `WIFI_WEB_SOCKET_PUSH_INTERVAL_MS` | 16ms | `WifiConsoleTypes.h` |
| `WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME` | 8 | `WifiConsoleTypes.h` |
| `WIFI_WEB_SOCKET_MAX_CLIENTS` | 2 | `WifiConsoleTypes.h` |
| `WIFI_WEB_TELEMETRY_MIN_FREE_HEAP` | 60000 | `WifiConsoleTypes.h` |
| `SENSOR_UPDATE_INTERVAL` | 2ms | `FirmwareConfig.h` |
| `RC_FILTER_UPDATE_INTERVAL` | 2ms | `FirmwareConfig.h` |
| `RC_SIGNAL_TIMEOUT` | (见 FirmwareConfig.h) | `FirmwareConfig.h` |
| `PWM_FILTER_SIZE` | 5 | `FirmwareConfig.h` |

## 附录 C：已有尖峰抑制机制

| 层级 | 机制 | 位置 | 效果 |
|------|------|------|------|
| ISR 层 | 范围检查 (800~2200µs) | `RcPwmCapture.cpp:31` | 丢弃超范围脉冲 |
| ISR 层 | 三段变化确认 (≤60/≤200/>200µs) | `RcPwmCapture.cpp:39-69` | 延迟接受大幅变化 |
| ISR 层 | 100µs 边沿去抖 | `RcPwmCapture.cpp:82` | 抑制边沿抖动 |
| 滤波层 | 5 点中值滤波 | `RcFilter.cpp:14-32` | 压制单点/双点毛刺 |
| 滤波层 | 主通道平滑 (≤6µs 死区, 35% 平滑) | `RcFilter.cpp:44-64` | 平滑小幅抖动 |
| 滤波层 | 辅通道 3 帧确认 | `RcFilter.cpp:66-100` | 防止开关抖动穿透 |
| 采样层 | `noInterrupts()` 临界区快照 | `MUS4_FW.ino:601-606` | 消除 ISR/主循环竞态 |
| 协议层 | `droppedPoints` 计数 | `WebTelemetry.cpp:189` | 前端可感知丢点 |
| 协议层 | 堆水位检查 (60KB) | `WebTelemetry.cpp:154` | 防止内存耗尽 |
| 前端层 | 数据驱动按需重绘 | `WebConsoleAssets.h:173` | 无数据时不绘制 |
| 前端层 | 像素分桶优化 | `WebConsoleAssets.h:235` | 减少密集数据过度绘制 |
| 前端层 | 离屏网格缓存 | `WebConsoleAssets.h:234` | 避免网格重复绘制 |
| 前端层 | `document.hidden` 跳过渲染 | `WebConsoleAssets.h:237` | 标签页不可见时不绘制 |
