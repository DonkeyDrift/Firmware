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
- **Wi-Fi TCP Console 作为低延迟命令行调试通道**。
- **Wi-Fi Web Console 作为下一阶段主要图形化调试入口**。
- **AP + STA 双模式同时支持户外直连和室内局域网调试**。

核心原则：

```text
无线只扩展通道，不改变车辆控制安全逻辑。
OTA 只在 Park 锁定、人工授权、短时间窗口内启用。
Wi-Fi 同时负责 OTA、TCP Console 和 Web Console，AP 作为保底入口。
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
                         │ Wi-Fi TCP Console        │
                         │ Wi-Fi Web Console        │
                         └────────────┬────────────┘
                                      │
                         ┌────────────▼────────────┐
                         │ Wireless OTA             │
                         │ ArduinoOTA / HTTP OTA    │
                         └─────────────────────────┘
```

建议新增模块：

```text
WirelessConfig.h/.cpp      # Wi-Fi AP/STA 开关、SSID、密码、鉴权配置
WirelessOta.h/.cpp         # OTA 初始化、OTA 状态机、安全门控
WirelessConsole.h/.cpp     # TCP Console / Web Console 命令桥接
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

### 下一阶段主线：Wi-Fi Web Console

这里的 Web Console 不是浏览器 USB Web Serial API，而是运行在 ESP32 Wi-Fi 上的 Web Serial 风格调试页：

```text
浏览器 Web UI
        ↓ HTTP
ESP32 Web Console
        ↓
