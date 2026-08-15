# Firmware

DonkeyDrift 项目的固件仓库，与上位机软件仓库 [DonkeyDrift](https://github.com/DonkeyDrift/DonkeyDrift)（DonkeyDrifter）配套：本仓库负责车上的底层实时控制，上位机仓库负责驾驶算法、数据采集、训练与 Web UI。

## 子项目

| 子项目 | 说明 | 当前版本 |
| --- | --- | --- |
| [MUS4_FW/](MUS4_FW/) | MUS4（LP-MU-S4）ESP32 + Arduino 遥控车/机器人底层控制固件 | v1.7.72 |

固件版本号以 [`MUS4_FW/libraries/mus4_core/src/BuildInfo.h`](MUS4_FW/libraries/mus4_core/src/BuildInfo.h) 为准，发布记录见 [`MUS4_FW/CHANGELOG.md`](MUS4_FW/CHANGELOG.md)。

## MUS4_FW 速览

- **RC PWM 输入采集**：CH1-CH6，覆盖转向、油门、Park、模式选择、漂移辅助开关与比例。
- **Pilot 串口控制**：支持 `Throttle:Steering` 帧、序号帧与校验帧，与上位机 DonkeyDrifter 对接。
- **三种驾驶模式**：手动 / 半自动 / 全自动的控制混合。
- **Park / 紧急制动**：安全状态机，可覆盖油门输出并驱动 LED 指示。
- **Drift Assist**：基于 IMU 横摆角速度的转向补偿。
- **Wi-Fi / TCP / Web Console（Drifter Console）**：无线命令台、状态页、日志、图表与 WebSocket 遥测。
- **OTA 更新**：ArduinoOTA 与 Web Console HTTP `/update` 两条通道。
- **I2C 传感器**：INA219 电流/电压、MPU6050 IMU 采样。
- **BLE 手柄输出**：仅在编译期未启用 Wi-Fi Console 时可用。

### 构建与刷写

```bash
cd MUS4_FW

# 编译
python arduino-cli.py -c --sketch MUS4_FW.ino

# 编译 + ArduinoOTA 上传
python arduino-cli.py -c --ota --ota-host <设备IP>

# 或走 Web Console HTTP OTA（设备已联网时）
curl -F "update=@build/MUS4_FW.ino.bin" "http://<设备IP>/update?auth="
```

### 测试

```bash
cd MUS4_FW
pytest tests/
```

硬件引脚、串口协议、Wi-Fi AP/STA 生命周期、控制台权限模型、训练与 Pilot 推理工具等详细说明，见 [`MUS4_FW/README.md`](MUS4_FW/README.md)（[中文文档](MUS4_FW/README.zh-CN.md)）与 [`MUS4_FW/docs/`](MUS4_FW/docs/) 目录。

## 仓库布局

- [`MUS4_FW/`](MUS4_FW/)：当前唯一在役子项目——固件主 sketch（`MUS4_FW.ino`）、模块化本地 Arduino 库（`libraries/mus4_*`）、构建/OTA 脚本、工具与 Python 测试、文档。
- 仓库根级 `.gitignore` 仅覆盖 OS / 编辑器级别的全局垃圾；各子项目维护自己的 `.gitignore`。

## 安全说明

本仓库固件直接驱动转向舵机与电调。凡涉及输出映射、Park / 紧急制动、模式混合或无线控制入口的改动，都必须保持 PWM 限幅、权限检查与失效保护行为不变；串口、Web Console 与 TCP Console 的输入一律视为不可信边界。

## 相关仓库

- [DonkeyDrift](https://github.com/DonkeyDrift/DonkeyDrift)：上位机 Python 自动驾驶/漂移平台（DonkeyDrifter），含统一 Web UI 与 Launcher 服务。
