# MUS4_FW 串口输出逻辑与功能配置巡检

- 巡检对象：`MUS4_FW.ino`（固件主骨架）+ `libraries/mus4_*` 各模块
- 固件版本：v1.10.1（`libraries/mus4_core/src/BuildInfo.h`）
- 巡检日期：2026-09-26（v1.10.1 更新：新增 Serial0/Serial1 角色对调开关）
- 关联文档：`docs/Inspect/esp32-telemetry-frequency-design.md`、`docs/Plan/DEV模式影响面与运行逻辑映射.md`

---

## 1. 串口硬件总览

ESP32 共使用 3 个硬件 UART（均为 115200 / 8N1），另有 WiFi 提供的"软串口"通道（TCP 2323 控制台 + HTTP 80 Web 控制台 + WebSocket 81 遥测）。自 v1.10.1 起，Serial0 与 Serial1 按**角色**（而非物理口）组织，角色映射唯一实现点为 `libraries/mus4_core/src/SerialRole.h`。

**当前固件（v1.10.1）启用了 `MUS4_SWAP_SERIAL0_SERIAL1`，Serial0 与 Serial1 处于对调状态**：

| 角色（代码标识） | 当前物理端口 | 引脚 | 职责 |
|------|------|--------|----------|
| `serialTelemetry` 主遥测 | **Serial0（USB Type-C / UART0，已对调）** | 板载 USB 桥接 | **上行遥测**（T..S.. / M:P / $IMU）+ **下行控制**（`<thr>:<str>`） |
| `serialConsole` 控制台 | Serial1（TTL，已对调） | RX1=16, TX1=17 | 固件日志（LOG_SERIAL 目标）+ ANSI TUI 仪表盘 + 本地命令行 |
| Serial2（不参与对调） | RX2=19, TX2=18 | TTL ↔ Linux 上位机（经 `UART_SEL`=GPIO12 选择） | 双向联通验证（PING/PONG）+ Auth 身份识别 + 上位机配网协议 + 1Hz BEAT 心跳 |

### 1.1 串口角色对调开关 MUS4_SWAP_SERIAL0_SERIAL1（v1.10.1 新增）

定义于 `FirmwareConfig.h`，**当前处于启用（对调）状态**：

- **何时需要切换**：当需要**从 USB Type-C 口输出主遥测信息**（上位机只接 USB 线、或 TTL 16/17 另作他用）时，必须启用本宏——这正是当前固件的默认状态。
- **如何恢复原布局**：注释掉 `FirmwareConfig.h` 里的 `#define MUS4_SWAP_SERIAL0_SERIAL1` 重新编译，即回到"主遥测走 TTL 16/17、日志/TUI 走 USB Type-C"的 v1.10.0 及之前布局。
- **对调的内容**：仅逻辑角色互换，硬件初始化（引脚、波特率 115200/8N1）不变。具体路由：
  - 主遥测单次 `write()`（T..S../M:P/$IMU 帧）→ `serialTelemetry`（对调后 = USB）；
  - 上位机下行控制帧在哪个口输入不敏感——两个口都跑 `readSerialBuf()`，行解析按角色打 WebLog 标签；
  - TUI 仪表盘与 `mus4Log` 的 SERIAL 目标 → `serialConsole`（对调后 = TTL 16/17）；
  - `ENABLE_SERIAL2_ECHO_TO_SERIAL0` 调试透传与 `#ifdef DEBUG` RC 打印 → 控制台端口（宏名沿用不改）。
- **对调模式的配套处理**：
  - 100Hz IMU + 60Hz T/S 改走 UART0 后，`Serial.setTxBufferSize(1024)` 前置于 `Serial.begin()`（沿用 v1.7.34 结论：默认 256B TX 环形缓冲会溢出导致帧内丢字符）；
  - 开机 banner 分流：对调后主遥测口输出 `ESP32 Receiver Telemetry Ready! (USB, roles swapped)`，默认布局仍输出 `ESP32 Receiver Serial1 Ready!`；
  - WebLog 源标签**按角色归类不随物理口漂移**：主遥测端口恒记 `serial1`（命中 `SERIAL1_WEB_LOG_CAPACITY=64` 专用高吞吐环形缓冲），控制台端口恒记 `serial`——避免对调模式下 60Hz 遥测帧涌入通用 64 槽日志环挤掉一般日志。
