# Web Console 多来源串口窗口与 1 MB 缓存实施方案

## 1. 背景与目标

当前 Web Console（`WebConsoleAssets.h`）已经具备：

- 一个可滚动、可暂停、可清空的日志显示区 `<div id="log">`。
- 一个命令目标下拉框 `<select id="cmdTarget">`，选项为 `Web / Serial / Serial1`，用于决定命令从 Web 端发到哪个物理/逻辑通道。
- 后端通过 `GET /api/log?since=<seq>` 增量推送日志，`WebLogBuffer` 以环形缓冲保存最近 64 条日志条目。

用户新需求：

1. 为 `Web / Serial / Serial1` 三个来源分别建立“窗口/缓存”。
2. 切换到某个来源时，日志区直接显示该来源已缓存的内容。
3. 每个来源缓存上限 **1 MB**；超出时只保留最新数据，最老的数据被替换。
4. 实现相关内容的“读取与转发”。

## 2. 可行性评估

| 约束 | 评估 |
|------|------|
| **1 MB × 3 缓存在 ESP32 端实现** | 不可行。ESP32 可用 RAM 约 320 KB，无法提供 3 MB 连续或分散缓存。 |
| **1 MB × 3 缓存在浏览器端实现** | 可行。现代浏览器可轻松在内存中保存 3 MB 文本；单字符约 2 字节（UTF-16），3 MB 约 150 万字符，内存占用可接受。 |
| **读取 Serial / Serial1 的实际输出** | 不能被动“嗅探” TX 引脚。必须在固件写入 `Serial` / `Serial1` 的代码点同步记录，或者在读取、转发时记录。 |
| **读取 Serial / Serial1 的输入** | 已具备：`SerialLineReader.cpp` 的 `readSerialBuf()` 逐行读取两个串口，可在此处转发到 Web 日志。 |
| **现有增量日志 API** | `/api/log` 已支持按 `seq` 增量拉取，只需保证每条日志带有正确的来源标识 `src`。 |

**结论**：目标可行，但 1 MB 缓存必须放在浏览器前端；固件端只保留轻量环形缓冲用于增量推送。若用户坚持“设备端 1 MB”，需要先确认硬件是否配备 PSRAM/SD 卡并另行设计。

## 3. 关键假设（基于当前需求描述）

1. “缓存上限 1 MB”指**浏览器内存中的每个来源文本缓存**。
2. “来源”分类为：
   - **Web**：现有 Web Console、TCP Console、Wi-Fi/OTA/启动/传感器等通过 `mus4Log` / `appendWebLog` 产生的日志。
   - **Serial**：从 USB Serial（`Serial`）接收到的命令，以及 Web 端选择 `target=serial` 转发的命令和其响应。
   - **Serial1**：从 RS232 Serial1（`Serial1`）接收到的命令、`Txx:Sxx` 遥测，以及 Web 端选择 `target=serial1` 转发的命令和其响应。
3. 复用原有的命令目标下拉框 `cmdTarget`（`Web / Serial / Serial1`）同时作为**日志来源选择器**：切换选项时既决定命令发往哪个通道，也决定日志窗口显示哪个来源的缓存。

## 4. 推荐方案（方案 A）

### 4.1 前端设计

复用已有的命令目标下拉框 `cmdTarget`（`Web / Serial / Serial1`），为其添加 `change` 事件，在切换命令目标的同时切换日志窗口：

```js
cmdTarget.addEventListener('change', e => { switchLogSource(e.target.value) });
```

新增状态：

```js
const LOG_SOURCE_MAX_BYTES = 1024 * 1024; // 1 MB
const sourceBuffers = { web: '', serial: '', serial1: '' };
let currentLogSource = 'web';
```

核心函数：

- `appendLogLine(text, src)`：将一行文本追加到 `src` 对应的缓冲。若缓冲总长度超过 `LOG_SOURCE_MAX_BYTES`，从头部按整行删除最老内容，直到低于上限。如果 `src === currentLogSource` 且未暂停，再同步到 `#log` DOM 并滚动到底部。
- `switchLogSource(src)`：切换 `currentLogSource`，同步 `cmdTarget.value`，将 `#log.textContent` 替换为对应缓冲内容，并滚动到底部。
- `clearLog()`：清空当前来源的缓冲和 `#log` DOM。
- `pollLog()`：从 `/api/log` 拉取新条目后，根据后端返回的 `src` 字段映射到 `web/serial/serial1`，调用 `appendLogLine`。

