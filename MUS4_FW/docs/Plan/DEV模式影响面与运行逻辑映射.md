# DEV 模式影响面与运行逻辑映射

> 配套阅读：[`开发模式与OTA自动Park方案.md`](开发模式与OTA自动Park方案.md) 是设计意图原稿；
> 本文档是**当前实现的事实映射**——把每条放权落到具体源码位置、状态字段、执行链路，
> 便于排障、安全审计与后续修改时确认影响面。
>
> 适用版本：`MUS4_FIRMWARE_VERSION = v1.7.8`（`BuildInfo.h`）。v1.7.6 之前的 DEV 安全边界存在历史偏差，详见 §3.1；v1.7.8 起 DEV 不再影响 Serial1 遥测与 AP 广播 SSID，详见 §2.3 与 §4。
> 修改 DEV 相关代码时，请同步更新本文件，并同步检查
> `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py`。

## 1. 状态字段与持久化

| 项 | 位置 | 说明 |
| --- | --- | --- |
| 运行时标志 | `libraries/mus4_core/src/RuntimeState.h:41` — `WifiRuntimeState::devModeEnabled` | 真源；默认 `false` |
| NVS 命名空间 | `libraries/mus4_core/src/WifiConsoleTypes.h:32` — `MUS4_PREF_NAMESPACE = "mus4"` | |
| NVS 键 | `libraries/mus4_core/src/WifiConsoleTypes.h:33` — `MUS4_PREF_DEV_MODE_KEY = "dev_mode"` | 布尔，跨重启保留 |
| 加载入口 | `libraries/mus4_wifi/src/WifiManager.cpp:192` — `loadDevModePreference()` | `setup()` 中、Web Console 启动前调用 |
| 保存入口 | `libraries/mus4_wifi/src/WifiManager.cpp:204` — `saveDevModePreference()` | 仅切换时写 flash |

`saveDevModePreference()` 是触发链的中枢，**一次保存联动四件事**（v1.7.8 收敛后简化）：

1. 落 NVS（`Preferences::putBool`）。
2. 更新 `wifiDevModeEnabled` 运行时标志。
3. 若开启：`keepDevModeOtaWindowActive(otaRuntime, wifiRuntime)`——立刻把 OTA 窗口"钉住"。
4. 若关闭且窗口已开但未在传输中：`closeWifiOtaWindow("DEV_MODE_OFF", otaRuntime)`——立刻关闭 OTA。

> v1.7.8 起 **不再调用** `scheduleWifiApRestart()`：AP 广播 SSID 派生只看 STA 连接状态，与 DEV 解耦，
> 切换 DEV 不会丢一次 AP/Web Console 连接。详见 §4。

## 2. `DEV ON` 实际放权的功能点

以下所有放权**只对 `origin == WIRELESS_ORIGIN_WEB` 生效**（即 HTTP `/api/cmd` 与
Web 配置 API）。TCP Console（端口 2323）、Serial、Serial1 完全不受 DEV 影响。

### 2.1 Web 命令未认证守卫被绕过

- 位置：`libraries/mus4_command/src/WirelessConsole.cpp:136-141`
- 关键代码：
  ```cpp
  bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
  ...
  if (!ws.consoleAuthenticated && !webDevMode) return false;
  ```
- 影响：未认证 Web 请求可继续走到下游分类判断。

### 2.2 OTA 三命令免认证

| 命令 | 普通态需要 | `DEV ON` 时（Web） | 源码位置 |
| --- | --- | --- | --- |
| `ENABLE_OTA` | 认证 + Park 锁定 | **免认证**，Park 锁定**仍必需** | `WirelessConsole.cpp:139`、`WifiOta.cpp:42-52` |
| `OTA_STATUS` | 认证 | **免认证** | `WirelessConsole.cpp:140` |
| `DISABLE_OTA` | 认证 | **免认证** | `WirelessConsole.cpp:140` |

`ENABLE_OTA` 内部仍有 `if (car_output.park != PARK_LOCKED) { out.println("NACK:PARK_REQUIRED"); ... }`
（`WifiOta.cpp:48-52`），**Park 锁定要求不被 DEV 放宽**。

### 2.3 OTA 窗口"常开化"