- **对调模式的运维注意**：
  - USB Type-C 同时承担主遥测后，**固件烧录（同一 USB 口）前需停掉占用串口的上位机**；ROM bootloader 启动噪声也会与遥测帧混在同一口上（上位机按行解析非协议行需忽略）。
  - TTL 16/17 变成控制台后，接 USB-TTL 线即可看到日志/TUI（需要 `LOG_SERIAL` 命令或未启用 WiFi 控制台的编译配置，见 §2.1）。

关键点：

- **UART_SEL（GPIO12）在 `setup()` 中被拉高**，选择 TTL ↔ CPU 通路（此时 RX_2_PIN=19 / TX_2_PIN=18 生效）。拉低才切换到 CPU 以外的主板连接（`FirmwareConfig.h` 注释）。
- 主遥测端口（无论对调与否）的 TX/RX 环形缓冲区显式设为 **1024 字节**（v1.7.34 修复：必须在 `begin()` 之前调用 `setTxBufferSize`，否则 100Hz IMU + 60Hz 遥测会打爆默认 256B 缓冲，造成 `$IMU`/`T..S..` 帧内字符丢失）。默认布局作用于 Serial1；对调布局下额外对 Serial0 生效（`MUS4_FW.ino` `setup()`）。
- Serial2 与主遥测口同为 115200；Serial2 还有独立调试宏 `ENABLE_SERIAL2_ECHO_TO_SERIAL0`（默认关闭，开启后逐字节透传到**控制台端口**）。

---

## 2. 控制台端口（默认 Serial0/USB；对调后 Serial1/TTL）：日志 + TUI + 本地命令行

> 本节"Serial0"均指**控制台角色**端口；当前固件（v1.10.1 对调状态）下实际为 Serial1（TTL 16/17）。

### 2.1 输出：统一日志入口 mus4Log

`libraries/mus4_log/src/Mus4Log.cpp` 提供唯一结构化日志入口：

- `mus4LogLine(source, line)` / `mus4Logf(source, fmt, ...)`，输出格式 `[source] message`，buffer 上限 192 字节。
- 日志目标由全局 `mus4LogTarget` 决定（`FirmwareConfig.h:35-43`）：
  - `ENABLE_WIFI_CONSOLE` 编译时**默认 `MUS4_LOG_TARGET_WEB`**：日志写入 WebLogBuffer（见 §6），**不打印到 Serial0**；
  - 未启用 WiFi 控制台时默认 `MUS4_LOG_TARGET_SERIAL`：直接打印到**控制台端口**（`serialConsole`，v1.10.1 起角色化；对调状态下 = Serial1/TTL）。
- 运行期可用命令 `LOG_WEB` / `LOG_SERIAL` 切换（`CommandDispatcher.cpp:112-123`）。
- 主循环里 TUI 与日志互斥：仅当 `mus4LogTarget == MUS4_LOG_TARGET_SERIAL` 时才驱动 TUI 渲染（`MUS4_FW.ino:766, 888`）——即 Web 日志模式下 Serial0 安静，Serial 日志模式下 TUI 活动。

典型 boot 输出（`setup()`）：

```
[boot] firmware=MUS4 version=v1.10.1 build="Sep 26 2026 06:27:00"   → 控制台端口（LOG_SERIAL 模式）/ WebLog
[boot] ESP32 Receiver Serial Ready!                                 → 控制台端口
ESP32 Receiver Telemetry Ready! (USB, roles swapped)                → 主遥测端口（对调状态）
（默认布局时此处为：ESP32 Receiver Serial1 Ready!）                  → 主遥测端口
```

### 2.2 输出：TUI 仪表盘（ANSI 终端 UI）

`libraries/mus4_ui/src/TUI.cpp` 在控制台端口上渲染基于 ANSI 转义序列的差分刷新界面（`TUI tui(serialConsole)`，随对调开关自动换口）：

