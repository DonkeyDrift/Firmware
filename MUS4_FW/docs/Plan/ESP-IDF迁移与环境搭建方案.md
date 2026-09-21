# MUS4_FW 迁移到 ESP-IDF 方案与本地编译环境搭建

> 适用环境：aarch64 Ubuntu 24.04（AidLux），中国大陆网络，ESP32（经典款）
> 目标：把 Arduino sketch 形态的 MUS4_FW 固件迁移为可用 `idf.py` 编译/烧录的 ESP-IDF 工程，并在本机搭好工具链。
> 结论先行：推荐“两步走”。**阶段 A** 用 ESP-IDF + arduino-esp32 组件承载现有代码，几乎不改业务代码就能得到 `idf.py build/flash/monitor`；**阶段 B** 再按模块把 Arduino API 换成原生 IDF API。

---

## 1. 当前程序分析

### 1.1 工程结构

| 部分 | 内容 | 规模 |
| --- | --- | --- |
| 主 sketch | `MUS4_FW.ino`：全局状态、`setup()`/`loop()`、WiFi/OTA 运行时、Serial1 上行帧、RC 滤波编排、TUI 刷新 | 927 行 |
| 业务模块 | `libraries/mus4_*`（core/rc/control/safety/command/log/ui/i2c/web/wifi/auth/cloud/diag） | 约 13,100 行 |
| 第三方 Arduino 库 | FastLED、Adafruit GFX/BusIO/INA219/MPU6050/Unified_Sensor/NeoPixel/SSD1306、Async_TCP、ESP_Async_WebServer、ESP32-BLE-Gamepad、NimBLE-Arduino | 约 407,000 行（随仓库 vendored） |
| 构建 | `arduino-cli`，FQBN `esp32:esp32:esp32:PartitionScheme=min_spiffs`，烧录 115200 | `config.yaml` / `sketch.yaml` / `wslbuild.yaml` |

业务代码真正“框架绑定”的部分远小于总行数：纯逻辑（`mus4_control`、`mus4_safety`、`mus4_rc` 的滤波、`mus4_command` 解析、`mus4_diag`、`mus4_log`）几乎不直接依赖 Arduino；真正需要改写的集中在 WiFi / Web / OTA / 传感器 / 显示 / 蓝牙。

### 1.2 构建与烧录链路（现状）

- `arduino-cli.py`：编译（`-c`）、上传（`-u`）、串口监视（`-s`）、OTA（`--ota`）。
- `arduino-cli-wsl.ps1`：Windows 主机 + WSL 加速编译；`wslbuild.yaml` 配置。
- 本地库优先：构建脚本通过 `--libraries libraries` 让工程内 vendored 库优先于全局库。
- 分区方案 `min_spiffs`（4MB Flash）：保留双 app 分区以支持 OTA。

### 1.3 Arduino API 使用面（仅统计工程代码：`MUS4_FW.ino` + `libraries/mus4_*`）

| API | 引用次数 | 主要位置 |
| --- | --- | --- |
| `String` | 417 | 全工程（协议解析、JSON、URL、日志） |
| `millis()/micros()/delay()` | 134 | 全工程 |
| `WiFi.*` | 95 | `mus4_wifi`、`mus4_web`、`mus4_cloud` |
| `FastLED/CRGB` | 38 | `mus4_ui/LedStatus.*`、`ControlMixer.cpp`、`MUS4_FW.ino` |
| `Serial/Serial1/Serial2` | 34 | `MUS4_FW.ino`、`mus4_log`、`mus4_command` |
| `Preferences`（NVS） | 26 | wifi/control/auth/core/web |
| `WebServer` / `AsyncWebServer` + `AsyncWebSocket` | 24 | `mus4_web/WebConsoleServer.cpp`、`WebTelemetry.*` |
| `ArduinoOTA` | 20 | `MUS4_FW.ino`、`mus4_wifi/WifiOta.cpp` |
| `esp_*` 原生 IDF API | 19 | `mus4_auth`（efuse）、`mus4_wifi`（partition/ota）、`mus4_rc`（mcpwm_cap）、`WifiManager`（esp_wifi） |
| `Update.*` | 14 | `mus4_web/WebConsoleServer.cpp`、`mus4_wifi/WifiOta.cpp` |
| `ledc*` | 9 | `mus4_safety/ActuatorOutput.cpp`（舵机/电调 300Hz 14bit）、`mus4_ui/Buzzer.cpp` |
| `Adafruit_*` 传感器 | 8 | `mus4_i2c/Sensors.cpp`、`I2CBusTools.cpp` |
| `BleGamepad/NimBLE` | 4 | `mus4_diag/GamepadMode.cpp` |
| `attachInterrupt` | 1 处（6 个 ISR） | `mus4_rc/RcPwmCapture.cpp` |

