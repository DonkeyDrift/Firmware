# 硬件引脚定义与说明

## 1. 概述

本文档详细定义了 **mus4** 自动驾驶小车控制系统的硬件引脚配置（MUS4-v2.3 PCB 版本）。该系统基于 **ESP32** 微控制器平台，集成了RC遥控接收机信号采集、电机/舵机PWM控制、上位机串行通信以及状态指示灯等功能。

本文档依据源代码 `mus4.ino` (MUS4-v2.3 PCB) 及架构文档编写，旨在为硬件连接、调试及后续开发提供权威参考。

## 2. 引脚功能总览

下表列出了系统中所有已定义及使用的引脚。

| 引脚编号 (GPIO) | 变量名称 | 功能分类 | I/O 方向 | 连接设备/模块 | 备注 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **5** | `LED_PIN` | LED 控制 | Output | WS2812B RGB LED | 用于显示驾驶模式及紧急停车状态 |
| **12** | `UART_SEL` | UART 选择 | Output | 串口选择开关 | 控制 UART 路由 |
| **21** | `SDA_PIN` | I2C 数据 | I/O | INA219 / MPU6050 | 已验证正确引脚 |
| **22** | `SCL_PIN` | I2C 时钟 | Output | INA219 / MPU6050 | 已验证正确引脚 |
| **16** | `RX_1_PIN` | Serial1 RX | Input | 上位机 / Pilot (RS232 RX) | 接收上位机控制指令 (波特率 115200) |
| **17** | `TX_1_PIN` | Serial1 TX | Output | 上位机 / Pilot (RS232 TX) | 发送车辆状态至上位机 (波特率 115200) |
| **23** | `STEERING_PIN` | PWM 输出 | Output | 转向舵机 (Servo) | 控制前轮转向角度 (50Hz) |
| **25** | `THROTTLE_PIN` | PWM 输出 | Output | 电机电调 (ESC) | 控制后轮电机转速 (50Hz) |
| **26** | `CH4_PIN` | PWM 输入 | Input | RC接收机 CH4 (模式/Mode) | 支持中断采集 |
| **27** | `CH5_PIN` | PWM 输入 | Input | RC接收机 CH5 (Drift 开关) | 支持中断采集 |
| **32** | `PWM_1` | PWM 输出 | Output | 预留 PWM 输出1 | 备用 PWM 输出通道 |
| **33** | `PWM_2` | PWM 输出 | Output | 预留 PWM 输出2 | 备用 PWM 输出通道 |
| **34** | `CH3_PIN` | PWM 输入 | Input | RC接收机 CH3 (停车/Park) | 仅输入引脚，支持中断采集 |
| **35** | `CH6_PIN` | PWM 输入 | Input | RC接收机 CH6 (Drift 强度旋钮) | 仅输入引脚，支持中断采集 |
| **36** | `CH1_PIN` | PWM 输入 | Input | RC接收机 CH1 (转向) | 仅输入引脚，支持中断采集 |
| **39** | `CH2_PIN` | PWM 输入 | Input | RC接收机 CH2 (油门) | 仅输入引脚，支持中断采集 |

> **注意**：GPIO 34, 35, 36, 39 在 ESP32 上为 **仅输入 (Input Only)** 引脚，非常适合用于高阻抗的信号采集，不能配置为输出模式，且无内部上拉/下拉电阻。

## 3. 详细引脚说明

### 3.1 RC 接收机输入通道 (GPIO 36, 39, 34, 26, 27, 35)

*   **硬件连接**：
    *   **CH1 (GPIO 36)**：连接接收机方向通道，对应程序中的 `CH_STEERING`。
    *   **CH2 (GPIO 39)**：连接接收机油门通道，对应程序中的 `CH_THROTTLE`。
    *   **CH3 (GPIO 34)**：连接接收机辅助通道，用于 **停车/解锁 (Park)** 控制，对应 `CH_PARK`。
    *   **CH4 (GPIO 26)**：连接接收机辅助通道，用于 **驾驶模式切换 (Mode)**，对应 `CH_MODE`。
    *   **CH5 (GPIO 27)**：连接接收机按钮型切换通道，用于 **Drift Assist 开关**，对应 `CH_DRIFT`。
    *   **CH6 (GPIO 35)**：连接接收机旋钮通道，用于 **Drift Assist 强度比例**，对应 `CH_DRIFT_SCALE`。

*   **软件配置**：
    *   配置为 `INPUT` 模式。
    *   使用 `attachInterrupt` 绑定中断服务函数 (`isr_functions`)，触发模式为 `CHANGE` (电平跳变)。
    *   **工作原理**：通过记录上升沿和下降沿的时间差 (`micros()`) 来计算 PWM 脉宽 (`pwm_value`)，从而解析遥控器的控制量。

