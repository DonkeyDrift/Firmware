

# LP-MU-S4

## 项目介绍

LP-MU-S4 是一款基于 ESP32 主控的扩展板控制系统，专为遥控车辆/机器人应用设计。本项目实现了 RC 接收机信号处理、多模式驾驶控制、紧急制动机制以及蓝牙游戏手柄模式等功能。

## 功能特性

- **多通道 RC 信号接收**：支持 4 通道 RC 接收机输入 (GPIO 36, 39, 34, 35)
- **双路执行器输出**：独立控制 2 个执行器通道 (GPIO 32, 33)
- **多种驾驶模式**：支持手动驾驶、自动驾驶等模式切换
- **紧急制动系统**：内置多级安全保护机制，确保系统安全运行
- **蓝牙游戏手柄模式**：可通过蓝牙连接游戏手柄进行控制
- **串行通信**：支持 RS485 串行通信接口 (GPIO 16, 17)
- **状态指示**：LED 状态指示灯 (GPIO 5)

## 硬件配置

### 引脚分配

| 功能 | GPIO | 说明 |
|------|------|------|
| RC CH1 | 36 | 通道1输入 |
| RC CH2 | 39 | 通道2输入 |
| RC CH3 | 34 | 通道3输入 |
| RC CH4 | 35 | 通道4输入 |
| OUT CH1 | 32 | 执行器通道1 |
| OUT CH2 | 33 | 执行器通道2 |
| TX | 16 | 串口发送 |
| RX | 17 | 串口接收 |
| LED | 5 | 状态指示灯 |
| SDA | 13 | I2C 数据 (预留) |
| SCL | 14 | I2C 时钟 (预留) |

## 软件架构

```
mus4/
├── mus4.ino          # 主程序入口
├── sketch.yaml       # Arduino 项目配置
└── Doc/
    ├── Arch/         # 架构文档
    │   └── architecture.md
    ├── Hardware/     # 硬件文档
    │   └── pin_definitions.md
    └── README/       # 开发文档
        ├── ArduinoCLI.md
        └── DevNote.md
```

### 核心模块

- **信号输入处理**：解析 RC 接收机 PWM 信号
- **状态机管理**：控制系统运行状态和模式切换
- **输出控制**：将输入信号映射到执行器输出
- **紧急制动**：检测异常状态并触发安全保护

## 快速开始

### 环境要求

- Python 3.7+
- Arduino CLI
- ESP32 开发板支持

### 安装步骤

1. 安装 Arduino CLI：
   ```bash
   # Linux/macOS
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

   # Windows
   choco install arduino-cli
   ```

2. 安装 ESP32 开发板：
   ```bash
   arduino-cli core install esp32:esp32
   ```

3. 配置项目：
   ```bash
   # 编辑 config.yaml 配置参数
   ```

### 构建与上传

使用提供的自动化脚本：

```bash
python arduino-cli.py --config config.yaml compile
python arduino-cli.py --config config.yaml upload
python arduino-cli.py --config config.yaml monitor
```

## 使用说明

1. **连接硬件**：按照 pin_definitions.md 连接 RC 接收机和执行器
2. **配置模式**：通过 RC 通道切换驾驶模式
3. **启动系统**：上电后系统自动初始化
4. **状态监控**：通过 LED 指示灯判断系统状态

### 模式说明

- **手动模式**：直接通过 RC 遥控器控制
- **自动模式**：执行预设的自动化任务
- **紧急模式**：触发紧急制动，停止所有输出

## 开发文档

详细的开发文档请参考以下文件：

- [系统架构](mus4/Doc/Arch/architecture.md)
- [硬件引脚定义](mus4/Doc/Hardware/pin_definitions.md)
- [ArduinoCLI 使用说明](mus4/Doc/README/ArduinoCLI.md)
- [开发笔记](mus4/Doc/README/DevNote.md)
- [蓝牙游戏手柄模式](./.trae/documents/Enable ESP32 Bluetooth Gamepad Mode.md)

## 参与贡献

1. Fork 本仓库
2. 新建 Feat_xxx 分支
3. 提交代码
4. 新建 Pull Request

## ChangeLog

### 2026-05-30 v1.5.14

- 固件次版本号从 `v1.5.13` 更新到 `v1.5.14`。
- 新增 Web Console Tub JSON 连续记录与浏览器下载功能，便于采集 CH1-CH6 等遥测样本交给模型分析。

### 2026-05-30 v1.5.13

- 固件次版本号从 `v1.5.12` 更新到 `v1.5.13`。
- 保留当前已验证的 WebSocket 曲线实时显示能力，便于通过 HTTP OTA 发布当前稳定固件。

## 许可证

本项目遵循 MIT 许可证。