### 1.4 已经存在的原生 IDF 代码（迁移的有利因素）

- `mus4_auth/AuthService.cpp`：`esp_efuse_mac_get_default()`。
- `mus4_wifi/WifiOta.cpp`：`esp_ota_get_running_partition()` / `esp_ota_get_next_update_partition()` / `esp_ota_get_state_partition()` / `esp_partition_erase_range()`。
- `mus4_rc/RcPwmCapture.cpp`：已写好可选的 `mcpwm_cap` 输入捕获路径（由 `ENABLE_RC_MCPWM_CAPTURE` 控制，当前默认 0，走 `attachInterrupt`）。
- `mus4_wifi/WifiManager.cpp`：`esp_wifi_disconnect()` 等。
- `MUS4_FW.ino`：`esp_ota_mark_app_valid_cancel_rollback()`。

也就是说，**这不是从零迁移**，而是把一个已模块化的 Arduino 工程切成 IDF 组件。

### 1.5 迁移的主要难点

1. `String` 417 处、`millis/micros/delay` 134 处，是全局性改写（阶段 B 成本最高）。
2. WiFi AP/STA 互斥状态机、Web 配置页（60+ 路由）、AsyncWebSocket 遥测、双 OTA 通道（ArduinoOTA + HTTP `/update`）、captive portal，是最重的部分。
3. Arduino 的 `setup()/loop()` 单线程模型要映射到 FreeRTOS 任务 + `esp_timer`，并处理看门狗与线程安全。
4. 第三方库（FastLED、ESPAsyncWebServer、NimBLE、Adafruit）在纯 IDF 下需要找等价组件或保留 Arduino 组件。

---

## 2. 迁移路线

### 2.1 阶段 A：ESP-IDF + arduino-esp32 组件（推荐先做，1～2 天）

把 Arduino core 当作 IDF 的一个组件，业务代码基本不动，立刻获得 `idf.py` 工具链。**这是“本地 IDF 编译和下载”落地的最快路径。**

版本配对（已核对组件注册表）：

| arduino-esp32 | 要求的 ESP-IDF |
| --- | --- |
| 3.3.12 | `>=5.3,<6.2` |
| 3.3.0 | `>=5.3,<5.6` |
| 3.2.0 | `>=5.3,<5.5` |
| 3.1.0 | `>=5.1,<5.2` |
| 3.0.0 | `>=5.1` |

当前 sketch 使用了 core 3.3.x 的 `ledcAttachChannel` / `ledcWriteChannel` / `ledcChangeFrequency`，因此选 **ESP-IDF v5.4.x（本机装的是 v5.4.4）+ arduino-esp32 3.3.x**。

目标目录结构：

~~~
MUS4_FW-idf/
├── CMakeLists.txt              # 工程入口
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml       # 声明 arduino-esp32 依赖
│   ├── main.cpp                # initArduino + setup/loop 适配层
│   └── MUS4_FW.ino             # 原 sketch（用 #include 方式编译）
└── components/                 # 本地库包装成 IDF 组件
    ├── mus4_core/ ... mus4_wifi/
    └── Adafruit_* / FastLED / ESP_Async_WebServer / ...
~~~

`CMakeLists.txt`：

~~~cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mus4_fw)
~~~

`main/idf_component.yml`：

~~~yaml
dependencies:
  idf: ">=5.3,<5.6"
  espressif/arduino-esp32: "^3.3.10"
~~~

`main/CMakeLists.txt`：

~~~cmake
idf_component_register(
  SRCS "main.cpp"
  INCLUDE_DIRS "."
  REQUIRES arduino-esp32
)
~~~

`main/main.cpp`（适配 `setup()/loop()`）：

~~~cpp
#include <Arduino.h>

void setup();
void loop();

extern "C" void app_main(void)
{
    initArduino();
    setup();
    for (;;) {
        loop();
        vTaskDelay(pdMS_TO_TICKS(1));   // 对应原 sketch 末尾的 delay(5)
    }
}
#include "MUS4_FW.ino"                 // 让预处理器把 sketch 当 .cpp 编译
~~~