- 位置：`libraries/mus4_wifi/src/WifiOta.cpp:27-33, 98-110, 127-133`
- 三处协同：
  - `keepDevModeOtaWindowActive()`：每次刷新 `os.windowOpen = true`、`deadlineMs = millis() + WIFI_OTA_WINDOW_MS`。
  - `updateWifiOta()` 第 100 行：`if (ws.devModeEnabled) keepDevModeOtaWindowActive(os, ws);`——每个 loop tick 重新钉住窗口。
  - `updateWifiOta()` 第 106 行：超时关闭的前提是 `!ws.devModeEnabled`，所以 **`DEV ON` 时窗口永不超时**。
  - `wifiOtaTtlMs()` 第 130 行：DEV 模式 TTL 始终返回 `WIFI_OTA_WINDOW_MS` 满值。
- 净效果：ArduinoOTA 端口 3232 **持续监听**，不需要每两分钟重新点 `ENABLE_OTA`。

> **v1.7.8 起：Serial1 遥测不再被窗口期影响**。`shouldEmitSerial1Telemetry()` 改为 `return !os.inProgress;`，
> 只在 OTA 真正传输期间暂停，DEV ON 窗口长期打开不再阻塞 ESP32 与上位机的串口通信。
> Park Guard 仍由 `forceWifiOtaParkLocked()` 在传输期内托底，安全性不降。

### 2.4 HTTP `/update` 端点免认证

- 位置：`libraries/mus4_web/src/WebConsoleServer.cpp:654-658`
- 关键代码：
  ```cpp
  static bool isWifiWebUpdateAuthOk()
  {
      if (ws.devModeEnabled) return true;
      if (ws.consoleAuthenticated) return true;
      // ...还支持一次性查询参数 token
  }
  ```
- 影响：`arduino-cli-wsl.ps1 -HttpOta` 路径下，DEV 状态时可以直接 `curl POST /update` 上传固件，无需 `AUTH:` 或 token。

### 2.5 Web 配置类 API 免认证

DEV 还放权了以下 Web 配置端点，统一模式 `if (!ws.consoleAuthenticated && !ws.devModeEnabled)`：

| 端点 | 源码位置 | 功能 |
| --- | --- | --- |
| `POST /api/wifi-ap` | `WebConsoleServer.cpp:311` | 修改 AP SSID 前缀 |
| `POST /api/wifi-sta` | `WebConsoleServer.cpp:479` | 保存 STA SSID（不含密码） |
| `POST /api/wifi-sta-password` | `WebConsoleServer.cpp:386` | 保存 STA 密码 |
| `POST /api/wifi-sta-clear` | `WebConsoleServer.cpp:524` | 清除 STA 配置 |

### 2.6 控制台显示/日志切换命令未认证

- 位置：`WirelessConsole.cpp:141, 143`
- 命中命令：`ANSI` / `NOANSI` / `FILTER_DEBUG` / `LOG_WEB` / `LOG_SERIAL` / `WIFI_STA_SSID` / `WIFI_STA_PASSWORD` / `WIFI_STA_APPLY` / `WIFI_STA_CLEAR`
- 机制：第 141 行 `webDevMode` 跳过未认证守卫之后，第 143 行 `return true` 直接放行。
- ⚠️ 这是设计意图（与 `wifi-sta` 配置类 API 一致），但调用方不可借此**间接**改 STA 凭据后接管设备——`WIFI_STA_*` 命令本身仍写入 NVS，等同 §2.5。

### 2.7 状态报文字段

- `STATUS` / `OTA_STATUS` 输出携带 `dev_mode=1`：
  - `WebConsoleServer.cpp:92`（Web `STATUS`）
  - `WifiOta.cpp:145`（`OTA_STATUS`）
- 便于 `wireless_console_policy.py` 与自动化脚本判定当前 DEV 状态。

### 2.8 校准命令免认证（Park 锁定仍必需）

- 位置：`WirelessConsole.cpp:165-166`
- 关键代码：
  ```cpp
  // DEV ON 允许校准命令免认证（但仍需 Park 锁定）。
  if (webDevMode && isCalibrationCommand(line)) return car_output.park == PARK_LOCKED;
  ```