这样切换来源时无需重新请求，瞬间显示已缓存内容。

### 4.2 后端设计

保持 `WebLogBuffer` 容量不变（64 条 × 160 字节 ≈ 10 KB），仅要求所有写入日志的条目使用**规范来源名** `"web"`、`"serial"`、`"serial1"`。

#### 4.2.1 Web 来源

现有 `appendWebLog("web", ...)` / `appendWebLog("cmd", ...)` / `appendWebLog("tcp", ...)` / `mus4LogLine("wifi", ...)` 等条目，统一归入 `"web"` 来源。可在 `appendWebLog` 内部做一次来源归类：

```cpp
static const char* canonicalLogSource(const char* source) {
    if (strcmp(source, "serial") == 0 || strcmp(source, "serial1") == 0) return source;
    return "web";
}
```

这样前端无需关心历史来源名称。

#### 4.2.2 Serial 来源

在 `SerialLineReader.cpp` 中：

1. 收到 `Serial` 的完整一行后，在调用 `dispatchCommandLine` 之前或之后，将去敏感化后的命令追加到 Web 日志：
   ```cpp
   appendWebLog("serial", redactWirelessConsoleLine(String(sb.buf)));
   ```
2. 为了捕获命令响应，改用 `StringPrint` 先收集响应，再写入 `Serial` 并追加到日志：
   ```cpp
   String response;
   StringPrint out(response);
   dispatchCommandLine(line, out, sb);
   ser.print(response);
   appendWebLogLines("serial", response);
   ```

在 `WebConsoleServer.cpp` 的 `handleWifiWebCommand()` 中，当 `target == "serial"` 时：

```cpp
appendWebLog("serial", String("> ") + redactWirelessConsoleLine(line));
Serial.println(line);
```

#### 4.2.3 Serial1 来源

与 Serial 对称：

1. `SerialLineReader.cpp` 中收到的 `Serial1` 完整行追加 `src = "serial1"`。
2. `Serial1` 命令响应同样用 `StringPrint` 收集后追加 `src = "serial1"`。
3. `MUS4_FW.ino` 中的周期性 Serial1 上行帧同步追加（v1.7.13 起 `T..S..` 已去冒号、并新增 `M:P` / `$IMU`，详见 CHANGELOG）：
   ```cpp
   String telem = String("T") + car_output.throttle + "S" + car_output.steering + "\n";
   Serial1.print(telem);
   appendWebLog("serial1", telem);
   ```
4. `handleWifiWebCommand()` 中 `target == "serial1"` 时追加 `src = "serial1"`。

#### 4.2.4 安全与隐私

- 所有来自用户输入的串口/Web 行都必须先经过 `redactWirelessConsoleLine()`，避免 `AUTH:`、`WIFI_STA_PASSWORD:` 等敏感信息进入日志。
- 现有 `WebLogBuffer` 仍只保存最近 64 条；即使被未授权读取 `/api/log`，也看不到历史敏感内容。
- 不改变任何认证、Park 锁定或 OTA 权限路径。

### 4.3 API 与数据格式

复用 `GET /api/log?since=<seq>`，返回格式不变：

```json
{
  "dropped": 0,
  "entries": [
    {"seq": 1, "t": 1234, "src": "web",   "line": "..."},
    {"seq": 2, "t": 1235, "src": "serial","line": "..."},
    {"seq": 3, "t": 1236, "src": "serial1","line": "..."}
  ]
}
```

`src` 仅取 `"web"`、`"serial"`、`"serial1"` 三个值之一，前端直接匹配。

### 4.4 缓存淘汰策略

- 按**字符长度**（`String.length`）计算每个来源缓冲大小。
- 上限 `1,048,576` 字符。
- 追加新行时，若超过上限，循环删除缓冲中最早的一行（从开头到第一个 `\n`），直到剩余空间足够；至少保留最新一行。
- 该策略保证：
  - 切换来源时立即显示。
  - 内存占用有硬上限。
  - 最老数据按整行丢弃，避免显示半行。

## 5. 最终采用的 UI 方案

**复用现有 `cmdTarget` 下拉框作为“命令目标 + 当前日志来源”**。切换选项时同时改变命令去向和显示缓存；浏览器仍维护三个来源的 1 MB 缓冲，切换时即时显示对应缓存。

该方案去掉了独立的 `logSource` 下拉框，避免界面上出现两个完全相同的 `Web/Serial/Serial1` 下拉框，符合用户“去掉多余的串口来源选项”的要求。

## 6. 改动文件清单