复用现有 Wireless Console 命令处理
```

访问方式：

```text
AP 户外模式：http://192.168.4.1/
STA 室内模式：http://<sta_ip>/
```

最小功能：

- 网页输入 `PING`、`STATUS`、`AUTH`、`ENABLE_OTA`、`OTA_STATUS` 等命令。
- 文本区域显示响应日志。
- 提供常用命令快捷按钮。
- 复用现有无线命令权限、安全门控和 OTA 开窗逻辑。

首版推荐使用 HTTP 轮询式 Web Console：

- `GET /` 返回单页 HTML。
- `GET /api/status` 返回当前状态。
- `POST /api/cmd` 提交一行命令并返回响应。

暂不做 WebSocket，避免额外依赖、状态管理和固件体积风险；如果后续需要实时日志流，再升级为 WebSocket。

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

## 远期可选 BLE 能力

当前阶段不推进 BLE UART。主要原因：

- Wi-Fi 已能覆盖 OTA、TCP Console 和下一阶段 Web Console。
- BLE UART 与现有 BLE Gamepad、Wi-Fi/OTA 共存存在体积和兼容性风险。
- Web Console 更适合室内和户外统一调试入口。

BLE Gamepad 仍作为已有能力保留。若后续确有手机低功耗调试或离线配网需求，再单独立项评估 BLE UART，不纳入当前 Web Console 路线。

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

采用 AP + STA 双模式：

```text
1. 启动 WIFI_AP_STA
2. 始终开启 MUS4-DEBUG SoftAP，作为户外调试和救援入口
3. 若配置了 STA SSID/PASSWORD，则同时尝试连接室内路由器
4. STA 成功后可通过局域网访问 Web Console、TCP Console 和 OTA
5. STA 失败不影响 AP、TCP Console、Web Console 或 OTA
```

状态输出应包含：

```text
ap_ip=192.168.4.1
sta_connected=0/1
sta_ip=...
web_port=80
```

本阶段 STA 凭据可先使用编译期常量，NVS 持久化和网页配网留到后续阶段。

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

### 第 3 阶段：Wi-Fi Web Console + AP/STA 双模式

能力：

```text
1. AP 模式下通过 http://192.168.4.1/ 打开 Web Console
2. STA 模式下通过 http://<sta_ip>/ 在室内局域网调试
3. 网页发送 PING / STATUS / AUTH / ENABLE_OTA / OTA_STATUS / 控制命令
4. 复用现有无线命令权限、Park 门控和 OTA 窗口
5. STATUS 输出 AP/STA 网络状态
```

最小实现：

```text
GET  /api/status
POST /api/cmd
```

### 第 4 阶段：可选 Web 配网 / NVS 持久化

能力：

```text
1. Web 页面配置 STA SSID / PASSWORD
2. NVS 保存 STA 凭据
3. STA 连接失败时保持 AP 可用
4. 提供清除 Wi-Fi 配置入口
```

远期如确有低功耗手机调试需求，再单独评估 BLE UART。

## 测试策略

### 单元测试

优先覆盖纯逻辑：

```text
1. 命令权限判断
2. OTA 开启条件判断
3. AP/STA Wi-Fi 模式状态机
4. AUTH / STATUS / ENABLE_OTA 命令解析
5. Web Console 与 TCP Console 的权限一致性
```

### 集成测试

在 ESP32 实机验证：

```text
1. Wi-Fi AP 启动
2. TCP Console 连接和断开
3. Web Console AP 访问
4. STA 连接成功后局域网访问
5. OTA 成功升级
6. OTA 中断恢复
```

### 回归测试

重点确认：

```text
1. RC PWM 输入稳定性不下降
2. Park 锁定语义不改变
3. Serial1 Pilot 控制不受影响
4. TUI 刷新不被无线日志刷爆
5. TCP Console 与 OTA 原有功能不回退
```

## 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| OTA 在车辆可运动状态触发 | 高 | Park 锁定、油门安全、人工授权、短窗口 |
| Wi-Fi/Web 服务占用主循环时间 | 中高 | 使用非阻塞 handleClient，发送限频 |
| Web Console 增加固件体积 | 中 | 首版使用内置 WebServer 和 HTTP 轮询，暂不引入 WebSocket |
| 无线通道绕过安全逻辑 | 高 | 统一 Command Router，不复制控制路径 |
| OTA 分区不匹配 | 中 | 先确认 FQBN 和 PartitionScheme |
| 未认证用户发送控制命令 | 高 | AUTH 门控和命令权限表 |

## 推荐下一步

先实施第 3 阶段：Wi-Fi Web Console + AP/STA 双模式。

开发前置约束：

```text
1. AP 保持默认开启，作为户外调试和救援入口
2. STA 作为可选室内调试入口，失败不能影响 AP
3. Web Console 复用现有 AUTH、Park 门控和 OTA 窗口
4. 首版使用 HTTP 轮询，不引入 WebSocket
```

## OTA 增强最小实施

当前阶段继续采用 ArduinoOTA，不引入 HTTP Web OTA，也不改变 Park 锁定、`AUTH` 和短时间窗口的 OTA 安全门控。

本轮增强目标：

```text
1. 固件启动日志输出 firmware、version 和 build 时间
2. Wi-Fi Console 的 STATUS 在原有字段末尾追加 version 和 build
3. TUI 标题使用统一固件版本号
4. WSL 构建脚本支持通过 -Ota 调用 espota.py 上传
```

运行时可观测输出示例：

```text
BOOT firmware=MUS4 version=v1.4-ota-enhanced build="May 26 2026 21:30:00"
STATUS mode=0 park=1 throttle=0 steering=0 wifi_frames=1 wifi_errors=0 ota_window=0 ota_progress=0 ota_ttl_ms=0 version=v1.4-ota-enhanced build="May 26 2026 21:30:00"
```

工具化 OTA 上传推荐命令：

```powershell
.\arduino-cli-wsl.ps1 -c -u -Ota -OtaHost 192.168.4.1
```

如自动查找不到 `espota.py`，显式指定：

```powershell
.\arduino-cli-wsl.ps1 -u -Ota -OtaHost 192.168.4.1 -EspotaTool "C:\Users\cross\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.8-cn\tools\espota.py"
```

## Wi-Fi Web Console 与 AP/STA 双模式最小实施

本阶段目标是提供浏览器可用的 Web Serial 风格调试页，同时支持户外 AP 直连和室内 STA 局域网访问。

### 开发范围

```text
1. 引入 WebServer.h
2. 新增 GET / 返回内嵌 HTML 调试页
3. 新增 GET /api/status 返回状态文本
4. 新增 POST /api/cmd 提交单行命令并返回响应
5. 将 TCP Console 与 Web Console 复用同一套命令权限和处理逻辑
6. STATUS 追加 AP/STA 网络状态
```

### AP/STA 行为

```text
1. AP 永远开启：MUS4-DEBUG / 192.168.4.1
2. STA 可选开启：配置 SSID/PASSWORD 后尝试连接室内路由器
3. STA 成功：可通过 http://<sta_ip>/ 访问 Web Console
4. STA 失败：AP 仍保持可用，不影响 TCP Console 和 OTA
```

建议启动日志和 `STATUS` 输出：

```text
web_port=80 ap_ip=192.168.4.1 sta_connected=0 sta_ip=0.0.0.0
```

### Web Console 页面

首版页面保持极简：

```text
- 命令输入框
- 响应日志区域
- PING / STATUS / AUTH / ENABLE_OTA / OTA_STATUS 快捷按钮
- 当前 AP IP、STA IP、固件版本显示
```

当前不做登录 Cookie 或 Session。Web Console 仍通过命令层 `AUTH:mus4-debug` 解锁控制命令。

### AP 户外验证

```text
1. 连接 MUS4-DEBUG
2. 打开 http://192.168.4.1/
3. 执行 PING，预期 PONG
4. 执行 STATUS，预期包含 version/build/ap_ip/sta_connected/web_port
5. 执行 AUTH:mus4-debug，预期 AUTH_OK
6. Park 锁定后执行 ENABLE_OTA，预期 OTA_READY
7. 同时确认 TCP Console 2323 仍可连接
```

### STA 室内验证

```text
1. 配置 STA SSID/PASSWORD
2. 编译上传固件
3. 从串口、TCP Console 或 AP Web Console 查看 sta_ip
4. 在同一局域网打开 http://<sta_ip>/
5. 重复 PING / STATUS / AUTH / ENABLE_OTA 测试
6. 断开路由器或填错密码，确认 http://192.168.4.1/ 仍可访问
```

### 后续扩展

```text
1. Web 配网页面
2. NVS 持久化 STA 凭据
3. WebSocket 实时日志流
4. 多客户端访问限制和会话管理
```