- 命中命令（`isCalibrationCommand`，WirelessConsole.cpp:138-151）：
  `STEER_CAL` / `CAL_SAVE` / `CAL_RETRY` / `CAL_ABORT` / `CAL_RESET` / `CAL_STATUS`
  / `JOYSTICK_CAL` / `JOYSTICK_SAVE` / `JOYSTICK_RETRY` / `JOYSTICK_ABORT` / `JOYSTICK_RESET`
- 影响：DEV ON + Web 来源时，上述命令**免认证**，但 **Park 锁定要求不变**——未锁 Park 时
  返回 `NACK:PARK_REQUIRED`（而非 `NACK:UNAUTHORIZED`，见 NACK 分流逻辑
  `WirelessConsole.cpp:189-191`）。
- 不受影响的命令：`TEST` / `BENCH` / `STRESS` / `REGRESS` / `FILTER_TEST` / `TEST_TUI`
  仍严格要求认证（不属于 `isCalibrationCommand`）。

## 3. `DEV ON` **不会**放权的东西

`webDevMode` 这条放行只出现在 §2 列出的位置；以下入口与功能在源码中**不读 `devModeEnabled`**
或对 DEV 显式拒绝，是有意为之的安全边界：

| 功能 | 不被放权的原因 / 源码位置 |
| --- | --- |
| **控制输出命令**（`Throttle:Steering` / `10:20` 等） | v1.7.7 收敛后 `WirelessConsole.cpp` 把控制命令分支放在 `if (!ws.consoleAuthenticated) return false;` 之后；DEV ON + 未认证 → `NACK:UNAUTHORIZED`。见 §3.1。 |
| **诊断 / 维护命令**（`TEST` / `BENCH` / `REGRESS` / `STRESS` / `FILTER_TEST` / `TEST_TUI`） | 同上；DEV ON 不放权，即使 Park 锁定也要求认证。见 §3.1。 |
| **校准命令**（`STEER_CAL` / `CAL_*` / `JOYSTICK_CAL` / `JOYSTICK_*`，不含 `JOYSTICK_STATUS`） | v1.7.30 起 DEV ON + Web 来源**免认证**（§2.8），但 Park 锁定要求不变。TCP / Serial 来源 DEV 不生效。 |
| **TCP Console（端口 2323）任何命令** | `WirelessConsole.cpp` —— `webDevMode` 要求 `origin == WIRELESS_ORIGIN_WEB`；TCP 来源 `webDevMode` 始终为 `false`。 |
| **Serial / Serial1 物理串口的 OTA 命令** | 本地路径走 `processLocalOtaMaintenanceCommand`（`WifiOta.cpp:80-96`）与 `openLocalWifiOtaWindow`（`WifiOta.cpp:63-78`），不读 `devModeEnabled`；本地 OTA 仍要求 `ENABLE_OTA:<密码>` 形式。 |
| **`ENABLE_OTA` 的 Park 锁定要求** | `WirelessConsole.cpp` `isWirelessOtaOpenCommand` 分支末尾 `&& car_output.park == PARK_LOCKED`、`WifiOta.cpp:48-52` 二次确认；DEV 不放宽。 |
| **OTA Park Guard** | OTA 窗口一旦打开，`forceWifiOtaParkLocked()`（`WifiOta.cpp:20-25`）强制 `rc_data.park = PARK_LOCKED`、`car_output.park = PARK_LOCKED`、`car_output.throttle = 0`；`updateWifiOta()` 第 102 行在 `os.inProgress || os.parkGuardActive` 时每个 tick 重新强制；DEV 不能跳过。 |

## 3.1 历史偏差与收敛（v1.7.6 → v1.7.7 修复）

### 历史偏差

v1.7.6 之前的实现里 `WirelessConsole.cpp:141` 把 `webDevMode` 当作"等价于已认证"使用，
142、144 行没有再单独排除 `webDevMode`。这意味着 **DEV ON 的 Web 来源未认证请求也可发控制/诊断命令**，
与设计稿 `开发模式与OTA自动Park方案.md` §3 第 48–49 行"dev mode 不放宽控制命令/诊断命令"的意图不一致。

### 收敛方向（v1.7.7）

按"回到设计稿"方向收敛 DEV 安全边界。`isWirelessCommandAllowed` 重排序：

