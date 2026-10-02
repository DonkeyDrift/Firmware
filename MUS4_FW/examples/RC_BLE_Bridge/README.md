# RC_BLE_Bridge —— CT-8B 蓝牙手柄桥

把 HOT RC CT-8B 枪控（经 F-08A 接收机）桥接成标准 BLE HID 手柄，让 Mac/PC 能用它操控 DD（DonkeyDrift）页面上的模拟器。

## 为什么需要它

CT-8B 本身**没有蓝牙、没有 USB、没有任何电脑接口**——它是纯 2.4GHz FHSS 枪控，只跟自家接收机通信。要让电脑用上它，必须有个东西读接收机的 PWM 输出再转发出去。本固件就是这块桥：ESP32 读 CH1/CH2 脉宽 → 伪装成 BLE HID Gamepad。

> 注：MUS4_FW 车端固件里也有一套 BLE Gamepad 模式（`ENABLE_GAMEPAD_MODE`），但它与 WiFi Console 互斥且车端 flash 已满（min_spiffs 1.9MB 分区，实测共存超 2%），无法在车上默认启用——所以桥接功能做成独立固件。

## 用法一：直接刷到车上 ESP32（推荐，零新增硬件）

车上 ESP32 本来就接着 F-08A 接收机（GPIO36/39）、接收机已与 CT-8B 对码，什么都不用拆。整个往返都走 WiFi OTA，**不需要数据线**：

```bash
# 1. 玩模拟器前：把桥刷上车（车端正常固件下执行；编译产物路径见下文"编译"）
curl -F "update=@build-bridge/RC_BLE_Bridge.ino.bin" "http://<车IP>/update?auth="

# 2. Mac 蓝牙配对 "Gamepad MU02"，DD 驾驶页输入源选「手柄」，目标选「模拟器」

# 3. 玩完：把车固件刷回来（桥模式下执行，两条通道任选其一）
curl -F "update=@build/MUS4_FW.ino.bin" "http://<车IP>/update"        # HTTP 通道
# 或 ArduinoOTA：espota.py -r -i <车IP> -p 3232 -P mus4-debug -f build/MUS4_FW.ino.bin
```

桥模式下的行为：

- **车原地不动**：桥固件从不驱动舵机/电调引脚（GPIO23/25 无 PWM 输出）——天然安全，但建议仍架空车轮。
- **Web Console 暂离线**：车端固件此时没在跑，属正常现象；刷回即 100% 复原。
- **真车无遥控**：CT-8B 此时被模拟器占用，物理上本来也不可能同时用。
- **刷回保障**：桥自带 ArduinoOTA（3232，密码 `mus4-debug`）+ HTTP `/update`（80）双通道；连不上家里 Wi-Fi 时 15 秒后自动开兜底 AP `MUS4-RC-Bridge`（192.168.4.1，开放），同样带 `/update`——永远刷得回来。

## 用法二：独立 ESP32 开发板

| 物品 | 说明 |
|------|------|
| ESP32 开发板 | 普通 ESP32 即可（不必 S2/S3） |
| F-08A 接收机 | CT-8B 套件自带那只；或另购一只与 CT-8B 对码（不影响车上已有绑定） |
| 杜邦线 4 根 | 见下表 |

接线（F-08A → ESP32）：

| 接收机 | ESP32 |
|--------|-------|
| VCC | 5V（VIN） |
| GND | GND（必须共地） |
| CH1（转向） | GPIO36 |
| CH2（油门） | GPIO39 |

有线刷机（在 `MUS4_FW` 目录下）：

```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 \
  --libraries libraries examples/RC_BLE_Bridge
```

## 编译

在 `MUS4_FW` 目录下（复用仓库 vendored 的 ESP32-BLE-Gamepad + NimBLE-Arduino 库）：

```bash
# 分区方案与车端一致（min_spiffs），编译产物在 build-bridge/
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs \
  --libraries libraries --build-path build-bridge examples/RC_BLE_Bridge
```

WiFi 凭据：把本机 `libraries/mus4_core/src/WirelessSecrets.h` 复制一份到 `examples/RC_BLE_Bridge/WirelessSecrets.h` 即可参与编译（该文件名已被 .gitignore 全局忽略，不入库）；没有该文件也能编译——STA 连不上时 15 秒后自动走兜底 AP（`MUS4-RC-Bridge`，192.168.4.1）。

## 配对与驾驶（Mac）

1. ESP32 上电，接收机通电，CT-8B 开机。
2. Mac：系统设置 → 蓝牙 → 配对 **`Gamepad MU02`**。
3. Chrome 打开 DD 页面（本机 `http://<IP>:8000`）→ 驾驶页 → 输入源选「手柄」→ 按一下手柄让浏览器暴露设备。
4. 默认 **z-axis 预设**直接可用（转向=右摇杆 X、油门=左摇杆 Y 反向）；若有偏差，在手柄设置面板校准一次（按设备记忆，以后免配）。
5. 驾驶目标选「模拟器」，开打。

## 行为参数

- 轴量程 0..32767，中位 16384；按校准点分段线性映射（`RC_*_MIN/MID/MAX`，与车端 `FirmwareConfig.h` 的 CT-8B 实测校准一致，由 `tests/test_rc_ble_bridge_example.py` 钉住）。
- 失控保护：通道脉宽越界（<800µs 或 >2200µs）或 1 秒无有效脉冲 → 该轴回中位。
- 发送节奏 50Hz；仅在有主机连接时发送。
- 串口（115200）每 500ms 打印一次两路脉宽，方便脱离电脑核对信号。