| 行 | 内容 | 刷新条件 |
|----|------|----------|
| 1 | `DonkeyCar Control System - <版本>`（青色标题） | 仅初始化 |
| 3 | `MODE: MANUAL / SEMI-AUTO / FULL-AUTO`（绿/黄/品红） | mode 变化 |
| 4 | `PARK: LOCKED/UNLOCKED` + `DRIFT: ON/OFF/ACTIVE`（含 GyroZ/Comp/Scale） | 变化 |
| 5 | `LOG: <最近一条 tui.log()>` | log 变化 |
| 7 | `RC: [CH1..CH6]` + `OUT S:<duty> T:<duty>`（舵机/电调 LEDC 实际占空比） | 任一变化 |
| 8 | `Out: Str <±100> Thr <±100>` | 变化 |
| 10+ | Throttle/Steering History 波形图（默认 `setWaveformEnabled(false)` 关闭） | — |
| 末尾 | `INA: 电压 电流 功率` / `MPU: A[ax,ay,az] G[gx,gy,gz]` | 每帧 |

- 自适应刷新率：`uiIntervalCurrent` 在 100–250ms 间调整，单帧渲染超 250ms 则 +30ms 放缓，否则 −20ms 加快（`MUS4_FW.ino:919-924`）。
- `NOANSI` / `ANSI` 命令切换纯文本/彩色模式；`FILTER_DEBUG` 切换滤波调试打印（`LocalCommands.cpp:15-21`）。
- 波形参数：`WAVE_WIDTH=20`、`WAVE_HEIGHT=6`（`FirmwareConfig.h:148-154`），当前主程序固定关闭波形以省 CPU。

### 2.3 输入：readSerialBuf → dispatchCommandLine

`Serial0` 与 `Serial1` 两个物理口的下行都走 `readSerialBuf()`（`mus4_command/src/SerialLineReader.cpp`），即**主遥测端口的控制帧与控制台端口的本地命令共用同一行解析入口**：

- 逐字节收行（忽略 `\r`，`\n` 触发处理），缓冲 256 字节（`SerialBufferTypes.h` 的 `SerialBuf`），超长整行丢弃并置 `overflow`。
- 每行回显到 WebLog（`> <line>`，密码类命令经 `redactWirelessConsoleLine()` 脱敏：`AUTH:`、`WIFI_STA_PASSWORD:`、`WIFI|ssid|<redacted>`）。
- 调用 `dispatchCommandLine(line, out, sb, /*pilotSilent=*/true)` —— **上位机高速控制帧不回 ACK**（避免主机把 ACK 当垃圾数据），本地终端调试时可带 seq 获得 `ACK:<seq>` / `NACK:<seq>`。
- 响应文本同时镜像进 WebLog。

---

## 3. 主遥测端口（默认 Serial1/TTL；对调后 USB Type-C）：遥测/控制通道（与 DonkeyCar 上位机对齐）

> 本节"Serial1"均指**主遥测角色**端口；当前固件（v1.10.1 对调状态）下实际为 Serial0（USB Type-C）。

### 3.1 上行（ESP32 → 上位机）

v1.7.33 起所有上行帧拼入**单一 512 字节栈缓冲后一次 `serialTelemetry.write()` 发出**，消除多次 print 的 TX 环形缓冲指针竞争与帧拼接问题（`MUS4_FW.ino`）；v1.10.1 起 `serialTelemetry` 由 `MUS4_SWAP_SERIAL0_SERIAL1` 决定绑定哪个物理口（当前对调 → USB Type-C）。

| 帧 | 格式 | 频率 | 条件 |
|----|------|------|------|
| 遥测 | `T<throttle>S<steering>\n` | ~60Hz（`RC_DATA_UPDATE_INTERVAL=16ms`） | 仅 `CAR_MODE_MANUAL`，且 OTA 未在传输 |
| 模式 | `M<mode>:P<park>\n` | 状态变化立即 + 1Hz 心跳（`MODE_PARK_HEARTBEAT_MS=1000`） | 所有模式，OTA 未在传输 |
| IMU | `$IMU,<seq>,<ts_ms>,<ax>,<ay>,<az>,<gx>,<gy>,<gz>\n` | ~100Hz（`IMU_TELEMETRY_INTERVAL_MS=10ms`） | 所有模式，MPU 在线且 OTA 未在传输 |