```cpp
bool isWirelessCommandAllowed(const String& line, WirelessCommandOrigin origin, WifiRuntimeState& ws)
{
    bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (line.equalsIgnoreCase("PING") || ...) return true;                                  // 公开命令
    if (line.startsWith("AUTH:")) return true;
    if (isWirelessOtaOpenCommand(line))    return (webDevMode || ws.consoleAuthenticated)
                                                  && car_output.park == PARK_LOCKED;       // §2.2
    if (isWirelessOtaStatusCommand(line) || isWirelessOtaCloseCommand(line))
        return webDevMode || ws.consoleAuthenticated;                                       // §2.2
    // DEV ON 显式白名单：显示/日志切换、Wi-Fi STA 配置类命令。
    if (line.equalsIgnoreCase("ANSI") || ...
        || isWifiStaConfigCommand(line))   return ws.consoleAuthenticated || webDevMode;    // §2.5 §2.6
    // DEV ON 允许校准命令免认证（但仍需 Park 锁定）。                                // §2.8 (v1.7.30)
    if (webDevMode && isCalibrationCommand(line)) return car_output.park == PARK_LOCKED;
    // 其余命令（控制 / 诊断）严格要求认证，不读 webDevMode。
    if (!ws.consoleAuthenticated) return false;
    if (isParkLockedWirelessCommand(line)) return car_output.park == PARK_LOCKED;
    return isWirelessControlCommand(line);
}
```

`processWirelessConsoleLine` 的 NACK 错误码同步收敛：未认证用户（即使 DEV ON）对非校准命令一律返回
`NACK:UNAUTHORIZED`；校准命令在 DEV ON + Park 未锁时返回 `NACK:PARK_REQUIRED`（v1.7.30）。

### 收敛后效果

- **控制命令** `10:20` / `Throttle:Steering` → DEV ON + 未认证 → `NACK:UNAUTHORIZED`。
- **非校准诊断命令** `TEST` / `BENCH` / `REGRESS` / `STRESS` / `FILTER_TEST` → DEV ON + 未认证 → `NACK:UNAUTHORIZED`（无论 Park 状态）。
- **校准命令** `STEER_CAL*` / `CAL_*` / `JOYSTICK_CAL*` / `JOYSTICK_*`（不含 `JOYSTICK_STATUS`）→ DEV ON + Park 已锁 → 放行；Park 未锁 → `NACK:PARK_REQUIRED`（v1.7.30）。
- **显示/日志切换 + WIFI_STA_*** → 保持 DEV ON 可放权（设计稿一致）。
- **OTA 三命令 + `/api/wifi-*` + `/update`** → 保持 DEV ON 可放权（核心便利不变）。

### 测试保护

`tests/test_wireless_console_policy.py::test_web_dev_mode_does_not_bypass_authentication_for_control_or_diagnostic`
锁定控制命令与非校准诊断命令（`10:20`、`TEST`、`BENCH`、`REGRESS`）在 DEV ON + 未认证下应被拒绝。
`tests/test_wireless_console_policy.py::test_web_dev_mode_allows_calibration_without_auth_but_keeps_park_guard`
锁定校准命令（`STEER_CAL` / `JOYSTICK_CAL` 等）在 DEV ON 下免认证但保留 Park 锁定的行为（v1.7.30）。

## 3.2 控制台密码为空时全通道免认证（v1.7.67）

`WIFI_CONSOLE_AP_PASSWORD` 为空字符串（当前配置）时，开放 AP 下任何人发送 `AUTH:`（冒号后留空）
都必然得到 `AUTH_OK`，认证门禁只剩操作摩擦、不提供实际安全性。为此固件新增编译期判定
`isWirelessConsoleAuthDisabled()`（`libraries/mus4_core/src/WifiConsoleTypes.h`）：密码为空时以下门禁
一律把会话视为已认证，**任何工具、任何来源都无需先发 `AUTH:`**：

- `WirelessConsole.cpp` `isWirelessCommandAllowed`（`authed = ws.consoleAuthenticated || isWirelessConsoleAuthDisabled()`），
  覆盖 TCP:2323 与 `/api/cmd`、校准命令分发；NACK 分流同步把"Park 未锁"报为 `NACK:PARK_REQUIRED`；
