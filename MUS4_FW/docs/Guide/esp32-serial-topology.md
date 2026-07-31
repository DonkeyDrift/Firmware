# ESP32 ↔ Linux 上位机串口拓扑

## 概述

MUS4 底盘采用 ESP32 作为底层控制器，与 Linux 上位机之间通过三路串口建立物理连接。本文档说明各路串口的引脚分配、电气特性、数据协议以及在 DonkeyDrift 系统中的角色分工。

## 物理拓扑总图

```
┌──────────────────────────────────────────────────────────────────────────┐
│                            ESP32-S3 (MUS4_FW)                            │
│                                                                          │
│  ┌── USB D+/D- ─────────────────────────────────── Type-C ──────────┐    │
│  │  Serial (USB CDC)  波特率: 115200                                 │    │
│  │  用途: 开发调试 / TUI 终端                                        │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│  ┌── GPIO16 (RX_1) ──────────────────── TTL 直连 ──── 上位机 UART ──┐   │
│  │  GPIO17 (TX_1)                                                   │    │
│  │  Serial1 (UART1)  波特率: 115200, 8N1                              │    │
│  │  用途: 车辆控制主数据通道 (上行遥测 + 下行指令)                     │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│  ┌── GPIO19 (RX_2) ───┐                    ┌── 上位机 UART ──────────┐   │
│  │  GPIO18 (TX_2)      │── UART_SEL(GPIO12) ──┤                       │   │
│  │  Serial2 (UART2)    │   LOW:  直连上位机   │  外部端子座 (RS232)   │   │
│  │  波特率: 115200      │   HIGH: 切换到端子座 ──┤                      │   │
│  └─────────────────────┘                    └────────────────────────┘   │
│                                                                          │
│  GPIO12 (UART_SEL) ─── 控制 Serial2 信号路由方向                         │
│                         LOW  = TTL 直连上位机 CPU                        │
│                         HIGH = 切换到外部 RS232 端子座                   │
└──────────────────────────────────────────────────────────────────────────┘
```

## 三路串口详细说明

### 1. Serial (USB CDC) — 开发调试通道

| 属性 | 值 |
|------|-----|
| **ESP32 侧** | 原生 USB CDC (Serial) |
| **物理连接** | USB Type-C 线缆直连上位机 USB 口 |
| **Linux 设备名** | `/dev/ttyUSB0` 或 `/dev/ttyACM0` |
| **波特率** | 115200 (`BAUD_RATE_0`) |
| **DonkeyDrift 是否使用** | 否（运行时完全不依赖此通道） |

**数据流向：**
- **下行 (上位机→ESP32)**: 通过 `readSerialBuf(Serial, serial0Buf)` 接收指令，可用于串口监视器手动调试
- **上行 (ESP32→上位机)**: TUI 终端渲染界面 (`TUI tui(Serial)`)、调试日志 (`DEBUG` 宏输出)

**用途：** 开发阶段通过 Arduino Serial Monitor 或 `screen /dev/ttyUSB0 115200` 观察 TUI 状态面板（RC 通道值、传感器数据、控制输出等），与 Serial1 控制通道完全解耦。

### 2. Serial1 (UART1, GPIO16/17) — 车辆控制主数据通道

| 属性 | 值 |
|------|-----|
| **ESP32 引脚** | RX = GPIO16 (`RX_1_PIN`), TX = GPIO17 (`TX_1_PIN`) |
| **物理连接** | TTL 电平直连上位机 CPU 硬件串口（不走电平转换芯片） |
| **Linux 设备名** | `/dev/ttyS4` (`ARDUINO_SERIAL_PORT`) |
| **波特率** | 115200, 8N1 (`BAUD_RATE_1`) |
| **DonkeyDrift 对接** | **是** — `actuator.py` 的 `Arduino` 类独占使用 |

**上行协议 (ESP32 → 上位机, ~160Hz 总带宽):**

| 帧格式 | 频率 | 触发条件 | 用途 |
|--------|------|----------|------|
| `T{throttle}S{steering}\n` | ~60Hz | 仅 MANUAL 模式 | 人工遥控油门/转向值回传 |
| `M{mode}:P{park}\n` | 1Hz 心跳 + 即时推送 | 状态变化时立即发送 | 模式(0=MANUAL/1=SEMI_AUTO/2=FULL_AUTO) 与手刹状态 |
| `$IMU,seq,ts_ms,ax,ay,az,gx,gy,gz\n` | ~100Hz | 所有模式持续发送 | 加速度(m/s²) + 陀螺仪(rad/s) |

**下行协议 (上位机 → ESP32):**
- 格式: `{throttle}:{steering}\n`
- Pilot AI 推理输出的控制指令，在 AUTO/SEMI_AUTO 模式下由固件解码后驱动舵机和电调
- 指令通过 `readSerialBuf(Serial1, serial1Buf)` 接收

**DonkeyDrift 代码映射:**

```python
# donkeycar/parts/actuator.py — Arduino 类初始化
Arduino.ard_device = serial.Serial(
    cfg.ARDUINO_SERIAL_PORT,  # "/dev/ttyS4"
    cfg.ARDUINO_BAUDRATE,     # 115200
    timeout=cfg.ARDUINO_TIMEOUT
)
```

Arduino 类的 `Arduino_readline()` 方法按行读取后根据前缀分流：
- `$IMU` → 解析后缓存至 `self.imu_data`，返回 `None`（不干扰控制流）
- `T...S...` / `M:P` → 解析为控制 dict 返回
- `ArdImu` Part 以 ~100Hz 轮询 `imu_data` 缓存，输出到 Memory 键 `imu/acl_x` ~ `imu/gyr_z`

