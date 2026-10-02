# RC_BLE_Bridge —— CT-8B 蓝牙手柄桥

把 HOT RC CT-8B 枪控（经 F-08A 接收机）桥接成标准 BLE HID 手柄，让 Mac/PC 能用它操控 DD（DonkeyDrift）页面上的模拟器。

## 为什么需要它

CT-8B 本身**没有蓝牙、没有 USB、没有任何电脑接口**——它是纯 2.4GHz FHSS 枪控，只跟自家接收机通信。要让电脑用上它，必须有个东西读接收机的 PWM 输出再转发出去。本固件就是这块桥：ESP32 读 CH1/CH2 脉宽 → 伪装成 BLE HID Gamepad。

> 注：MUS4_FW 车端固件里也有一套 BLE Gamepad 模式（`ENABLE_GAMEPAD_MODE`），但它与 WiFi Console 互斥且车端 flash 已满（min_spiffs 1.9MB 分区，实测共存超 2%），无法在车上默认启用——所以桥接功能做成独立示例固件。

## 硬件

| 物品 | 说明 |
|------|------|
| ESP32 开发板 | 普通 ESP32 即可（不必 S2/S3） |
| F-08A 接收机 | CT-8B 套件自带那只；可临时拆用车上的，或另购一只与 CT-8B 对码（不影响车上已有绑定） |
| 杜邦线 4 根 | 见下表 |

接线（F-08A → ESP32）：

| 接收机 | ESP32 |
|--------|-------|
| VCC | 5V（VIN） |
| GND | GND（必须共地） |
| CH1（转向） | GPIO36 |
| CH2（油门） | GPIO39 |

## 编译与刷机

在 `MUS4_FW` 目录下（复用仓库 vendored 的 ESP32-BLE-Gamepad + NimBLE-Arduino 库）：

```bash
# 编译
arduino-cli compile --fqbn esp32:esp32:esp32 \
  --libraries libraries --build-path build-bridge examples/RC_BLE_Bridge

# 有线刷机（换成实际串口）
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 \
  --libraries libraries examples/RC_BLE_Bridge
```

## 使用

1. ESP32 上电，接收机通电，CT-8B 开机（确认已与该接收机对码）。
2. Mac：系统设置 → 蓝牙 → 配对 **`Gamepad MU02`**。
3. Chrome 打开 DD 页面（本机 `http://<IP>:8000`）→ 驾驶页 → 输入源选「手柄」→ 按一下手柄让浏览器暴露设备。
4. 默认 **z-axis 预设**直接可用（转向=右摇杆 X、油门=左摇杆 Y 反向）；若有偏差，在手柄设置面板校准一次（按设备记忆，以后免配）。
5. 驾驶目标选「模拟器」，开打。

## 行为参数

- 轴量程 0..32767，中位 16384；按校准点分段线性映射（`RC_*_MIN/MID/MAX`，与车端 `FirmwareConfig.h` 的 CT-8B 实测校准一致）。
- 失控保护：通道脉宽越界（<800µs 或 >2200µs）或 1 秒无有效脉冲 → 该轴回中位。
- 发送节奏 50Hz；仅在有主机连接时发送。
- 串口（115200）每 500ms 打印一次两路脉宽，方便脱离电脑核对信号。