> `.ino` 在 IDF 里不会被编译。两种做法：① 把 `MUS4_FW.ino` 改名 `MUS4_FW.cpp` 并在顶部 `#include <Arduino.h>`；② 保留 `.ino`，在 `main.cpp` 里 `#include "MUS4_FW.ino"`（示例采用后者）。注意 Arduino 会自动生成函数原型，IDF 不做这件事；`MUS4_FW.ino` 现有函数定义顺序基本自洽，若编译报“未声明”就补前向声明。

`partitions.csv`（对齐 Arduino `min_spiffs`，4MB）：

~~~
# Name,   Type, SubType, Offset,   Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xe000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x1E0000,
app1,     app,  ota_1,   0x1F0000, 0x1E0000,
spiffs,   data, spiffs,  0x3D0000, 0x30000,
~~~

`sdkconfig.defaults`（最小集）：

~~~
CONFIG_IDF_TARGET="esp32"
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_ESPTOOLPY_BAUD_115200B=y
~~~

本地 `libraries/` 的第三方库需要包成 IDF 组件：优先用组件管理器（`idf_component.yml` 里声明 `espressif/arduino-esp32` 等），其余没有 IDF 元数据的库（FastLED、Adafruit_*、mus4_*）在 `components/` 下为每个库生成一个 `CMakeLists.txt`，把源码与 include 目录注册进去（例如 `components/mus4_rc/CMakeLists.txt` 指向 `libraries/mus4_rc/src`）。

### 2.2 阶段 B：逐模块原生 IDF 化（推荐顺序）

1. 先换“无框架”基础设施（低风险）：`millis/micros/delay` → `esp_timer_get_time()` / `vTaskDelay()`；`String` → `std::string` 或定长 `char[]` + `snprintf`。
2. GPIO/中断/PWM：`pinMode/digitalWrite` → `gpio_config` / `gpio_set_level`；`attachInterrupt` → `gpio_install_isr_service` + `gpio_isr_handler_add`；`ledc*` → `driver/ledc.h`；RC 捕获切到已写好的 `mcpwm_cap` 路径。
3. NVS：`Preferences` → `nvs_flash_init` + `nvs_open/nvs_get_*/nvs_set_*`。
4. I2C 传感器：`Wire` + Adafruit → `driver/i2c_master.h`（IDF 5.2+ 新驱动），MPU6050/INA219 用寄存器级读写；或暂时保留 Adafruit 组件。
5. WS2812：FastLED → `led_strip`（RMT）。
6. WiFi/网络：`WiFi.h` → `esp_netif` + `esp_event` + `esp_wifi` + `esp_wifi_scan`；`ESPmDNS` → `mdns` 组件；`DNSServer` → 自建 UDP:53 captive portal。
7. HTTP：同步 `WebServer.h` → `esp_http_server`；`ESPAsyncWebServer`+WebSocket → `esp_http_server` 的 `httpd_ws`；`HTTPClient/WiFiClientSecure` → `esp_http_client` + `esp-tls`。
8. OTA：`ArduinoOTA` → `esp_ota_ops` + 自建 TCP 服务（或 `esp_https_ota`）；`Update.h` 的 `/update` → `esp_ota_begin/write/end` + `httpd`。
9. BLE 手柄：`ESP32-BLE-Gamepad` + `NimBLE-Arduino` → IDF 自带 `esp-nimble`。
10. 串口：`HardwareSerial` → `uart_driver_install` / `uart_read_bytes` / `uart_write_bytes`，或 `usb_serial_jtag`。

### 2.3 Arduino → ESP-IDF API 映射表

