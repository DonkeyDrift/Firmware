# 无线 OTA 与无线串口调试方案

## 背景

MUS4 当前主固件已经具备 USB `Serial`、RS232 `Serial1` 和 BLE Gamepad 能力，但尚未在主控制链路中集成 Wi-Fi、OTA 或无线串口调试能力。

当前相关入口：

- `mus4.ino` 初始化 USB `Serial` 和 `Serial1`。
- `loop()` 中分别读取 USB 串口和 RS232 串口命令。
- 当前已定义 `ENABLE_GAMEPAD_MODE`，并使用 `BleGamepad` 输出 BLE 手柄数据。

无线能力的设计目标是：在不破坏现有 RC PWM、Pilot、Park、紧急制动、安全状态机和 TUI 的前提下，增加 OTA 升级和无线调试通道。

## 总体结论

推荐采用：

- **Wi-Fi 作为 OTA 主通道**。
- **Wi-Fi TCP Console 作为主要无线串口调试通道**。
- **BLE UART 作为配网、现场低速调试和 OTA 开启辅助通道**。
- **BLE Gamepad 与 BLE UART 初期采用编译期开关二选一**，稳定后再评估共存。

核心原则：

```text
无线只扩展通道，不改变车辆控制安全逻辑。
OTA 只在 Park 锁定、人工授权、短时间窗口内启用。
BLE 优先用于配网和低速调试，Wi-Fi 负责 OTA 和主要无线串口。
```

## 推荐架构

```text
                 ┌──────────────────────┐
                 │      USB Serial       │
                 └──────────┬───────────┘
                            │
                 ┌──────────▼───────────┐
                 │    Command Router     │
                 └──────┬───────┬───────┘
                        │       │
       ┌────────────────▼─┐   ┌─▼────────────────┐
       │   Serial1 RS232   │   │ Wireless Console │
       └───────────────────┘   └──────┬───────────┘
                                      │
                         ┌────────────▼────────────┐
                         │ Wi-Fi TCP / WebSocket    │
                         │ BLE UART                 │
                         └────────────┬────────────┘
                                      │
                         ┌────────────▼────────────┐
                         │ Wireless OTA             │
                         │ ArduinoOTA / HTTP OTA    │
                         └─────────────────────────┘
```

建议新增模块：

```text
WirelessConfig.h/.cpp      # Wi-Fi/BLE 开关、SSID、密码、鉴权配置
WirelessOta.h/.cpp         # OTA 初始化、OTA 状态机、安全门控
WirelessConsole.h/.cpp     # 无线串口：TCP/WebSocket/BLE UART 命令桥接
```

主循环只保留轻量调用：

```cpp
wirelessConsole.update();
wirelessOta.update();
```

## Wi-Fi OTA 方案

### 开发期方案：ArduinoOTA

适合实验室和开发阶段：

```text
电脑 arduino-cli / Arduino IDE
        ↓
局域网 OTA
        ↓
ESP32
```

优点：

- 开发体验好。
- 可以直接通过网络上传。
- 不需要自定义上传页面。

限制：

- 依赖同一局域网。
- 鉴权能力有限。
- 不适合长期暴露在开放网络中。

### 产品化方案：HTTP OTA / Web OTA

适合后续上位机或浏览器升级：

```text
浏览器 / 上位机 / HTTP 客户端
        ↓
上传 .bin
        ↓
ESP32 HTTP OTA 服务
        ↓
OTA 分区
```

优点：

- 可控性强。
- 可以加入登录、版本检查、升级进度和错误提示。
- 适合被上位机自动化调用。

限制：

- 代码量更大。
- 需要处理上传失败、版本回滚、超时等异常路径。

## OTA 安全门控

MUS4 是车辆底层控制系统，OTA 不能长期开放，也不能在车辆可运动状态下执行。

建议 OTA 只在满足以下条件时允许开启：

```text
1. car_output.park == PARK_LOCKED
2. 油门处于中位或零输出
3. 当前不处于紧急制动恢复过程
4. 最近 N 秒没有有效 Pilot 控制输入
5. 用户显式发送 ENABLE_OTA 命令
6. 命令通过鉴权
```

推荐 OTA 开启流程：

```text
输入：ENABLE_OTA:密码

固件检查：
- Park 是否锁定
- 油门是否安全
- 是否处于安全状态
- 密码是否正确

通过后：
- 开启 OTA 窗口 120 秒
- LED 进入 OTA 指示模式
- 超时自动关闭 OTA
```

不建议让 OTA 服务随固件启动后永久开放。

## OTA 分区要求

ESP32 必须使用支持 OTA 的分区表，例如：

```text
app0
app1
spiffs / littlefs 可选
```

如果当前 Arduino 配置使用无 OTA 分区，会出现固件可以编译但无法 OTA 的问题。

需要检查并统一当前 FQBN 和 board options。项目中目前存在两处默认 FQBN 来源：