- `$IMU` 数值格式 `%.4f`：加速度 m/s²、角速度 rad/s；`seq` 为 uint16 自增；对齐上位机 GRU W=16 ring buffer（注释 `FirmwareConfig.h:142`）。
- **OTA 门控**：`shouldEmitSerial1Telemetry()`（`WifiOta.cpp:48-54`）在 `os.inProgress` 期间返回 false —— OTA 真正传输时暂停全部 Serial1 上行（v1.7.8 起；DEV 模式下 windowOpen 常开不再阻塞通信）。
- 写入不完整仅做静态计数（`s1WriteDropCount`），不刷屏。
- Web 侧镜像：`T..S..` 帧以 10Hz 节流（`TELEM_WEB_LOG_INTERVAL_MS=100`）写入 WebLog 源 `serial1`；`M:P` 帧每次变化/心跳都写入。

### 3.2 下行（上位机 → ESP32）

主遥测端口的下行经 `readSerialBuf(serialTelemetry, …)` → `dispatchCommandLine()`（对调状态下物理口为 USB Type-C）。控制帧格式（`CommandParser.cpp`）：

```
<thr>:<str>              基本格式，thr/str ∈ [-100, 100]
<thr>:<str>:<seq>        带序号
<thr>:<str>*<CRC>        *后跟 2 位 hex 校验和（payload 字节和 & 0xFF）
<thr>:<str>:<seq>*<CRC>  全格式
```

- 越界（±100 之外）返回解析失败；`pilot_data` 更新，`sb.frames++`；失败 `sb.errors++`。
- 除控制帧外，Serial1 上也可发本地命令（MODE/SERVO_MID/JOYSTICK_* 等，见 §8），ACK 回到 Serial1。

---

## 4. Serial2：联通验证 + Auth + 上位机配网通道

独立处理函数 `handleSerial2()`（`MUS4_FW.ino:416-537`），**不经过** `dispatchCommandLine`，避免 PING/PONG 被当控制命令解析。行缓冲 128 字节，超长丢弃。

按优先级分三级：

1. **上位机配网响应**（Linux 上位机 → ESP32）：
   - `STATUS|<state>` → 更新 `hostWifiStatus`（如 `CONNECTING`）
   - `OK|<ip>` → `hostWifiStatus="connected"`，记录 `hostWifiIp`
   - `FAIL|<reason>` → `hostWifiStatus="failed"`，记录错误
   - `HOSTIP|<ipv4>` → 上位机周期上报自身局域网 IP（带 IPv4 文本校验，记录时间戳供 `/api/host-wifi-status` 展示 `host_ip_age_s`）
   - 对应下行命令由 Web 命令 `target=serial` 转发：`WIFI|<ssid>|<password>\n`（`WebConsoleServer.cpp:288-316`）
2. **Auth 身份识别**（`ENABLE_AUTH_SERVICE` 启用时）：`CMD:READ_HW_ID|READ_UID|WRITE_UID|CLEAR_UID` + `ARG:<uuid>`（多行状态机，ARG 超时 5s 返回 `ERR:05`）。响应 `OK:...` / `ERR:NN:...`（`mus4_auth/src/AuthService.cpp`）。HW ID 为 eFuse MAC 的 12 位小写 hex；user_id 为 UUID v4 格式校验，存 NVS 命名空间 `auth`。
3. **任意其他文本** → `ECHO,<line>\n` 回显（防御性）。

附加行为：

- `PING,<seq>` → `PONG,<seq>,<millis>`（双向联通验证第 1 级）。
- **1Hz 心跳**：每秒主动发 `BEAT,<millis>\n`（`MUS4_FW.ino:529-536`）。
- `ENABLE_SERIAL2_ECHO_TO_SERIAL0`（默认关）开启后：可打印字符直显、不可打印 `\xNN` 转义输出到 Serial0，超 200ms 无新数据强制换行。

---

## 5. WiFi 控制台（三个网络端口）

`ENABLE_WIFI_CONSOLE` 下的三类远程通道（均可下命令、看状态，权限模型见 §8.2）：

### 5.1 TCP 原始控制台 — 端口 2323（`WIFI_CONSOLE_PORT`）