- `WifiOta.cpp` `openWifiOtaWindow` 的 `NACK:AUTH_REQUIRED` 检查；
- `WebConsoleServer.cpp` 5 处 `/api/wifi-*` 配置端点的 403 检查与 `isWifiWebUpdateAuthOk()`（HTTP OTA）。

**不受影响的安全维度**：Park 锁定要求（`TEST`/`BENCH`/校准/`ENABLE_OTA` 等）原样保留；`AUTH:` 命令
行为不变（空密码返回 `AUTH_OK`）。一旦 `WIFI_CONSOLE_AP_PASSWORD` 配置为非空，所有门禁自动恢复
原语义，无需改动任何判断点。

测试锁定：`tests/test_wireless_console_policy.py::TestWirelessConsoleAuthDisabled`（免认证放行 +
Park 规则不变 + 默认 `auth_disabled=False` 语义不变）。

## 4. AP 广播 SSID 派生（已退役 v1.7.22）

历史 v1.7.7 / v1.7.8 引入过"STA 连接后，AP SSID 自动追加 STA 短码 + IP 尾段"（如 `MU04-ESP-DON-3.43`）的派生逻辑。**v1.7.22 起整体移除**：

- 触发前提（v1.7.18 起 AP/STA 互斥切换）：AP 与 STA 永远不会同时广播——STA 上线后 1s grace 通过即 `stopWifiApForStaOnly()` 关 AP。STA 在线时根本扫不到 AP，派生 SSID 失去意义。
- 实现移除：`buildWifiDevApSsid()` / `wifiStaSsidShortUpper()` / `wifiStaIpTailText()` 三个 static 函数删除；`getActiveWifiApSsid()` 简化为直接 `return String(wifiApSsid);`。
- UI 文案同步删除：Web Console AP 配置面板不再出现「开启 DEV 模式且 STA 连接成功后，AP 名称会自动追加 STA SSID 前 3 位大写和 IP 后两段」。
- 保留：`WIFI_AP_SSID_SUFFIX="-ESP"` / `WIFI_AP_SSID_PREFIX_MAX_LEN=6` 是基础命名规则，与派生无关。
- 兼容性：DEV 模式与 AP SSID 完全解耦——这一点 v1.7.8 时已断开，本次只是把"始终派生"也一起去掉。`/api/wifi-ap` 与 `/api/status` 返回的 `ap_ssid` 始终是基础名。

> 历史细节（已不存在于代码）：派生格式曾为 `<前缀>-ESP-<STA短码3字符大写>-<STA IP 后两段>`，
> 由 `updateWifiSta()` 在 STA 上线分支里通过 `scheduleWifiApRestart()` 排队重启 AP 切换广播。
> v1.7.22 起 STA 上线分支不再排队 AP 重启（也不再调 `getActiveWifiApSsid()` 比对），分支退化为只武装 up grace。

## 5. `DEV ON` 完整执行链（典型 HTTP OTA 场景）

```text
浏览器拨动 DEV 开关
  └─ POST /api/devmode  body=1
        └─ handleWifiWebDevModeSet (WebConsoleServer.cpp:266)
              ├─ saveDevModePreference(true)
              │     ├─ Preferences.putBool("mus4"/"dev_mode", 1)        ← 持久化
              │     ├─ wifiDevModeEnabled = true                         ← 运行时标志
              │     ├─ keepDevModeOtaWindowActive(otaRuntime, wifiRuntime)
              │     │     ├─ ensureWifiOtaStarted()                      ← ArduinoOTA.begin() 监听 3232
              │     │     ├─ os.windowOpen = true
              │     │     └─ os.deadlineMs = now + WIFI_OTA_WINDOW_MS    ← 120s（后续每 tick 重置）
              │     └─ scheduleWifiApRestart()                            ← 如已连 STA → 改广播 SSID
              └─ 200 {"enabled":true,"saved":true}

[arduino-cli-wsl.ps1 -HttpOta 直接 push 固件]
curl POST /update  (multipart firmware.bin)
  └─ handleWifiWebUpdateUpload (WebConsoleServer.cpp 内 /update 路由)
        ├─ isWifiWebUpdateAuthOk()
        │     └─ ws.devModeEnabled → return true                          ← §2.4
        └─ Update.write(...) → Update.end() → restart

[loop() 中持续维护，OTA 上传期间]
loop()
  ├─ updateWifiOta(otaRuntime, wifiRuntime)
  │     ├─ ws.devModeEnabled → keepDevModeOtaWindowActive()               ← §2.3 每 tick 重置
  │     ├─ os.windowOpen → 进入处理
  │     ├─ os.inProgress || os.parkGuardActive → forceWifiOtaParkLocked() ← §3 末行 / 双向保护
  │     │     ├─ rc_data.park = PARK_LOCKED
  │     │     ├─ car_output.park = PARK_LOCKED
  │     │     └─ car_output.throttle = 0                                  ← 油门强制为 0
  │     ├─ !ws.devModeEnabled && timeout → closeWifiOtaWindow("TIMEOUT")  ← DEV 时跳过
  │     └─ ArduinoOTA.handle()                                            ← 端口 3232 同样可接 OTA
  └─ ...
```

