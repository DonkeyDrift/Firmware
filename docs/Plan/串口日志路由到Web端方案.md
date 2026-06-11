# 实施计划：串口日志路由到 Web 端 + 打印目标二选一

## Context

当前 MUS4 固件的诊断/状态日志全部通过 `Serial.print*` 输出到 Type-C 串口，Web Console 仅有自身命令历史和 WiFi/OTA 生命周期的少量日志。用户希望：
1. 把串口原本打印的诊断信息同步到 Web 端 `/api/log`；
2. 新增配置，使串口端打印和 Web 端打印二选一，**默认 Web 端打印**。

关键约束：不能影响 `Serial1` 协议回传 `Txx:Sxx`、串口命令的 ACK/NACK 响应、TCP/Web Console 的协议响应，也不能绕过认证/Park 策略。

## 修改文件

- `mus4.ino` — 主要改动
- `wireless_console_policy.py` — 新增打印目标策略函数
- `tests/test_wireless_console_policy.py` — 新增策略测试

## 步骤

### 1. 新增编译宏与运行时变量

在 `mus4.ino` 的 `#define ENABLE_WIFI_CONSOLE` 附近添加：

```cpp
#define MUS4_LOG_TARGET_SERIAL 0
#define MUS4_LOG_TARGET_WEB    1

#ifndef MUS4_LOG_TARGET
#ifdef ENABLE_WIFI_CONSOLE
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_WEB
#else
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_SERIAL
#endif
#endif
```

同时添加运行时切换变量和串口命令：

```cpp
uint8_t mus4LogTarget = MUS4_LOG_TARGET;
```

在 `PROCESS_COMMAND_LINE` 中增加两个诊断命令（需认证）：
- `LOG_WEB` — 切换到 Web 端打印
- `LOG_SERIAL` — 切换到串口端打印

在 `wireless_console_policy.py` 的 `GENERAL_AUTHENTICATED_COMMANDS` 中增加 `"LOG_WEB"`, `"LOG_SERIAL"`。

### 2. 新增路由函数

在 `appendWifiWebLog()` 声明之后添加：

```cpp
static void mus4LogLine(const char* source, const String& line) {
    if (mus4LogTarget == MUS4_LOG_TARGET_WEB) {
#ifdef ENABLE_WIFI_CONSOLE
        appendWifiWebLog(source, line);
#endif
    } else {
        Serial.println("[" + String(source) + "] " + line);
    }
}

static void mus4Logf(const char* source, const char* fmt, ...) {
    char buf[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    mus4LogLine(source, String(buf));
}
```

`WebLogEntry.line` 从 96 提升到 160，以容纳前缀拼接后的内容。

`WIFI_WEB_LOG_CAPACITY` 从 48 提升到 64，缓解 I2C 扫描等密集日志的覆盖速度。

### 3. 替换串口诊断输出

按优先级从高到低，只替换“人类可读诊断日志”，**不碰**协议/响应输出。

#### 3a. TUI 事件日志

将 `tui.log(...)` 改为 `mus4LogLine("tui", ...)`，涉及：
- `"System Locked: Park Mode Active/Entered"`
- `"System Unlocked: Park Mode Exited"`
- `"Start Emergency stop"`
- `"Emergency STOP ready/done"`
- `"Emergency Stop FSM reset"`

Web 模式下这些事件进入 `/api/log`；Serial 模式下仍走 `Serial.println`。

#### 3b. setup() 启动日志

将 Boot、WiFi AP/STA 初始化结果等 `Serial.printf/println` 替换为 `mus4Logf("boot"/"wifi", ...)`。

WiFi 生命周期中已有 `appendWifiWebLog("wifi"/"ota", ...)` 的双写位置，去掉多余的 `Serial.print`，统一走 `mus4LogLine`。

#### 3c. I2C/传感器初始化

`scanI2CBus()`、`setup_ina219()`、`setup_mpu6050()` 中的 `Serial.print*` 替换为 `mus4Logf("i2c"/"ina219"/"mpu6050", ...)`。同一逻辑的连续 `Serial.print` 合并为一行 `mus4Logf`，避免 Web 日志碎片化。

#### 3d. 诊断命令输出

`runFilterTests()`、`runBenchmarks()`、`runStress()`、`runRegression()` 内部的 `Serial.printf` 替换为 `mus4Logf("test", ...)`。命令响应（ACK/NACK）保持原 `out` 输出不变。

#### 3e. 其他 Serial.print

`notifyDegrade()`、`filterDebugEnabled` 下的调试打印、模式切换打印等，按同理替换。不批量替换所有 `Serial.print`，遗漏的留后续迭代。

### 4. TUI 渲染条件化

在 `loop()` 中：

```cpp
if (mus4LogTarget == MUS4_LOG_TARGET_SERIAL) {
    tui.update(millis());
    lastUICycleDuration = tui.getLastRenderDuration();
} else {
    lastUICycleDuration = 0;
}
```

Web 模式下跳过 TUI ANSI 渲染，Web 端已有 `/api/data` + `/api/log` 承接展示。

### 5. 不改动的内容

- `Serial1.printf("T%d:S%d\n", ...)` — 协议回传，不动
- `readSerialBuf(Serial, ...)` / `readSerialBuf(Serial1, ...)` — 输入读取，不动
- `PROCESS_COMMAND_LINE` 中的 `out.println("ACK"/"NACK"/...)` — 命令协议响应，不动
- `wifiConsoleClient.print(...)` — TCP Console 响应，不动
- `appendWifiWebLog("web"/"cmd"/"tcp", ...)` — Web Console 自身历史，不动
- HTTP API 处理函数 — 不动
- 认证/Park/权限判断路径 — 不动

### 6. Python 策略与测试

`wireless_console_policy.py` 新增：

```python
GENERAL_AUTHENTICATED_COMMANDS = {"ANSI", "NOANSI", "FILTER_DEBUG", "LOG_WEB", "LOG_SERIAL"}

def select_log_target(configured, wifi_console_enabled):
    if configured == "web" and wifi_console_enabled:
        return "web"
    return "serial"
```

`tests/test_wireless_console_policy.py` 新增测试：
- `test_log_web_and_log_serial_require_authentication` — 未认证时 LOG_WEB/LOG_SERIAL 被拒
- `test_select_log_target_defaults_to_web_when_wifi_enabled`
- `test_select_log_target_falls_back_to_serial_when_wifi_disabled`
- `test_select_log_target_serial_when_configured`

### 7. Web Console 命令注册

在 `handleWifiWebCommand()` 中增加 `LOG_WEB` / `LOG_SERIAL` 的处理，同步更新 `mus4LogTarget` 变量并返回确认消息。

## 验证

1. **编译**：`python arduino-cli.py -c` 通过
2. **Python 测试**：`pytest tests/test_wireless_console_policy.py` 全绿
3. **固件运行 — Web 模式（默认）**：
   - 打开 Web Console，访问 `/api/log?since=0` 可看到 boot/wifi/i2c/tui 事件
   - Type-C 串口不持续刷 TUI/诊断日志
   - 串口命令输入仍可用，`TEST`/`BENCH` 等响应正常
   - `Serial1` 协议 `Txx:Sxx` 正常
4. **固件运行 — Serial 模式**：
   - 串口发送 `LOG_SERIAL` 后，诊断日志回到 Serial
   - Web Console `/api/log` 不再收到固件诊断日志（命令历史仍正常）
   - 串口发送 `LOG_WEB` 可切回
5. **权限**：未认证发送 `LOG_WEB`/`LOG_SERIAL` 返回 NACK
