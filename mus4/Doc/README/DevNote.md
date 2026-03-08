# 开发笔记

## 1. 开发环境配置

### 1.1 系统配置
- 配置Ubuntu系统
    - 用户名：dkc
    - 密码：donkeycar
- 安装Donkeycar 5.2.0
- 安装dfrobot_firebeetle2_esp32e的Arduino-cli开发环境
- 配置sketch.yaml文件
    - default_fqbn: esp32:esp32:dfrobot_firebeetle2_esp32e
    - default_port: /dev/ttyS4

### 1.2 串口通信
- 外置USB串口号为 /dev/ttyACM1  上传[Y]，数据 [Y]
- 内部USB串口号为 /dev/ttyS4    上传[Y]，数据 [N]

## 2. 固件程序说明

### 2.1 下载速率
- 固件程序下载速率为 **115200**

### 2.2 串口协议
- 协议格式：`T:S\n`
  - `T` 代表 Throttle（油门值）
  - `S` 代表 Steering（转向值）
  - 结尾为 `\n`（换行符）
- 示例：`10:20\n` 表示油门 10，转向 20

## 3. MUS4-v2.3 PCB 引脚变更记录

### 3.1 变更说明
针对 MUS4-v2.3 PCB 调整了部分引脚定义：

| 功能 | 引脚定义 | 说明 |
|------|----------|------|
| CH1_PIN | 36 | 接收机 PWM 输入 CH1 通道（转向） |
| CH2_PIN | 39 | 接收机 PWM 输入 CH2 通道（油门） |
| CH3_PIN | 34 | 接收机 PWM 输入 CH3 通道（停车） |
| CH4_PIN | 26 | 接收机 PWM 输入 CH4 通道（模式） |
| STEERING_PIN | 23 | CH1 转向舵机输出 |
| THROTTLE_PIN | 25 | CH2 油门电调输出 |
| PWM_1 | 32 | PWM 输出 1 号通道（预留） |
| PWM_2 | 33 | PWM 输出 2 号通道（预留） |

### 3.2 注意事项
- GPIO 34、36、39 为 ESP32 仅输入引脚（Input Only）
- 舵机和电调 PWM 输出已变更到 GPIO 23 和 25

## 4. 功能模块说明

### 4.1 驾驶模式
程序支持三种驾驶模式：

| 模式 | 宏定义 | 说明 | LED 颜色 |
|------|--------|------|----------|
| 0 | CAR_MODE_MANUAL | 手动模式（RC 控制） | 绿色 |
| 1 | CAR_MODE_SEMI_AUTO | 半自动模式（Pilot 转向 + RC 油门） | 黄色 |
| 2 | CAR_MODE_FULL_AUTO | 自动驾驶模式（Pilot 控制） | 蓝色 |

### 4.2 停车/解锁控制
- **锁定（进入停车模式）**：按住 CH3 按钮 0.5 秒
- **解锁（退出停车模式）**：按住 CH3 按钮 1 秒
- 停车状态下 LED 会闪烁（当前模式颜色 + 红色）

### 4.3 紧急停车状态机
当触发停车信号时，系统进入紧急停车状态机：
1. **EST_IDLE**：空闲状态
2. **EST_READY**：准备刹车（500ms，油门设为 15）
3. **EST_BRAKING**：全力刹车（1500ms，油门设为 -100）
4. **EST_DONE**：刹车完成（油门归零）

### 4.4 蓝牙手柄模式
- 定义 `ENABLE_GAMEPAD_MODE` 宏可启用蓝牙手柄功能
- 使用 `BleGamepad` 库将 RC 信号转换为蓝牙手柄输入
- 设备名称：`Gamepad MU03`
- 映射关系：
  - CH1（转向）→ Right Thumb X
  - CH2（油门）→ Left Thumb Y

## 5. 调试与开发

### 5.1 调试输出
- 取消注释 `#define DEBUG` 可启用调试输出
- 调试信息将通过 USB 串口输出

### 5.2 已禁用功能
当前版本以下功能已注释屏蔽：
- ESP-NOW 无线通信
- WiFi 功能
- MPU6050 IMU 传感器
- INA219 电压电流监测
- I2C 总线初始化

### 5.3 注意事项
- 为测试接收机，当前版本已启用模式选择和停车功能
- 系统默认启动时处于停车锁定状态（Park Locked）
- 首次使用需要长按 CH3 按钮 1 秒解锁

## 6. 关键常量配置

| 常量 | 值 | 说明 |
|------|-----|------|
| PWM_MIN | 819 | PWM 最小计数值 |
| PWM_MAX | 1638 | PWM 最大计数值 |
| MOTOR_MID | 1229 | 电机中位值 |
| MOTOR_RANGE | 390 | 电机控制范围 |
| SERVO_MID | 1250 | 舵机中位值 |
| SERVO_RANGE | 440 | 舵机控制范围 |
| PARK_LOCK_HOLD_TIME | 500ms | 锁定所需按住时间 |
| PARK_UNLOCK_HOLD_TIME | 1000ms | 解锁所需按住时间 |
| EMERGENCY_STOP_READY_DURATION | 500ms | 紧急停车准备时间 |
| EMERGENCY_STOP_BRAKE_DURATION | 1500ms | 紧急停车刹车时间 |

---
*文档版本：v2.3*
*基于固件版本：MUS4-v2.3 PCB*
*更新日期：2026-03-08*