| 文件 | 改动内容 |
|------|----------|
| `WebConsoleAssets.h` | 复用 `cmdTarget` 作为日志来源选择器；新增三来源缓冲与 `appendLogLine / switchLogSource / clearLogSource` 逻辑；更新 `pollLog()` 来源映射。 |
| `WebLogBuffer.cpp` | 在 `appendWebLog` 中加入规范来源归类，保证 `src` 只输出 `web/serial/serial1`。 |
| `SerialLineReader.cpp` | 读取 `Serial`/`Serial1` 的输入后追加对应来源日志；用 `StringPrint` 捕获命令响应并追加。 |
| `WebConsoleServer.cpp` | `handleWifiWebCommand()` 中 `target=serial/serial1` 时以对应来源记录转发的命令。 |
| `MUS4_FW.ino` | `Serial1.printf("T%d:S%d\n")` 遥测同步追加 `serial1` 来源日志。 |
| `tests/test_firmware_feature_flags.py` | 补充/更新关于日志窗口、下拉框、缓存上限相关的结构断言。 |
| `wireless_console_policy.py` / `tests/test_wireless_console_policy.py` | 若新增命令或权限语义才需修改；本方案预计无需改动。 |

## 7. 实现步骤

1. **后端来源规范与捕获**
   - 修改 `WebLogBuffer.cpp` 统一 `src` 输出。
   - 修改 `SerialLineReader.cpp` 捕获 Serial/Serial1 输入与响应。
   - 修改 `WebConsoleServer.cpp` 中转发命令的来源标签。
   - 修改 `MUS4_FW.ino` 中 Serial1 遥测的双写。

2. **前端多来源缓存**
   - 在 `WebConsoleAssets.h` 中为 `cmdTarget` 添加 `change` 事件，使其同时切换日志来源。
   - 重写 `line()` 为 `appendLogLine(text, src)`，实现 1 MB 环形文本缓冲。
   - 修改 `clearLog()`、`togglePause()` 以适配多来源。

3. **测试与回归**
   - 更新 `tests/test_firmware_feature_flags.py` 中关于 HTML 结构的断言。
   - 运行 `pytest tests/`。
   - 使用 WSL 编译：`.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`。

4. **真机验证**
   - 打开 Web Console，切换 `Web / Serial / Serial1`，确认各自显示对应内容。
   - 在 Web 端选择 `target=serial` 发送命令，确认 Serial 窗口出现 `> 命令`。
   - 通过 USB Serial/Serial1 发送命令，确认对应窗口出现输入与 ACK。
   - 持续产生日志，验证 1 MB 上限下最老行被整行丢弃。

## 8. 验证标准

- `pytest tests/test_firmware_feature_flags.py` 通过。
- `pytest tests/test_wireless_console_policy.py` 通过（无权限语义变化时自然通过）。
- WSL 编译无错误。
- Web Console 三个来源切换无延迟，缓存内容正确。
- 敏感命令（`AUTH:`、`WIFI_STA_PASSWORD:`）不会以明文出现在任何来源窗口。
- OTA、Park、认证等安全路径行为无回归。

## 9. 风险与缓解

| 风险 | 缓解 |
|------|------|
| 设备端 RAM 不足 | 固件缓冲保持 64 条不变，1 MB 缓存完全放在浏览器。 |
| Serial/Serial1 响应捕获引入额外 String 拷贝 | 响应长度受 `SerialBuf` 256 字节限制，拷贝开销极小。 |
| 高频遥测（Serial1 `Txx:Sxx` 500 Hz）导致日志条目过多 | 后端仍只保留 64 条；前端 1 MB 足以保存大量短行，不影响控制循环。 |
| 来源切换时 DOM 内容瞬间替换造成滚动抖动 | 替换后主动设置 `scrollTop = scrollHeight`。 |
| 测试断言因 HTML 结构调整失败 | 同步更新 `test_firmware_feature_flags.py`。 |

## 10. 待用户确认事项

1. 1 MB 缓存是否确实放在浏览器端？若要求设备端 1 MB，需要额外硬件（PSRAM/SD）支持，方案需重新设计。
2. （已确认）复用现有的 `cmdTarget` 下拉框作为日志来源选择器，不再新增独立的 `logSource`。
3. Serial/Serial1 窗口是否还需要显示设备主动输出的诊断日志（如 `LOG_SERIAL` 模式下 `Serial.println` 的内容）？当前方案默认只捕获串口输入与命令响应；若需要，可增加可选的“镜像 Serial 诊断日志到 Web”功能。