- `updateWifiConsole()`（`WifiManager.cpp:1396+`）单客户端 accept；连接后发送 banner：
  ```
  MUS4 WiFi Console Ready
  Use AUTH:<password> to unlock control commands
  ```
- 每行经 `processWirelessConsoleLine()`（`WirelessConsole.cpp`）分发：`PING`→`PONG`，`STATUS`→状态长串，`AUTH:<pwd>`→`AUTH_OK/AUTH_FAIL`，其余走命令权限表后进入 `dispatchCommandLine`。
- 行超限回 `NACK:OVERFLOW`。当前 `WIFI_CONSOLE_AP_PASSWORD` 为**空串** → `isWirelessConsoleAuthDisabled()` 为 true，等效免认证。

### 5.2 HTTP Web 控制台 — 端口 80（`WIFI_WEB_CONSOLE_PORT`）

页面：`/`（Drifter Console 主页）、`/judge`、`/drift`、`/update`（OTA 上传页）+ 强制门户兼容路由（`/generate_204` 等）。

主要 API（`WebConsoleServer.cpp:1568-1613` 注册表）：

| 端点 | 方法 | 说明 |
|------|------|------|
| `/api/status` | GET | 全量状态长串（mode/park/throttle/steering/ota/heap/ws 统计/ap/sta/mdns/host_ip 等） |
| `/api/cmd` | POST | 命令入口；`target=serial\|serial1` 时原样转发 Serial2（上位机配网） |
| `/api/log` | GET | WebLog 增量 JSON（`?since=<seq>`），含 `dropped` 计数 |
| `/api/data` | GET | 遥测缓冲 `points[]`（紧凑 JSON）+ `latest`（全字段 JSON，含五轴 IMU/漂移/执行器 duty） |
| `/api/joystick-cal` | GET/POST | 摇杆校准状态与动作（start/save/retry/abort/reset） |
| `/api/wifi-ap`、`/api/wifi-sta`(+`/password`,`/scan`,`/clear`,`/history`,`/history/delete`) | GET/POST | AP/STA 配置管理 |
| `/api/host-wifi-status` | GET | 上位机配网状态（Serial2 通道回执） |
| `/api/judge-config`、`/api/drift-config`(+`/reset`) | GET/POST | 评分/漂移参数（blob 一次写 NVS，避免行车中逐键 commit 卡顿） |
| `/api/mute`、`/api/language`、`/api/devmode` | GET/POST | 静音 / 界面语言 / DEV 模式开关 |
| `/update` | POST(+upload) | HTTP OTA 分区写入；OTA 传输中 middleware 对非 `/update` 请求直接 503 |

### 5.3 WebSocket 遥测 — 端口 81（`WIFI_WEB_SOCKET_PORT`）

`mus4_web/src/WebTelemetry.cpp`，AsyncWebServer 独立端口：

- 连接即推 `{"type":"hello","seq":<最新seq>}`；日志通过 `webLogBufferSetSocketSink` 实时推 `{"type":"log",...}`。
- 数据帧为**自定义二进制 schema v2**（16ms 推送节流，`WIFI_WEB_SOCKET_PUSH_INTERVAL_MS`）：魔数 `'M','4',2,0` + dropped/seq/t/dt + latest 全量（thr/str/gyroZ + 五轴 gx,gy,ax,ay,az + mode/park + 6 路 PWM + pilot + 漂移 + 电压/pseudoSpeed/actuator duty/servo_mid/motor_mid/throttle_min/max）+ 最多 8 点 history（`WIFI_WEB_SOCKET_MAX_POINTS_PER_FRAME=8`）。
- 限流保护：OTA 传输期拒新连接并停推；堆 < 60000（`WIFI_WEB_TELEMETRY_MIN_FREE_HEAP`）跳过；最多 2 客户端（`WIFI_WEB_SOCKET_MAX_CLIENTS`）；keepalive 60s。所有 String 写入约束在 main loop 单上下文（v1.7.17 修复跨任务撕堆）。

---

## 6. WebLog 环形缓冲（日志双缓冲设计）

`mus4_web/src/WebLogBuffer.cpp`：

