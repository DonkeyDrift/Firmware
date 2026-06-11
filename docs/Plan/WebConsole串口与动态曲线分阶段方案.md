# Web Console 串口打印、命令发送与动态曲线开发方案

## Context

MUS4 当前已完成 Wi-Fi Console 与 Web Console 的基础闭环：ESP32 启动 `ENABLE_WIFI_CONSOLE` 后提供 AP、TCP Console、Web Console、认证门控和 OTA 窗口。现有 Web Console 只提供简单命令输入、快捷按钮、状态文本刷新和返回日志追加，调试体验仍接近“单次命令请求”，缺少持续串口打印视图和车辆运行数据曲线。

本方案目标是在不改变车辆控制安全逻辑、不引入额外前端依赖、不影响 RC/Pilot 主控制链路实时性的前提下，分阶段增强 Web Console：先做串口式打印与发送，再做动态数据曲线，最后做稳定性与验收收尾。

## 现有可复用入口

- `mus4.ino` 中 `processWirelessConsoleLine(const String& line, Print& out)`：Web/TCP 无线命令统一处理入口，继续复用，避免 Web Console 自建命令解析。
- `mus4.ino` 中 `handleWifiWebCommand()`：现有 `POST /api/cmd`，继续作为命令发送接口。
- `mus4.ino` 中 `printWirelessStatus(Print& out)`：现有 `GET /api/status` 数据源，可继续用于页面顶部摘要状态。
- `mus4.ino` 中 `StringPrint`：可复用为 HTTP 响应拼接工具。
- `mus4.ino` 中 `car_output`、`rc_data`、`pilot_data`、`sensorData`、`wifiConsoleBuf`、OTA 状态变量：作为动态曲线与状态数据源。
- `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py`：继续覆盖认证、Park/OTA 权限策略；如新增 Web API 权限语义，需要同步补测。

## 阶段一：Web Console 串口打印与命令发送体验

### 目标

把 Web Console 从“命令按钮页面”升级为接近串口监视器的调试界面：上方显示连接/状态，中央持续追加打印，底部输入命令并显示响应。

### 后端设计

1. 在 `mus4.ino` 的 `#ifdef ENABLE_WIFI_CONSOLE` 区域新增 Web 日志环形缓冲：
   - 固定容量预分配，例如 4 KiB 或 8 KiB。
   - 记录全局递增序号 `webLogSeq`，前端按 `since` 拉取增量。
   - 溢出时覆盖旧内容，并记录丢弃计数。

2. 新增轻量日志写入函数：
   - `appendWifiWebLog(const char* source, const String& line)` 或等价实现。
   - 先只捕获 Web Console/TCP Console 命令与响应、无线连接事件、OTA 事件，不尝试全量劫持所有 `Serial.print`，降低改动面。
   - 后续如需要“全部 USB Serial 打印镜像”，再单独设计 `Print` 包装器。

3. 新增 API：
   - `GET /api/log?since=<seq>`：返回日志增量。
   - 响应建议用 JSON lines 或简化 JSON：包含 `seq`、`ts_ms`、`source`、`line`。
   - 无新增内容时返回空数组，避免 204 在浏览器 fetch 中增加分支复杂度。

4. 保留 `POST /api/cmd`：
   - 继续调用 `processWirelessConsoleLine()`。
   - 将输入命令和输出响应都写入 Web 日志。
   - 空命令继续返回 `NACK:EMPTY`。

### 前端设计

1. 重写当前内嵌 `WIFI_WEB_CONSOLE_HTML`，仍保持单文件、无外部 CDN。
2. 页面区域：
   - 状态栏：显示 `STATUS` 摘要。
   - 日志窗口：等宽字体、自动滚动、可清空、可暂停滚动。
   - 命令输入：Enter 发送，按钮发送。
   - 快捷命令：`PING`、`STATUS`、`AUTH:mus4-debug`、`ENABLE_OTA`、`OTA_STATUS`。
3. 轮询策略：
   - `/api/log` 每 200 ms 拉取一次。
   - `/api/status` 每 2-5 秒刷新一次。

### 验证

- 浏览器打开 `http://192.168.4.1/`。
- 输入 `PING`，日志窗口出现 `> PING` 和 `PONG`。
- 未认证发送 `0:0`，日志显示 `NACK:UNAUTHORIZED`。
- 发送 `AUTH:mus4-debug` 后再发送 `0:0`，日志显示 `ACK`。
- TCP Console 连接、Web Console 发送命令时，日志均能显示关键事件。

## 阶段二：动态数据 API 与曲线绘制

### 目标

在 Web Console 中实时绘制车辆关键数据曲线，用于调试 RC 输入、Pilot 输出、传感器状态、电流波动和漂移辅助行为。

### 后端设计

1. 新增数据采样环形缓冲：
   - 固定容量，例如 200 点。
   - 采样周期初始设为 50 ms（20 Hz），不要跟随主循环每帧采样。
   - 使用 `lastWebDataSampleMs` 控制采样节奏。

2. 数据点字段建议：
   - `ts_ms`：`millis()` 时间戳。
   - `throttle`：`car_output.throttle`。
   - `steering`：`car_output.steering`。
   - `rc_throttle_pwm`：`rc_data.throttle`。
   - `rc_steering_pwm`：`rc_data.steering`。
   - `pilot_throttle`、`pilot_steering`。
   - `current_mA`：`sensorData.current_mA`。
   - `voltage`：`sensorData.loadVoltage` 或 `busVoltage`。
   - `gyroZ`：`sensorData.gyroZ`。
   - `mode`、`park`。