## 6. 安全意义小结

`DEV ON` 的设计原意是 **"只放权 Web 上传 + 配置面板"** 的开关；v1.7.8 收敛后落地五层安全网 + 两条解耦：

1. **Park 锁定未放权**：开 OTA 仍要求 `car_output.park == PARK_LOCKED`，开窗后 Park Guard 强制油门=0。
2. **TCP 入口未放权**：TCP Console、Serial、Serial1 都不读 `devModeEnabled`。
3. **控制 / 诊断命令未放权**（v1.7.7）；**校准命令有条件放权**（v1.7.30）：DEV ON 严格只放权 OTA + Web 配置 + 显示/日志切换 + WIFI_STA_*；校准命令（STEER_CAL / JOYSTICK_CAL 等）免认证但仍需 Park 锁定。
4. **Serial1 通信不受窗口影响**（v1.7.8）：DEV ON 时 windowOpen 长期为 true 不再阻塞 ESP32 ↔ 上位机的 Serial1 遥测；仅在 OTA 真正传输期间暂停。
5. **NVS 持久化**：开关状态跨重启保留——升级流程稳定，但同时意味着**忘记关 DEV 的设备永远暴露 `/update` 无密上传**。生产入库前应统一 `DISABLE_OTA` 并把 DEV 关掉。

**已解耦的副作用**（v1.7.8）：

- **AP 广播 SSID** 不再随 DEV 切换变化：始终遵循"STA 已连接 → 派生 SSID；STA 未连接 → 基础 SSID"。
- **AP 重启调度** 不再由 `saveDevModePreference` 触发：切换 DEV 不丢 Web Console 连接。

历史偏差与收敛过程见 §3.1。

## 7. 测试与回归保护

- `wireless_console_policy.py` 是固件无线权限策略的 Python 镜像，桌面侧覆盖未认证/Park/OTA 窗口规则。
- `tests/test_wireless_console_policy.py` 用 pytest 锁定关键策略行为；新增/修改 DEV 边界时**先**在此扩展用例，再改固件，遵循项目 CLAUDE.md 的 TDD 纪律。
- `tests/test_firmware_feature_flags.py` 用源码断言保护若干 Web Console UI / `/update` 端点的关键代码片段，DEV UI 文案与端点路由名变更时一并维护。

## 8. 关键文件清单

- `libraries/mus4_core/src/RuntimeState.h` — `WifiRuntimeState::devModeEnabled` 真源。
- `libraries/mus4_core/src/WifiConsoleTypes.h` — NVS namespace 与 key 常量。
- `libraries/mus4_wifi/src/WifiManager.cpp` — load/save、AP 重启、动态 SSID 派生。
- `libraries/mus4_wifi/src/WifiOta.cpp` — OTA 窗口、Park Guard、`OTA_STATUS` 字段。
- `libraries/mus4_command/src/WirelessConsole.cpp` — `isWirelessCommandAllowed`、`processWirelessConsoleLine`。
- `libraries/mus4_web/src/WebConsoleServer.cpp` — `/api/devmode`、`/api/wifi-*`、`/update` 端点。
- `libraries/mus4_web/src/WebConsoleAssets.h` — DEV 切换开关 UI、`renderDevMode/setDevMode` 前端逻辑。
- `wireless_console_policy.py` + `tests/test_wireless_console_policy.py` — 策略镜像与回归测试。
