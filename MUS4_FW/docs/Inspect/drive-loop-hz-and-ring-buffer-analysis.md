# DRIVE_LOOP_HZ 提升分析与 ESP32 本地环形缓冲设计

## 一、DRIVE_LOOP_HZ = 60 对系统处理能力的影响

### 背景

用户将 `myconfig.py` 中的 `DRIVE_LOOP_HZ` 从默认的 20 提升到 60。Vehicle 主循环负责：从 Memory 读取最新状态 → 模型推理 → 写回控制指令 → `ArdPWMThrottle.run_threaded()` 写串口下发。本文分析该变更的实际收益与潜在瓶颈。

### 实际收益

**控制延迟降低 3 倍。** 20Hz 时两个控制周期之间最长间隔 50ms，60Hz 时缩短到 16.7ms。对漂移场景，以 300°/s 的典型横摆率计算，50ms 延迟意味着 15° 的角度偏差，16.7ms 则降至 5°——差距显著。

**IMU 数据录制率提升。** Tub 以 Vehicle 循环速率录制，每个循环取一次 Memory 中的 IMU 快照。20Hz 时 100Hz 的 `$IMU` 流 80% 被丢弃，60Hz 时只丢弃 40%。GRU 模型的 W=16 ring buffer 实际获得的时间分辨率从 50ms/步 提升到 16.7ms/步，时序粒度提升了 3 倍。

**下行指令更密集平滑。** `ArdPWMThrottle.run_threaded()` 每次被 Vehicle 调用就向 ESP32 发一次 `{thr}:{str}\n`。60Hz 意味着 ESP32 每 16.7ms 收到一次控制更新，舵机和电调的指令流更平滑，减少 20Hz 下 50ms 间隔造成的阶梯状控制跳变。

### 代价与瓶颈

**模型推理时间必须 < 16.7ms。** 这是最硬的约束。Vehicle 主循环是同步阻塞的——每轮迭代后按剩余时间 sleep：

```python
# vehicle.py:161
sleep_time = 1.0 / rate_hz - (time.time() - start_time)
if sleep_time > 0.0:
    time.sleep(sleep_time)
else:
    # jitter violation: 本轮耗时已超过 1/rate_hz，无法维持目标频率
```

如果模型推理加上所有 Part 的 `run()` 总耗时超过 16.7ms，循环实际频率会掉到推理速度的上限，60Hz 只是无法达成的目标值。Keras/TensorFlow 模型在 Pi 级别硬件上推理通常在 20–50ms，即使 20Hz 也只是勉强跑满。

**摄像头帧率必须跟上。** `CAMERA_FRAMERATE = DRIVE_LOOP_HZ` 意味着摄像头需要输出 60fps。高分辨率或 USB 摄像头可能只能到 30fps——此时模型推理的是重复帧，白白消耗算力。

**串口写入频率增加（但不构成瓶颈）。** 每次 Vehicle 循环在 AUTO 模式下调用 `Arduino.ard_device.write()`。60Hz 意味着每秒 60 次串口写入。Serial1 波特率 115200，每帧约 8 字节，60Hz 仅约 480 B/s，占用率低于 0.5%，串口带宽充裕。

### 验证方法

从 20Hz 到 60Hz，理论上有收益，但能否兑现取决于模型推理速度是否 < 16.7ms。建议：

1. 用 `PartProfiler`（Vehicle.stop() 时自动输出 prettytable）实测各 Part 的 P50/P95/P99 耗时分布
2. 如果模型推理 P95 > 16.7ms，60Hz 只是空转，实际频率被卡在推理速度上限
3. 可以考虑将模型推理改为异步（threaded part），让控制下发和推理解耦，但需要处理推理延迟导致的控制滞后

---

## 二、ESP32 侧本地环形缓冲设计

### 当前架构的信息损失

```
MPU6050 ──500Hz──▶ ESP32 loop ──100Hz──▶ Serial1 ──▶ ArdImu ──▶ Vehicle 20-60Hz ──▶ Tub 录制
                     ▲                              ▲                    ▲
                 4/5 丢弃                       40-80% 丢弃           单点采样
```