```text
config.yaml: esp32:esp32:esp32
sketch.yaml: esp32:esp32:dfrobot_firebeetle2_esp32e
```

建议统一成明确支持 OTA 的配置，例如使用默认 OTA 分区或项目专用 OTA 分区表。

## Wi-Fi 无线串口调试方案

### 首选：TCP Console

ESP32 开启 TCP Server，例如端口 `2323`：

```text
电脑 nc / PuTTY / 自定义 Pilot 工具
        ↓ TCP
ESP32 WirelessConsole
        ↓
复用现有命令解析
```

优点：

- 最接近串口体验。
- 实现简单。
- 延迟低。
- 可以直接使用 `nc`、PuTTY 或 Python 脚本调试。

调试示例：

```bash
nc 192.168.4.1 2323
```

可输入：

```text
STATUS
TEST
BENCH
NOANSI
ANSI
Throttle:Steering
```

ESP32 返回：

```text
ACK
NACK
Txx:Sxx
状态日志
```

### 次选：WebSocket Console

适合后续图形化调试面板：

```text
浏览器 Web UI
        ↓ WebSocket
ESP32
```

优点：

- 适合实时显示油门、转向、Park、模式、IMU、电流等状态。
- 可以扩展成完整调试网页。

限制：

- 需要 WebServer / WebSocket 依赖。
- 代码和内存占用更高。
- 初期不建议优先实现。

## 命令路由设计

不要为无线通道复制一套命令解析逻辑。建议抽象命令来源：

```cpp
enum class CommandSource {
    UsbSerial,
    Rs232Serial,
    WifiTcp,
    BleUart
};
```

现有逻辑可以从：

```cpp
readSerialBuf(Serial, serial0Buf, false);
readSerialBuf(Serial1, serial1Buf, true);
```

扩展为：

```cpp
readSerialBuf(Serial, serial0Buf, false);
readSerialBuf(Serial1, serial1Buf, true);
wirelessConsole.update();
```

无线命令最终仍进入现有 `Throttle:Steering`、`TEST`、`BENCH`、`REGRESS` 等解析路径。

关键约束：

```text
无线通道只是输入输出通道，不允许绕过 Park、安全状态机、模式融合逻辑。
```

## BLE 无线串口调试方案

### 推荐用途

BLE 适合：

```text
1. 手机现场查看状态
2. 发送简单调试命令
3. 配置 Wi-Fi SSID / 密码
4. 开启 OTA 窗口
5. 低频遥测
```

不建议 BLE 承担主 OTA。

### BLE UART 服务

可实现类似 Nordic UART Service：

```text
RX Characteristic：手机 → ESP32
TX Characteristic：ESP32 → 手机
```

命令示例：

```text
STATUS
ENABLE_OTA:123456
WIFI:ssid,password
TEST
NOANSI
```

响应示例：

```json
{"mode":1,"park":1,"throttle":0,"steering":1500}
```

### 与 BLE Gamepad 的关系

当前项目已经启用 BLE Gamepad。BLE UART 有两个选择：

#### 选择 A：保留 BLE Gamepad，新增 BLE UART

优点：

- 同时拥有手柄和调试能力。

风险：

- BLE 服务复杂度增加。
- HID + UART 组合需要验证兼容性。
- 手机和电脑端连接行为可能更复杂。

#### 选择 B：编译期二选一

推荐初期采用：

```cpp
#define ENABLE_GAMEPAD_MODE
// #define ENABLE_BLE_CONSOLE
```

或：

```cpp
// #define ENABLE_GAMEPAD_MODE
#define ENABLE_BLE_CONSOLE
```

优点：

- 稳定。
- 内存压力小。
- 调试问题少。

限制：

- 同一固件不能同时做 BLE 手柄和 BLE 串口。

建议先实现编译期二选一，稳定后再评估 BLE HID + UART 共存。

## Wi-Fi 工作模式

### STA 模式

ESP32 连接已有路由器：

```text
ESP32 → 实验室 Wi-Fi / 车载路由器
```

适合：

- 固定实验室。
- OTA 上传。
- 长时间调试。
- 多设备管理。

限制：

- 依赖外部路由器。
- 现场没有 Wi-Fi 时不可用。

### AP 模式

ESP32 自己开热点：

```text
手机/电脑 → MUS4-XXXX 热点 → ESP32
```

适合：

- 野外调试。
- 没有路由器的场景。
- 首次配网。

限制：

- OTA 速度可能较慢。
- 电脑连上 AP 后可能没有互联网。

### 推荐模式

采用双模式：

```text
1. 启动时尝试 STA 连接已保存 Wi-Fi
2. 连接失败则开启 AP
3. AP 下提供配网页面、TCP Console、OTA 开启入口
```

## 权限与安全策略

### 鉴权命令

建议支持：

```text
AUTH:password
ENABLE_OTA:120
DISABLE_OTA
STATUS
PING
```

未认证前只允许：

```text
PING
STATUS
AUTH
```