- **通用缓冲** 64 槽（`WIFI_WEB_LOG_CAPACITY`）：源 `serial`/`serial1` 之外的 web/tcp/cmd/ota/wifi/cloud/boot/tui/filter 等；行宽 64 字节。
- **Serial1 专用缓冲** 64 槽（`SERIAL1_WEB_LOG_CAPACITY`）：只收 `serial1` 源（T..S../M:P 等），防止 60Hz 遥测把通用日志挤掉；行宽仅 16 字节。
- 两个缓冲按 `seq` 归并输出（`writeWebLogsJson`）；溢出计数 `dropped` 暴露在 `/api/log` 与 `/api/status` 的 `web_log_dropped`。
- 实时性：每条 append 即回调 socket sink（WebSocket `type:log` 广播）。

---

## 7. 定时/节流参数速查（FirmwareConfig.h + WifiConsoleTypes.h）

| 宏 | 值 | 含义 |
|----|-----|------|
| `SENSOR_UPDATE_INTERVAL` | 2ms | INA219/MPU6050 轮询（~500Hz 上限，实际受 loop 周期约束） |
| `RC_FILTER_UPDATE_INTERVAL` | 2ms | RC 中值滤波更新 |
| `RC_DATA_UPDATE_INTERVAL` | 16ms | Serial1 `T..S..` 帧节奏（~60Hz，仅 MANUAL） |
| `IMU_TELEMETRY_INTERVAL_MS` | 10ms | Serial1 `$IMU` 帧节奏（~100Hz） |
| `MODE_PARK_HEARTBEAT_MS` | 1000ms | Serial1 `M:P` 心跳 |
| `TELEM_WEB_LOG_INTERVAL_MS` | 100ms | T..S.. 写 WebLog 节流（10Hz） |
| `UI_UPDATE_INTERVAL` | 2ms | TUI 基准刷新（实际由自适应 100–250ms 控制） |
| `RC_SIGNAL_TIMEOUT` | 1s | RC 信号超时（µs） |
| `RC_PWM_MIN/MAX` | 800/2200 µs | 合法脉宽边界（越界视为噪声丢弃） |
| `PWM_FILTER_SIZE` | 5 | 滑窗中值滤波窗口 |
| `WIFI_WEB_DATA_INTERVAL_MS` | 16ms | Web 遥测采样（`sampleWifiWebData`，~60Hz 入 256 槽环形） |
| `WIFI_WEB_SOCKET_PUSH_INTERVAL_MS` | 16ms | WS 二进制帧推送节流 |
| `WIFI_OTA_WINDOW_MS` | 120000 | OTA 窗口 TTL |
| `WIFI_STA_APPLY_DELAY_MS` | 800 | STA 新配置延迟生效窗口 |
| Cloud 上报 | 5min 心跳 / 1min 首报重试 / 5s HTTP 超时 | `CloudReporter.cpp:51-53` |

---

## 8. 功能配置（编译开关与命令面）

### 8.1 FirmwareConfig.h 功能宏（当前值）

| 宏 | 状态 | 作用 |
|----|------|------|
| `MUS4_SWAP_SERIAL0_SERIAL1` | **开**（v1.10.1 新增） | Serial0(USB) ↔ Serial1(TTL) 角色对调：主遥测改走 USB Type-C、控制台改走 TTL 16/17；**需要从 USB Type-C 口输出主遥测信息时切换**（当前默认即对调）。详见 §1.1 |
| `ENABLE_WIFI_CONSOLE` | **开** | WiFi TCP/HTTP 控制台、Web 日志、DEV 模式、OTA、云上报等的前提 |
| `ENABLE_WIFI_WEBSOCKET_TELEMETRY` | **开**（随上者联动） | 端口 81 WS 二进制遥测 |
| `ENABLE_AUTH_SERVICE` | **开** | eFuse 身份识别命令（CMD:READ_HW_ID 等），Serial2 与所有串口可用 |
| `ENABLE_CLOUD_REPORT` | **开** | 「找小车」上报：STA 联网后 POST `{"device_id","lan_ip","hostname","version"}` 到 `CLOUD_REPORT_URL_DEFAULT`（find-dkc.pages.dev/report；本地 `WirelessSecrets.h` 可覆盖 URL） |
| `DISABLE_WIFI_NAME_DISCOVERY` | **开** | 关闭 mDNS/NetBIOS/LLMNR 主机名发现（弱网下多播风暴会挤掉 WS 曲线/日志通道） |
| `ENABLE_GAMEPAD_MODE` | 关（`ENABLE_WIFI_CONSOLE` 打开时自动关闭） | BLE 手柄 |
| `ENABLE_DIAGNOSTIC_COMMANDS` | 关（注释） | FILTER_TEST 单元测试等诊断命令 |
| `ENABLE_SERIAL2_ECHO_TO_SERIAL0` | 关（注释） | Serial2 → Serial0 逐字节 hex 调试透传 |
| `ENABLE_BOOT_STEERING_SELF_TEST` | 关（注释） | 开机转向信号处理自测（`run_steering_tests`） |
| `ENABLE_RC_MCPWM_CAPTURE` | 0 | 用 MCPWM 外设捕获 RC PWM（否则 GPIO 中断方式） |
| `MUS4_LOG_TARGET` | WEB（WiFi 控制台开启时） | 日志默认去处（见 §2.1） |