| 功能 | 现状（Arduino） | IDF 目标 | 说明 |
| --- | --- | --- | --- |
| GPIO | `pinMode/digitalWrite/digitalRead` | `gpio_config` / `gpio_set_level` / `gpio_get_level` | 直接替换 |
| 外部中断 | `attachInterrupt` + `IRAM_ATTR` | `gpio_install_isr_service` + `gpio_isr_handler_add` | ISR 仍需 `IRAM_ATTR` |
| 高精度计时 | `micros()` / `millis()` | `esp_timer_get_time()` / `xTaskGetTickCount()` | 单位 µs |
| 延时 | `delay()` | `vTaskDelay(pdMS_TO_TICKS())` | loop 需让出 CPU |
| PWM 输出 | `ledcAttachChannel/ledcWriteChannel` | `driver/ledc.h`：`ledc_timer_config` + `ledc_channel_config` + `ledc_set_duty/update_duty` | 300Hz / 14bit |
| 蜂鸣器变频 | `ledcChangeFrequency` | 重配 `ledc_timer_config` 频率或 MCPWM | |
| RC 输入捕获 | `attachInterrupt`（可选 `mcpwm_cap`） | `driver/mcpwm_cap.h` | 代码已就绪，建议启用 |
| NVS | `Preferences` | `nvs.h` / `nvs_flash.h` | 命名空间映射到 NVS namespace |
| 字符串 | `String` | `std::string` / `char[]` + `snprintf` | 417 处，可逐步替换 |
| 串口 | `Serial/Serial1/Serial2` | `uart_driver_install` / `uart_read_bytes` / `uart_write_bytes` | |
| I2C | `Wire` + Adafruit | `driver/i2c_master.h` | |
| WS2812 | FastLED | `led_strip`（RMT） | |
| WiFi | `WiFi.h` | `esp_netif` + `esp_event` + `esp_wifi` | 95 处 |
| mDNS | `ESPmDNS` | `mdns` 组件 | |
| 配网 DNS | `DNSServer` | UDP:53 自建 | |
| Web 配置页 | `WebServer` | `esp_http_server` | |
| WebSocket | `ESPAsyncWebServer`+`AsyncTCP` | `httpd_ws` | |
| Arduino OTA | `ArduinoOTA` | `esp_ota_ops` + 自建服务 | |
| HTTP 上传 OTA | `Update.h` | `esp_ota_begin/write/end` | 与 `httpd` 结合 |
| HTTPS 上报 | `HTTPClient`+`WiFiClientSecure` | `esp_http_client`+`esp-tls` | |
| BLE 手柄 | `ESP32-BLE-Gamepad`+NimBLE-Arduino | `esp-nimble` | |
| 芯片 ID | `esp_efuse_mac_get_default` | 同左 | 已是原生 |

---

## 3. 本地 ESP-IDF 环境搭建（中国大陆网络）

### 3.1 本机网络实测（决定镜像策略）

| 站点 | 结果 | 用途 |
| --- | --- | --- |
| `github.com` / `codeload.github.com` | 直连超时 | 不能用 |
| `pypi.org` | 直连超时（仅解析到 IPv6） | 不能用 |
| `dl.espressif.com` | 302 可用 | 工具链、pip wheel 源 |
| `components.espressif.com` / `components-file.espressif.cn` | 200 可用 | 组件管理器 |
| `gitee.com` | 200 可用 | arduino-esp32 等源码镜像 |
| `jihulab.com/esp-mirror/...` | 可用 | **ESP-IDF 源码 + 子模块镜像** |
| `mirrors.aliyun.com/ubuntu-ports` | 可用（偶发同步中） | apt |
| `pypi.tuna.tsinghua.edu.cn` | 200 可用 | pip |

策略：

1. ESP-IDF 源码走 `https://jihulab.com/esp-mirror/espressif/esp-idf.git`（子模块相对路径会解析到同镜像）。
2. 工具链（xtensa-esp-elf 等）由 `install.sh` 从 `dl.espressif.com` 下载（IDF 默认下载地址）。
3. Python 依赖用 `PIP_INDEX_URL=https://pypi.tuna.tsinghua.edu.cn/simple`（IDF 还会用 `IDF_PIP_WHEELS_URL`，默认 `https://dl.espressif.com/pypi`）。
4. 组件管理器若慢，可设 `IDF_COMPONENT_REGISTRY_URL` 或用 `components-file.espressif.cn` 镜像文件。

### 3.2 系统依赖（Ubuntu 24.04 / aarch64）

~~~bash
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
~~~

> 注意：Aliyun 的 `noble-updates/*/dep11` 偶尔 “Mirror sync in progress” 导致 `apt-get update` 返回 100。重试即可；若仍失败，可临时注释掉 `dep11` 相关报错的源，或直接执行 `apt-get install`（包索引通常已可用）。

### 3.3 获取 ESP-IDF 源码（含子模块）

~~~bash
mkdir -p ~/esp && cd ~/esp
git clone --depth 1 --branch v5.4.4 https://jihulab.com/esp-mirror/espressif/esp-idf.git
cd esp-idf
# 关键：不要用 --recursive，否则会去 github 拉 cmock 的测试子模块 c_exception
git submodule update --init --depth 1 --jobs 4
~~~

> `components/cmock/CMock/vendor/c_exception` 指向 github，属于测试框架，编译固件不需要，因此用非递归方式跳过。若 `git clone --recursive` 卡住，就是卡在这里。

### 3.4 安装工具链