### 3.2 执行器输出通道 (GPIO 23, 25)

*   **硬件连接**：
    *   **Steering (GPIO 23)**：连接舵机信号线。
    *   **Throttle (GPIO 25)**：连接电调 (ESC) 信号线。

*   **软件配置**：
    *   使用 ESP32 的 `ledc` 库生成 PWM 信号。
    *   **频率**：50 Hz (标准舵机/电调控制频率)。
    *   **分辨率**：14 bit (计数范围 0-16383)。
    *   **输出映射**：
        *   系统内部计算出的控制量 (-100 ~ 100) 被映射到对应的脉宽范围。
        *   **舵机中位**：`SERVO_MID` (1250) ± `SERVO_RANGE` (440)。
        *   **电机中位**：`MOTOR_MID` (1229) ± `MOTOR_RANGE` (390)。
        *   程序中包含了 `constrain` 限制，确保输出信号在安全范围内 (`PWM_MIN` ~ `PWM_MAX`)。

### 3.3 串行通信接口 (GPIO 16, 17)

*   **硬件连接**：
    *   连接至上位机 (Pilot) 或 RS232 转换模块。
    *   **GPIO 16 (RX)**：接收来自上位机的 `Throttle:Steering\n` 控制指令。
    *   **GPIO 17 (TX)**：仅在 MANUAL 模式下向上位机回传最近一次从串口接收到的控制指令 `T:S\n`（约 60 Hz）；ASSIST / AUTO 模式下关闭 TX 遥测。

*   **软件配置**：
    *   使用 `Serial1` 对象初始化。
    *   波特率：`115200`，配置为 `SERIAL_8N1`。

### 3.4 状态指示灯 (GPIO 5)

*   **硬件连接**：
    *   连接 WS2812B 可编程 RGB LED 灯珠的数据引脚。

*   **软件配置**：
    *   使用 `FastLED` 库驱动。
    *   **颜色定义**：
        *   **绿色**：手动模式 (Manual)。
        *   **黄色**：半自动模式 (Semi-Auto)。
        *   **蓝色**：全自动模式 (Full-Auto)。
        *   **红色闪烁**：紧急停车状态 (Emergency Stop)。

### 3.5 UART 选择控制 (GPIO 12)

*   **硬件连接**：
    *   连接 UART 选择开关或跳线。

*   **软件配置**：
    *   配置为 `OUTPUT` 模式。
    *   默认输出 `LOW`。

### 3.6 I2C 接口 (GPIO 21, 22) - *已验证*

*   **硬件连接**：
    *   GPIO 21 (SDA) 和 GPIO 22 (SCL) 用于连接 I2C 总线设备，如 INA219 (电压电流监测) 和 MPU6050 (IMU)。
*   **状态**：
    *   已通过测试验证为正确的 I2C 引脚配置，相关的 `Wire.begin` 初始化及读取函数已启用。

### 3.7 备用 PWM 输出 (GPIO 32, 33)

*   **硬件连接**：
    *   预留的 PWM 输出通道，可用于扩展其他执行器。

*   **状态**：
    *   当前版本未使用，但已在代码中定义。

## 4. 硬件连接关系图

下图展示了 ESP32 主控与各外部硬件模块的物理连接及逻辑交互关系。