引脚/通道映射（MUS4-v2.4.2 PCB）：RC 输入 CH1–CH6 = GPIO 36/39/34/26/27/35（`pwm_value[]` 索引 0–5 依次为 STEERING/THROTTLE/PARK/MODE/DRIFT/DRIFT_SCALE）；执行输出 舵机=23、电调=25、备用 PWM_1=32/PWM_2=33；LED=WS2812B@GPIO5×1；蜂鸣器=GPIO2；I2C SDA/SCL=21/22@400kHz（MPU6050 + INA219）。

### 8.2 命令集与权限模型

`dispatchCommandLine`（Serial0/Serial1 共用）与 `processWirelessConsoleLine`（TCP/Web 共用）汇总：

**控制/校准类**（无线来源需权限判定，`WirelessConsole.cpp:168-189`）：
- 控制帧 `<t>:<s>`（无线来源必须已认证）
- `MODE 0|1|2`（`MODE:` 同义）：设行车模式，需认证但 Park 锁定下也允许（油门仍钳 0）
- 摇杆校准：`JOYSTICK_CAL/SAVE/RETRY/ABORT/RESET/STATUS`；旧别名 `STEER_CAL/CAL_SAVE/CAL_RETRY/CAL_ABORT/CAL_RESET/CAL_STATUS` 回 `ACK:DEPRECATED_USE_JOYSTICK_*`
- 中点/行程：`SERVO_MID [4915,9830]`、`MOTOR_MID`、`THROTTLE_MIN`、`THROTTLE_MAX`（查询/设置双形态，范围受 mid 约束）
- OTA：`ENABLE_OTA`（无线，需 Park LOCKED）、`ENABLE_OTA:<token>`（本地串口）、`OTA_STATUS`、`DISABLE_OTA`
- WiFi STA：`WIFI_STA_SSID:` / `WIFI_STA_PASSWORD:` / `WIFI_STA_APPLY` / `WIFI_STA_CLEAR` / `WIFI_STA_STATUS`

**显示/日志类**（免 Park，需认证或 DEV）：
- `ANSI` / `NOANSI` / `FILTER_DEBUG` / `LOG_WEB` / `LOG_SERIAL`

**查询类**（始终放行）：`PING`（→PONG）、`STATUS`（→全量状态行）、`WIFI_STA_STATUS`

**Auth 类**（最高优先级，先于一切分发）：`CMD:READ_HW_ID` / `CMD:READ_UID` / `CMD:WRITE_UID`+`ARG:` / `CMD:CLEAR_UID`

权限分层要点：
- 控制台密码为空 → 免认证（当前即此状态）。
- `DEV ON`（Web 来源）放权白名单：OTA + Web 配置 + 显示/日志切换 + WIFI_STA_* + 校准命令（仍需 Park 锁定）；控制与诊断命令（TEST/BENCH/STRESS 等）始终要求认证。
- 校准/自测类命令在 Park 未锁定时回 `NACK:PARK_REQUIRED`，未认证回 `NACK:UNAUTHORIZED`。

### 8.3 OTA 与 Serial1 上行的联动