### 3. Serial2 (UART2, GPIO18/19) — 可切换扩展串口

| 属性 | 值 |
|------|-----|
| **ESP32 引脚** | RX = GPIO19 (`RX_2_PIN`), TX = GPIO18 (`TX_2_PIN`) |
| **切换控制** | GPIO12 (`UART_SEL`) — LOW=直连上位机, HIGH=外部端子座 |
| **Linux 设备名** | 待分配（直连模式下为上位机另一路硬件串口） |
| **波特率** | 115200 (`BAUD_RATE_1`) |
| **当前状态** | 固件中**尚未初始化使用**（无 `Serial2.begin()` 调用） |

**UART_SEL 切换逻辑:**

```
UART_SEL = LOW  (digitalWrite(UART_SEL, LOW))
  ESP32 GPIO19/18 ←──→ 上位机 CPU UART (TTL 直连)
  用途: 预留的第二路直连控制/调试通道

UART_SEL = HIGH (digitalWrite(UART_SEL, HIGH))
  ESP32 GPIO19/18 ←──→ 外部 RS232 端子座
  用途: 连接外部串口设备（GPS、激光雷达、外部 IMU 等）
```

**说明:** Serial2 当前为硬件预留设计。固件中 `setup()` 已配置 `UART_SEL=LOW` 将其切换为直连上位机模式，但未编写 `Serial2.begin()` 和相关数据收发逻辑，属于待实现的扩展通道。未来可根据需要：
- 作为第二路控制通道（冗余/分离不同类型数据流）
- 切换到外部端子座连接 GPS/雷达等串口外设

## 数据流全景图

```
                        DonkeyDrift Vehicle Loop
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
   ArdPWMSteering       ArdPWMThrottle          ArdImu
   (threaded=True)      (threaded=False)      (threaded=True)
   ——唯一串口读取线程      ——写入控制指令          ——轮询 imu_data
          │                    │                    │
          └────────┬───────────┘                    │
                   │                                │
           Arduino.ard_device               Arduino.imu_data
           (pyserial.Serial)                 (dict 缓存)
                   │
           /dev/ttyS4 (115200)
                   │
           Serial1 (GPIO16/17)
           TTL 直连上位机
                   │
    ╔══════════════╧══════════════╗
    ║      ESP32 MUS4_FW         ║
    ║                            ║
    ║  readSerialBuf(Serial1)    ║  ← 接收下行 {thr}:{str}
    ║                            ║
    ║  Serial1.print(T...S...)   ║  → 上行遥测 (~60Hz)
    ║  Serial1.print(M:P)        ║  → 上行状态 (1Hz)
    ║  Serial1.write($IMU...)    ║  → 上行IMU (~100Hz)
    ║                            ║
    ║  readSerialBuf(Serial)     ║  ← USB调试指令
    ║  TUI tui(Serial)           ║  → TUI渲染
    ╚════════════════════════════╝
                   │
         Serial (USB CDC)
         Type-C ─── 开发调试终端
```

## 配置键速查

### ESP32 固件侧 (`FirmwareConfig.h`)

| 宏定义 | 值 | 说明 |
|--------|-----|------|
| `BAUD_RATE_0` | 115200 | Serial (USB CDC) 波特率 |
| `BAUD_RATE_1` | 115200 | Serial1/Serial2 波特率 |
| `RX_1_PIN` | 16 | Serial1 接收引脚 |
| `TX_1_PIN` | 17 | Serial1 发送引脚 |
| `RX_2_PIN` | 19 | Serial2 接收引脚 (UART_SEL=LOW 直连上位机) |
| `TX_2_PIN` | 18 | Serial2 发送引脚 (UART_SEL=LOW 直连上位机) |
| `UART_SEL` | 12 | Serial2 路由切换 (LOW=直连CPU, HIGH=外部端子座) |

### DonkeyDrift 侧 (`myconfig.py`)

| 配置键 | 默认值 | 说明 |
|--------|--------|------|
| `ARDUINO_SERIAL_PORT` | `/dev/ttyS4` | Serial1 对应的 Linux 设备 |
| `ARDUINO_BAUDRATE` | 115200 | 与固件 `BAUD_RATE_1` 一致 |
| `ARDUINO_TIMEOUT` | 1 | 串口读取超时 (秒) |
| `ARDUINO_WRITE_TIMEOUT` | 1 | 串口写入超时 (秒) |
| `DRIVE_TRAIN_TYPE` | `"ARDUINO_CONTROLLER"` | 选择 ESP32 控制分支 |
| `HAVE_IMU` | `False` | 启用 ArdImu 从 $IMU 帧提取数据 |

## 相关文件

| 文件 | 说明 |
|------|------|
| `Firmware/MUS4_FW/MUS4_FW.ino` | ESP32 固件主 sketch，串口初始化与数据收发 |
| `Firmware/MUS4_FW/libraries/mus4_core/src/FirmwareConfig.h` | 引脚定义与波特率配置 |
| `donkeycar/parts/actuator.py` | DonkeyDrift 侧 Arduino 串口驱动、ArdPWM、ArdImu |
| `donkeycar/templates/myconfig.py` | 车辆配置模板，含串口配置键 |
| `donkeycar/templates/complete.py` | 车辆组装逻辑 (`add_drivetrain`, `add_imu`) |