未认证时禁止：

```text
Throttle:Steering
REGRESS
BENCH
ENABLE_OTA
```

### 命令权限表

| 命令类型 | 未认证 | 已认证 | Park 解锁 |
| --- | --- | --- | --- |
| `PING` | 允许 | 允许 | 允许 |
| `STATUS` | 允许 | 允许 | 允许 |
| `TEST` | 禁止 | 允许 | 建议禁止 |
| `BENCH` | 禁止 | 允许 | 建议禁止 |
| `Throttle:Steering` | 禁止 | 允许 | 按模式处理 |
| `ENABLE_OTA` | 禁止 | 允许 | 必须 Park 锁定 |
| `REGRESS` | 禁止 | 允许 | 必须 Park 锁定 |

## 实时性影响与约束

Wi-Fi 和 BLE 都依赖 ESP32 后台任务，必须避免阻塞主循环。

实现约束：

```text
1. 不使用长时间阻塞的 while 等待 Wi-Fi
2. 不在 loop 中同步处理大文件上传
3. OTA 期间暂停或限制控制输出
4. Web/TCP/BLE 发送必须限频
5. 日志不要无限广播
```

推荐频率：

```text
无线状态遥测：10Hz 以下
调试日志：按需开启
TCP Console 读取：每轮 loop 少量处理
OTA：仅 Park 锁定时独占进行
```

## 实施路线

### 第 1 阶段：Wi-Fi TCP 无线串口

目标：先能无线调试，不动 OTA。

新增：

```text
WirelessConfig.h/.cpp
WirelessConsole.h/.cpp
```

能力：

```text
1. ESP32 启动 AP：MUS4-DEBUG-xxxx
2. TCP 2323 端口提供 Console
3. 支持 STATUS / TEST / NOANSI / ANSI
4. 后续接入现有 Throttle:Steering 命令
```

验证：

```text
1. 电脑连接热点
2. nc 192.168.4.1 2323
3. 输入 STATUS
4. 确认返回车辆状态
```

### 第 2 阶段：Wi-Fi OTA

新增：

```text
WirelessOta.h/.cpp
```

能力：

```text
1. ENABLE_OTA 命令开启 OTA 窗口
2. Park 锁定才允许 OTA
3. ArduinoOTA 或 HTTP OTA 上传
4. LED 显示 OTA 状态
5. OTA 完成后自动重启
```

验证：

```text
1. Park 未锁定时 ENABLE_OTA 被拒绝
2. Park 锁定时 ENABLE_OTA 成功
3. OTA 上传成功后版本号变化
4. OTA 中断后设备仍可启动旧固件
```

### 第 3 阶段：BLE UART 调试 / 配网

能力：

```text
1. 手机 BLE 连接
2. 发送 STATUS / ENABLE_OTA
3. 配置 Wi-Fi SSID / 密码
4. 可选替代 BLE Gamepad
```

建议先做：

```cpp
#define ENABLE_BLE_CONSOLE
// #define ENABLE_GAMEPAD_MODE
```

稳定后再评估 BLE HID + UART 共存。

## 测试策略

### 单元测试

优先覆盖纯逻辑：

```text
1. 命令权限判断
2. OTA 开启条件判断
3. Wi-Fi 模式状态机
4. AUTH / STATUS / ENABLE_OTA 命令解析
5. 不同 CommandSource 的权限差异
```

### 集成测试

在 ESP32 实机验证：

```text
1. Wi-Fi AP 启动
2. TCP Console 连接和断开
3. 多客户端连接限制
4. OTA 成功升级
5. OTA 中断恢复
6. BLE Console 与 BLE Gamepad 编译期二选一
```

### 回归测试

重点确认：

```text
1. RC PWM 输入稳定性不下降
2. Park 锁定语义不改变
3. Serial1 Pilot 控制不受影响
4. TUI 刷新不被无线日志刷爆
5. BLE Gamepad 原有功能不回退
```

## 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| OTA 在车辆可运动状态触发 | 高 | Park 锁定、油门安全、人工授权、短窗口 |
| Wi-Fi/BLE 占用主循环时间 | 中高 | 全部使用非阻塞 update，发送限频 |
| BLE Gamepad 与 BLE UART 冲突 | 中 | 初期编译期二选一 |
| 无线通道绕过安全逻辑 | 高 | 统一 Command Router，不复制控制路径 |
| OTA 分区不匹配 | 中 | 先确认 FQBN 和 PartitionScheme |
| 未认证用户发送控制命令 | 高 | AUTH 门控和命令权限表 |

## 推荐下一步

先实施第 1 阶段：Wi-Fi TCP 无线串口。

开始编码前需要确认：

```text
1. 是否允许固件默认启动 AP
2. AP 名称和默认密码
3. TCP Console 是否允许发送控制命令，还是首版只允许 STATUS/TEST
4. 是否先禁用 BLE Gamepad，避免无线调试阶段资源冲突
```