```mermaid
graph TD
    subgraph ESP32_Controller [ESP32 Main Controller]
        direction LR
        P36[GPIO 36<br>(Input Only)]
        P39[GPIO 39<br>(Input Only)]
        P34[GPIO 34<br>(Input Only)]
        P26[GPIO 26]
        P27[GPIO 27]
        P35[GPIO 35<br>(Input Only)]
        
        P23[GPIO 23<br>(PWM Out)]
        P25[GPIO 25<br>(PWM Out)]
        P32[GPIO 32<br>(PWM Out)]
        P33[GPIO 33<br>(PWM Out)]
        
        P16[GPIO 16<br>(RX1)]
        P17[GPIO 17<br>(TX1)]
        
        P5[GPIO 5<br>(Data)]
        P12[GPIO 12<br>(UART SEL)]
        
        P21[GPIO 21<br>(SDA)]
        P22[GPIO 22<br>(SCL)]
    end

    subgraph RC_System [RC Receiver System]
        RC_CH1[Channel 1<br>Steering] -->|PWM Signal| P36
        RC_CH2[Channel 2<br>Throttle] -->|PWM Signal| P39
        RC_CH3[Channel 3<br>Park] -->|PWM Signal| P34
        RC_CH4[Channel 4<br>Mode] -->|PWM Signal| P26
        RC_CH5[Channel 5<br>Drift Switch] -->|PWM Signal| P27
        RC_CH6[Channel 6<br>Drift Scale] -->|PWM Signal| P35
    end

    subgraph Actuators [Actuators]
        P23 -->|PWM 50Hz| Servo[Steering Servo]
        P25 -->|PWM 50Hz| ESC[Electronic Speed Controller]
        ESC -->|Power| Motor[Drive Motor]
    end

    subgraph Upper_Computer [Pilot / Upper Computer]
        P17 -->|RS232 TX| PC_RX[RX]
        PC_TX[TX] -->|RS232 RX| P16
    end

    subgraph Indicators [Indicators]
        P5 -->|WS2812 Protocol| LED[WS2812B RGB LED]
    end

    subgraph I2C_Devices [I2C Devices]
        P21 <-->|I2C Data| INA219[Power Monitor]
        P22 -->|I2C Clock| INA219
        P21 <-->|I2C Data| MPU6050[IMU Sensor]
        P22 -->|I2C Clock| MPU6050
    end

    %% Electrical Flow (Conceptual)
    style P36 fill:#e1f5fe,stroke:#01579b
    style P39 fill:#e1f5fe,stroke:#01579b
    style P34 fill:#e1f5fe,stroke:#01579b
    style P26 fill:#e1f5fe,stroke:#01579b
    style P27 fill:#e1f5fe,stroke:#01579b
    style P35 fill:#e1f5fe,stroke:#01579b
    
    style P23 fill:#fff3e0,stroke:#e65100
    style P25 fill:#fff3e0,stroke:#e65100
    style P32 fill:#fff3e0,stroke:#e65100
    style P33 fill:#fff3e0,stroke:#e65100
    
    style P16 fill:#f3e5f5,stroke:#4a148c
    style P17 fill:#f3e5f5,stroke:#4a148c
```

## 5. MUS4-v2.3 PCB 变更说明

相较于之前的版本，MUS4-v2.3 PCB 进行了以下引脚调整：

| 功能 | 旧版本引脚 | v2.3 版本引脚 | 说明 |
|------|-----------|--------------|------|
| CH1 (转向输入) | - | GPIO 36 | 接收机 CH1 通道 |
| CH2 (油门输入) | - | GPIO 39 | 接收机 CH2 通道 |
| CH3 (停车输入) | - | GPIO 34 | 接收机 CH3 通道 |
| CH4 (模式输入) | GPIO 35 | GPIO 26 | 接收机 CH4 通道变更 |
| CH5 (Drift 开关) | - | GPIO 27 | 接收机 CH5 通道 |
| CH6 (Drift 强度旋钮) | - | GPIO 35 | 接收机 CH6 通道 |
| 转向舵机输出 | GPIO 32 | GPIO 23 | 舵机输出引脚变更 |
| 油门电调输出 | GPIO 33 | GPIO 25 | 电调输出引脚变更 |
| PWM_1 预留 | - | GPIO 32 | 备用 PWM 输出 |
| PWM_2 预留 | - | GPIO 33 | 备用 PWM 输出 |

## 6. 附录

### 6.1 配置注意事项

1.  **输入引脚限制**：GPIO 34, 35, 36, 39 内部没有上拉/下拉电阻。在连接 RC 接收机时，确保接收机能提供稳定的高低电平信号。如果引脚悬空，读取的值将是随机的。
2.  **PWM 频率匹配**：代码中设置的 PWM 频率为 50Hz，这是标准模拟舵机的频率。如果更换为数字舵机或特殊电调，可能需要调整频率。
3.  **电平兼容性**：ESP32 的 I/O 电平为 3.3V。
    *   **RC 接收机**：通常输出 5V PWM，但大多数情况下直接连接到 ESP32 输入引脚是安全的（具体取决于 ESP32 模组的耐受性，建议串联 1kΩ 电阻保护）。
    *   **舵机/电调**：大多数 5V 供电的舵机/电调可以识别 ESP32 的 3.3V 信号为高电平，通常无需电平转换。
4.  **电源管理**：
    *   请勿直接从 ESP32 的 3.3V 或 5V 引脚为舵机或电机供电，这会导致电流过大烧毁开发板。舵机和电机应由独立的 BEC (Battery Elimination Circuit) 供电，并与 ESP32 共地 (GND)。

### 6.2 PWM 参数配置

| 参数 | 舵机 (Steering) | 电调 (Throttle) |
|------|----------------|----------------|
| 中位值 | 1250 | 1229 |
| 范围 | ±440 | ±390 |
| 最小限制 | 819 | 819 |
| 最大限制 | 1638 | 1638 |
| 频率 | 50Hz | 50Hz |
| 分辨率 | 14bit | 14bit |

---
*文档版本：v2.3*
*基于固件版本：MUS4-v2.3 PCB*
*更新日期：2026-03-08*