3. 新增 API：
   - `GET /api/data?since=<seq>`：返回新增采样点。
   - `GET /api/data/latest` 可选：返回最近 N 点，用于页面首次加载。

4. 数据格式：
   - 优先 JSON，字段短名可减少响应体积，例如 `t`、`thr`、`str`、`cur`、`gz`。
   - 对 ESP32 内存敏感，避免一次构造过大的 `String`；如果实现复杂，第一版限制返回最近 100 点。

### 前端设计

1. 使用原生 `<canvas>` 绘图，不引入 Chart.js 等外部依赖。
2. 曲线分组：
   - 控制曲线：Throttle / Steering，范围 -100 到 100。
   - RC PWM 曲线：CH1 / CH2，范围 1000 到 2000。
   - 传感器曲线：Current / GyroZ，可自动缩放或固定范围。
3. 页面控件：
   - 暂停/继续。
   - 清空曲线。
   - 选择显示通道。
   - 显示当前最新数值。
4. 刷新策略：
   - `/api/data` 每 100-200 ms 拉取一次。
   - Canvas 本地重绘，不要求每次数据点都触发完整 DOM 更新。

### 验证

- 转动方向/油门输入，Throttle/Steering 曲线实时变化。
- 改变 RC 输入，RC PWM 曲线变化。
- 车辆静止时 GyroZ 接近稳定；移动/旋转时可见波动。
- 电机动作时 Current 曲线有变化。
- 页面打开 10 分钟，ESP32 不重启，主控制无明显卡顿。

## 阶段三：稳定性、安全与验收

### 稳定性

1. 所有缓冲区固定大小，避免高频动态分配。
2. HTTP 响应限制最大返回点数和日志行数。
3. 拉取频率在前端限流，后端不做阻塞等待。
4. Web Console 更新放在现有 `updateWifiWebConsole()` 中，保持 `handleClient()` 非阻塞调用。
5. 页面 HTML 仍存放在 `PROGMEM`，避免占用过多 RAM。

### 安全边界

1. 命令权限不新开口子：所有控制命令仍通过 `processWirelessConsoleLine()` 与 `isWirelessCommandAllowed()`。
2. 日志和数据曲线默认可读，但不得输出 Wi-Fi 密码、OTA 密码或真实密钥。
3. OTA 仍要求认证且 Park 锁定。
4. 如新增“串口转发到 Serial/Serial1 原始通道”的能力，必须单独加认证门控，不放入第一阶段。

### 验收标准

- `pytest tests/test_wireless_console_policy.py -v` 通过。
- 固件可编译上传。
- Web Console 页面可打开。
- 命令发送、响应显示、日志追加可用。
- 曲线能持续显示 throttle、steering、current、gyroZ。
- 未认证控制命令仍被拒绝。
- Park/OTA 门控不回归。
- 页面长时间运行不导致 ESP32 重启或明显控制延迟。

## 需要修改的文件

第一优先级：

- `mus4.ino`
  - 新增日志环形缓冲、数据采样环形缓冲。
  - 新增 `/api/log`、`/api/data` 端点。
  - 更新 `WIFI_WEB_CONSOLE_HTML`。
  - 在 `setupWifiWebConsole()` 注册新端点。
  - 在 `loop()` 或 `updateWifiWebConsole()` 附近增加数据采样调用。

第二优先级：

- `wireless_console_policy.py`
  - 仅当新增 API 涉及权限策略变化时修改。

- `tests/test_wireless_console_policy.py`
  - 仅当新增权限语义时补充测试。

文档：

- `Doc/Valid/无线串口调试验证指南.md`
  - 实现后追加 Web Console 串口打印与动态曲线验收步骤。

## 测试计划

### 桌面测试

```powershell
pytest tests/test_wireless_console_policy.py -v
```

### 编译上传

```powershell
.\arduino-cli-wsl.ps1 -Compile -Upload -Serial
```

或：

```powershell
python arduino-cli.py -cu --no-progress
python arduino-cli.py -s
```

### 真机验证

1. 连接 `MUS4-DEBUG`。
2. 打开 `http://192.168.4.1/`。
3. 验证日志窗口：`PING`、`STATUS`、`AUTH:mus4-debug`、`0:0`。
4. 验证动态曲线：操作 RC 油门/转向，观察曲线变化。
5. 验证安全门控：未认证控制命令拒绝，认证后允许，OTA 仍需 Park 锁定。
6. 长时间运行页面 10 分钟，观察串口日志、页面刷新和车辆控制是否稳定。

## 风险与缓解

- 风险：内嵌 HTML 继续增大，影响可维护性。
  - 缓解：阶段一先保持单文件；若页面复杂度继续上升，再评估拆分生成脚本或压缩 HTML。

- 风险：高频 JSON 构造导致主循环抖动。
  - 缓解：固定采样频率、限制返回点数、避免每帧构造大字符串。

- 风险：日志缓冲误以为是完整 Serial 镜像。
  - 缓解：第一阶段明确只捕获 Web/TCP 命令、响应和关键无线事件；全量 Serial 镜像另立阶段。

- 风险：曲线 API 暴露运行状态。
  - 缓解：当前 Web Console 只在设备 AP/局域网调试使用；控制命令仍需认证，敏感密钥不输出。
