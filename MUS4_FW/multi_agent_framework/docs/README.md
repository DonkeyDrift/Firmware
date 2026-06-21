# 嵌入式系统多智能体协作开发框架 (Embedded Multi-Agent Framework)

该框架旨在通过多个专业化智能体（Agents）的协作，自动化管理 ESP32 固件开发、Linux Shell 脚本编写以及 Web 界面交互测试的复杂流程。

## 核心架构与功能模块

本系统实现了 5 个核心智能体，并通过 **Python Queue (消息队列)** 实现进程/线程间的通信 (IPC)。

### 1. 需求分析智能体 (RequirementAgent)
*   **职责**：解析自然语言需求，拆解为技术规格。
*   **处理内容**：识别出 ESP32 需要实现的 WiFi AP、GPIO、传感器；Linux 需要的系统监控、部署；Web 需要的 WebSocket 和 UI。

### 2. 任务规划智能体 (PlanningAgent)
*   **职责**：根据依赖关系制定甘特图/任务队列。
*   **处理内容**：优先调度底层的 ESP-IDF FreeRTOS 初始化和 WiFi 协议栈配置，然后是 Linux 网络配置和系统服务，最后安排 Web UI 层的开发。

### 3. 代码开发智能体 (DevelopmentAgent)
*   **职责**：生成具体的源代码文件。
*   **处理内容**：
    *   **ESP32 固件**：基于 `ESP-IDF v5.1`，生成 C 语言代码，包含 `esp_wifi` 初始化、`esp_http_server` (WebSocket 支持) 以及 `FreeRTOS` 的多任务传感器数据采集调度。
    *   **Linux 脚本**：生成符合 POSIX 标准的 bash 脚本，实现了严格的错误处理 (`trap ERR`)、性能监控 (CPU/内存) 及日志记录机制。

### 4. 系统集成智能体 (IntegrationAgent)
*   **职责**：进行跨平台软硬件联调。
*   **处理内容**：验证 ESP32 处于 AP 模式时，Linux 客户端和 Web 浏览器能够正确接入其 IP (192.168.4.1)，并确保 WebSocket 全双工通道的数据收发无误。

### 5. 测试调试智能体 (TestingAgent)
*   **职责**：执行自动化测试流程。
*   **处理内容**：运行 ESP32 单元测试（Unity 框架），执行 Linux 脚本集成测试，生成端到端的自动化测试报告，并输出最终的交付件。

---

## 目录结构说明

```text
multi_agent_framework/
├── framework/                 # Python 多智能体核心运行环境
│   ├── core.py                # 智能体基类及 IPC 队列通信封装
│   ├── agents.py              # 5 大智能体业务逻辑实现
│   └── main.py                # 系统入口与任务下发
├── esp32_firmware/            # 智能体生成的 ESP32 固件 (ESP-IDF 架构)
│   └── main/
│       └── main.c             # FreeRTOS、WiFi AP 及 WebSocket 服务源码
├── linux_scripts/             # 智能体生成的 Linux 维护脚本
│   └── sys_monitor.sh         # POSIX 标准的系统资源监控与日志脚本
├── web_ui/                    # 智能体生成的 Web 控制端
│   └── index.html             # 基于 WebSocket 的实时交互控制面板
└── docs/                      # 生成的系统文档
    └── README.md              # 架构说明与部署指南
```

## 部署与运行指南

### 1. 运行多智能体模拟框架
在任意支持 Python 3 的环境中执行：
```bash
cd framework
python3 main.py
```
您将在控制台看到各智能体之间基于消息队列的协作过程。

### 2. 编译并烧录 ESP32 固件
需在安装了 `ESP-IDF` 的环境中执行：
```bash
cd esp32_firmware
idf.py set-target esp32s3   # 根据实际芯片选择
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3. 运行 Linux 监控脚本
在 Lattepanda MU 或 Ubuntu 环境下：
```bash
cd linux_scripts
chmod +x sys_monitor.sh
sudo ./sys_monitor.sh
```

### 4. Web 界面交互
将电脑或手机连接至 ESP32 释放的 WiFi 热点 (`MUS4_AGENT_AP`)，然后双击打开 `web_ui/index.html` 即可实现实时控制与状态查看。