~~~bash
export IDF_TOOLS_PATH=$HOME/.espressif
export PIP_INDEX_URL=https://pypi.tuna.tsinghua.edu.cn/simple
export PATH=/usr/bin:$PATH          # 避免命中其它项目的 venv（本机 PATH 里有 DonkeyDrift/.venv）
cd ~/esp/esp-idf
./install.sh esp32
~~~

### 3.5 激活环境

~~~bash
export IDF_TOOLS_PATH=$HOME/.espressif
export PATH=/usr/bin:$PATH     # 本机 PATH 首位是别的项目 venv(python3.11)，必须让 export.sh 选到系统 python3.12
cd ~/esp/esp-idf
. ./export.sh
~~~

> 常见错误：`export.sh` 会检测 `python3` 并寻找 `$IDF_TOOLS_PATH/python_env/idf5.4_py3.12_env`。若 PATH 里先命中的是别的 venv（本机是 `DonkeyDrift/.venv` → Python 3.11），会报 `idf5.4_py3.11_env not found`；把 `/usr/bin` 放到 PATH 最前即可。
> 另外不要把 `. ./export.sh` 放进管道（如 `. ./export.sh | grep`），否则它在子 shell 里执行，环境变量不会留在当前 shell。

可写入 `~/.bashrc`：

~~~bash
alias get_idf='. $HOME/esp/esp-idf/export.sh'
~~~

### 3.6 编译与烧录（本机 /dev/ttyACM* 为 CH343 双串口）

~~~bash
cd ~/esp/esp-idf/examples/get-started/hello_world
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyACM1 flash monitor
~~~

本机实测 USB 设备为 `1a86:55d2 QinHeng USB_Dual_Serial`，对应 `/dev/ttyACM0` 与 `/dev/ttyACM1`；结合 `config.yaml` 的 `preferred_description_keywords: ["SERIAL-B"]`，下载口通常是 SERIAL-B 对应的 `ttyACM1`（以实际枚举为准，可用 `esptool.py flash_id` 确认）。

### 3.7 常见坑

1. `git clone --recursive` 卡死：只做 `--depth 1` + 非递归 `git submodule update`。
2. `install.sh` 用到错误的 Python：先 `export PATH=/usr/bin:$PATH` 或指定 `IDF_PYTHON_ENV_PATH`。
3. pip 超时：务必设 `PIP_INDEX_URL` 为清华源。
4. 串口权限：把用户加入 `dialout`。
5. 分区/Flash：默认 2MB 的 hello_world 配置要改 `CONFIG_ESPTOOLPY_FLASHSIZE_4MB`，业务工程用自定义分区表。
6. 组件管理器拉 GitHub：设置 `IDF_COMPONENT_REGISTRY_URL` 或改用镜像文件源。

---

## 4. 本机验证记录（2026-09，aarch64 Ubuntu 24.04 / AidLux）

| 项目 | 结果 |
| --- | --- |
| ESP-IDF 版本 | v5.4.4（JihuLab 镜像，23 个子模块已检出） |
| 工具链 | xtensa-esp-elf-gcc 14.2.0（crosstool-NG esp-14.2.0_20260121）；esptool.py 4.12.0 |
| `idf.py --version` | ESP-IDF v5.4.4 |
| 系统 Python | 3.12.3（IDF venv：`~/.espressif/python_env/idf5.4_py3.12_env`） |
| 编译 | `hello_world` 在 `/home/aidlux/esp/projects/hello_world` 用 `idf.py build` 成功 |
| 串口设备 | `1a86:55d2 USB_Dual_Serial` → `/dev/ttyACM0` / `/dev/ttyACM1` |
| 下载口 | `/dev/ttyACM1`（esptool 识别到 ESP32-D0WD-V3 rev v3.1，40MHz，MAC 94:51:dc:48:f5:4c） |
| 另一路 | `/dev/ttyACM0` 不能连接芯片（`No serial data received`），符合 SERIAL-A/B 双通道拓扑 |

烧录命令（会覆盖板上现有固件，确认后再执行）：

~~~bash
cd /home/aidlux/esp/projects/hello_world
. /home/aidlux/esp/esp-idf/export.sh
idf.py -p /dev/ttyACM1 flash monitor
~~~

## 5. 风险与回退

- 阶段 A 会引入 Arduino core，二进制体积与启动行为与 `arduino-cli` 构建略有差异；建议保留 `arduino-cli` 构建作为回退。
- 阶段 B 的 WiFi/Web/OTA 重写风险最高，务必在台架上先验证 Park/急停/遥控链路，再验证无线功能。
- 任何输出映射、Park、急停、模式融合改动都必须保留 PWM 限幅与失效安全路径。