- OTA 传输期（ArduinoOTA 或 HTTP `/update` 任一）：`forceWifiOtaParkLocked()` 强制 Park 锁 + 油门 0；`shouldEmitSerial1Telemetry()=false` 暂停 Serial1 全部上行；WS 拒新连接/停推；HTTP 侧非 `/update` 请求 503。
- 传输期间状态灯故障灯效（随机乱闪），重启后延续到开机蜂鸣器播完 + 800ms 空闲。
- OTA 成功后 `esp_ota_mark_app_valid_cancel_rollback()` 取消 bootloader 回滚（v1.7.28）。

---

## 9. 数据流一图流

```
RC 接收机(6ch PWM) ──中断捕获──▶ pwm_value[] ──中值+辅助通道稳定──▶ pwm_filtered[]
                                                                      │
MPU6050/INA219 (I2C 400k) ──2ms轮询──▶ mpu6050Data/ina219Data          │
                                                                      ▼
上位机 ──主遥测口下行 <t>:<s>──▶ pilot_data ──▶ ControlMixer/Safety ─▶ car_output ──▶ 舵机23/电调25
                                       ▲                                │
Web命令/API ───────────────────────────┘                                ├─▶ 主遥测口上行 T/S、M:P、$IMU（单缓冲一次 write，对调后=USB Type-C）
Serial2 PING/AUTH/配网 ──▶ handleSerial2（独立路径）                     ├─▶ wifiWebData[256] ─▶ /api/data + WS:81 二进制
TCP:2323 / Web:80 命令 ──▶ processWirelessConsoleLine ─▶ dispatch       └─▶ 控制台端口 TUI（LOG_SERIAL 模式，对调后=TTL 16/17）
                                                    │
                                                    └─▶ WebLogBuffer(64+64) ─▶ /api/log + WS log 实时推送
mus4Log[source] ──▶ target=WEB: appendWebLog / target=SERIAL: Serial0 println
CloudReporter ──▶ find-dkc.pages.dev/report（5min 心跳）
```

---

## 10. 巡检观察（潜在关注点）

1. **串口角色已对调（v1.10.1 当前状态）**：主遥测在 USB Type-C 上、日志/TUI 在 TTL 16/17 上。只接 TTL 线看不到遥测、只接 USB 看不到日志——现场排查前先确认角色状态（`FirmwareConfig.h` 的 `MUS4_SWAP_SERIAL0_SERIAL1` 是否定义 + 主遥测口开机 banner 是 `Telemetry Ready! (USB, roles swapped)` 还是 `Serial1 Ready!`）。
2. **Serial0 在默认配置（LOG_WEB）下只有 TUI 之前的 boot 行**——`mus4Log` 全部进 Web 缓冲，且 TUI 仅在 `LOG_SERIAL` 模式渲染；现场只插控制口调试会"看起来没日志"，属预期行为，需先发 `LOG_SERIAL`。
3. **主遥测下行命令的 ACK 与遥测同路**：上位机在 MANUAL 高速下发控制帧时 `pilotSilent=true` 不回 ACK，但如果混发本地命令，ACK 会出现在主遥测口上行流里，上位机解析需容忍。
4. **主遥测端口的 `Telemetry/Serial1 Ready!` banner 是裸文本**，未走帧协议，上位机需按"非协议行"忽略；对调模式下 ROM bootloader 的启动噪声也与遥测帧同口输出。
5. **Serial2 的 `WIFI|` 命令**只能从 Web `/api/cmd target=serial` 触发（需认证/DEV），Serial2 本身不下发配网命令，只收结果帧。
6. **cloud 上报是同步 HTTPS POST**（1–3s 阻塞主循环），仅低频（首报 + 5min）；弱网长时间失败时有 1min 快速重试上限保护（成功前），代码注释已提示后续可异步化。
7. **TUI 波形默认关闭**（`tui.setWaveformEnabled(false)`），`WAVE_WIDTH/HEIGHT` 宏保留仅为兼容；若重新启用需评估 250ms 单帧预算（`evalDegrade` 会因渲染超时置 degradeReason 0x04）。
8. **对调模式下烧录与遥测共口**：USB Type-C 被主遥测占用，`arduino-cli` 烧录前需停掉上位机进程；`config.yaml` 的 `dtr_rts` 复位方式不受影响（下载口即 UART0，复位时序在上电阶段完成）。