三层降采样后，Tub 里存储的是每 16–50ms 一个 IMU 快照。一次持续 200ms 的漂移过程，Tub 里只有 4–12 个 IMU 样本。关键瞬态——比如横摆角速度在 50ms 内从 0 飙升到 5 rad/s 的峰值——可能完全落在采样间隙中，录到的只是峰值前后的平缓值。

### 环形缓冲是什么

在 ESP32 内存中维护一个固定大小的循环数组，以传感器原生速率（或配置速率，如 500Hz）持续写入最新 IMU 样本，新数据覆盖最旧的数据：

```
ESP32 内存中的环形缓冲（500Hz / 16 槽示例）：

写入指针 ──▶ [t-15] [t-14] ... [t-2] [t-1] [t0]  ◀── 最新样本
              │                              │
              └── 32ms 前                     └── 当前 (0ms)

每次 MPU6050 采样 (2ms) 写入一个槽，指针前移，覆盖最旧的槽。
缓冲始终保持最近 32ms 的完整 500Hz 动力学历史。
```

### 应用场景与意义

**碰撞/事故取证。** 碰撞前 500ms 内车辆经历了什么加速度峰值？哪个轴向最先出现冲击？当前单点采样完全无法回答。环形缓冲保留碰撞前 32ms（16 槽 × 2ms）的全分辨率数据，上位机检测到异常后发送指令 dump 缓冲，获得完整的事故前动力学曲线。

**漂移质量评估。** 漂移的关键指标——最大横摆角速度、侧向加速度峰值、回正时刻的角速度过零点——都需要高频采样才能精确定位。500Hz 数据中这些特征点的时间精度是 ±2ms，而 20Hz 录制是 ±50ms，差了 25 倍。事后分析漂移质量时，环形缓冲提供的高频数据可以精确刻画漂移的起漂、维持、回正三个阶段。

**触发式传输，不占常态带宽。** 缓冲数据不需要持续上传——上位机在检测到特定条件时（模式切换、急减速/急加速、手动触发）通过下行指令请求 dump，ESP32 将缓冲一次性打包发送。这样既保留了高频信息，又不增加常态串口带宽占用。

**离线训练数据增强。** 拿到 500Hz 全分辨率数据后，可以通过降采样模拟不同录制频率对模型精度的影响，帮助用数据（而非直觉）确定最优的 `DRIVE_LOOP_HZ` 配置。

### 实现要点（概要）

1. **缓冲结构**：在 ESP32 侧定义 `struct ImuSample { uint32_t ts_ms; float accel[3]; float gyro[3]; }`，数组大小可配置（如 32/64/128 槽）
2. **写入**：每次 `read_mpu6050()` 后追加到环形缓冲，更新写指针
3. **下行协议**：新增指令如 `$DUMP_IMU\n`，ESP32 收到后将缓冲内容打包为二进制或 CSV 帧一次性发送
4. **上位机解析**：在 `Arduino_readline()` 中新增 `$IMU_DUMP` 帧类型，解析后写入 Memory 供录制或分析

### 相关配置参考

```python
# myconfig.py — 当前用户配置
DRIVE_LOOP_HZ = 60                    # 目标循环频率（受模型推理速度约束）
DRIVE_TRAIN_TYPE = "ARDUINO_CONTROLLER"
ARDUINO_SERIAL_PORT = "/dev/ttyS4"
```

```c
// FirmwareConfig.h — ESP32 固件侧（当前）
#define SENSOR_UPDATE_INTERVAL 2      // MPU6050 轮询间隔 (ms) — 500Hz
#define IMU_TELEMETRY_INTERVAL_MS 10  // $IMU 上行间隔 (ms) — 100Hz
#define RC_DATA_UPDATE_INTERVAL 16    // T...S... 上行间隔 (ms) — 60Hz
```

## 相关文档

- [ESP32 上行遥测频率设计分析](../guide/esp32-telemetry-frequency-design.md)
- [ESP32 串口拓扑](../guide/esp32-serial-topology.md)
