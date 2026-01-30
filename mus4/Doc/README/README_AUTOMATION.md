# Arduino 自动化构建工具

## 简介
这是一个用于自动化管理 Arduino (ESP32) 项目编译、上传和监控的 Python 脚本。
它提供了命令行接口，支持配置文件，并集成了美观的**米字型旋转加载动画**，提升用户体验。

## 功能特性
- **自动编译**: 调用 `arduino-cli` 编译 Sketch。
- **自动上传**: 自动检测端口并上传固件。
- **串口监控**: 打开串口监视器。
- **流程组合**: 支持单一操作或组合操作 (如 编译+上传+监控)。
- **配置管理**: 通过 `config.yaml` 管理默认参数。
- **视觉反馈**: 命令行进度条和旋转动画，实时显示任务状态。
- **跨平台**: 支持 Windows, Linux, macOS。

## 快速开始

### 1. 依赖
确保已安装 Python 3 和 `arduino-cli`。
```bash
pip install pyyaml
```

### 2. 配置
编辑 `config.yaml` 设置您的板型和端口。

### 3. 使用示例

**仅编译:**
```bash
python3 automation.py -c
```

**编译并上传 (显示进度动画):**
```bash
python3 automation.py -cu
```

**一键全流程 (编译 + 上传 + 监控):**
```bash
python3 automation.py -cum
```

**指定端口:**
```bash
python3 automation.py -cum -p /dev/ttyUSB0
```

## 参数说明
- `-c, --compile`: 编译项目
- `-u, --upload`: 上传固件
- `-m, --monitor`: 打开串口监控
- `-p, --port`: 指定串口设备
- `-b, --baud`: 指定波特率 (默认 115200)
- `--fqbn`: 指定板型 (默认 esp32:esp32:esp32)

## 故障排查
日志文件保存在 `mus4/automation.log`。
如果遇到 `Permission denied` 错误，请检查串口权限 (`sudo chmod 666 /dev/ttyACM*`)。
