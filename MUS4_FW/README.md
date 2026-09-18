# LP-MU-S4 / MUS4 Firmware

[中文文档](README.zh-CN.md)

MUS4 (LP-MU-S4) is an ESP32 + Arduino based low-level control firmware for RC vehicles and robotics platforms. It handles RC PWM input capture, host-side Pilot serial control, multi-mode control blending, Park / emergency braking, I2C sensor sampling, terminal UI output, Wi-Fi/TCP/Web Console, OTA updates, and optional BLE Gamepad output when Wi-Fi Console is disabled.

The current main Arduino sketch is [`MUS4_FW.ino`](MUS4_FW.ino). Firmware version metadata is defined in [`BuildInfo.h`](BuildInfo.h), and release notes are maintained in [`CHANGELOG.md`](CHANGELOG.md).

## Features

- **RC PWM input capture**: CH1-CH6 for steering, throttle, Park, mode selection, Drift Assist enable, and Drift Assist ratio.
- **Pilot serial control**: supports `Throttle:Steering`, sequence-numbered frames, and checksum frames.
- **Three driving modes**: manual, semi-auto, and full-auto control blending.
- **Park / emergency braking**: safety state machine that overrides throttle output and drives LED indication.
- **Actuator PWM output**: ESP32 `ledc` output for steering servo and ESC.
- **Wi-Fi / TCP / Web Console**: wireless command console, status page, logs, charts, and WebSocket telemetry.
- **OTA updates**: ArduinoOTA and Web Console HTTP `/update` upload paths.
- **I2C sensors**: INA219 and MPU6050 sampling.
- **Drift Assist**: steering compensation based on IMU yaw rate and CH5/CH6 state.
- **BLE Gamepad mode**: available only when Wi-Fi Console is not enabled at compile time.

## Hardware Pins

Authoritative pin definitions are documented in [`Doc/Hardware/pin_definitions.md`](Doc/Hardware/pin_definitions.md). The current firmware targets the MUS4 v2.4.2 / v2.3 pin layout.

| Function | GPIO | Notes |
| --- | --- | --- |
| RC CH1 Steering | 36 | Input only |
| RC CH2 Throttle | 39 | Input only |
| RC CH3 Park | 34 | Input only |
| RC CH4 Mode | 26 | PWM input |
| RC CH5 Drift enable | 27 | PWM input |
| RC CH6 Drift ratio | 35 | Input only |
| Steering servo | 23 | `ledc` PWM, 300 Hz / 14-bit, 1000-2000 µs mapping |
| ESC throttle | 25 | `ledc` PWM, 300 Hz / 14-bit, 1000-2000 µs mapping |
| Spare PWM_1 | 32 | Reserved output |
| Spare PWM_2 | 33 | Reserved output |
| WS2812B LED | 5 | Mode and emergency stop indication |
| UART_SEL | 12 | UART route select |
| Serial1 RX | 16 | RS232 / Pilot input |
| Serial1 TX | 17 | RS232 / Pilot output |
| I2C SDA | 21 | INA219 / MPU6050 |
| I2C SCL | 22 | INA219 / MPU6050 |

> GPIO 34, 35, 36, and 39 are ESP32 input-only pins. They cannot be used as outputs and do not provide internal pull-up or pull-down resistors.

## Control Modes

| Mode | ID | Macro | Steering source | Throttle source | LED |
| --- | --- | --- | --- | --- | --- |
| Manual | 0 | `CAR_MODE_MANUAL` | RC | RC | Green |
| Semi-auto | 1 | `CAR_MODE_SEMI_AUTO` | Pilot | RC | Yellow |
| Full-auto | 2 | `CAR_MODE_FULL_AUTO` | Pilot | Pilot | Blue |

## Serial Protocol

Input frames:

```text
Throttle:Steering\n
Throttle:Steering:Seq\n
Throttle:Steering*XX\n
MODE <m>\n
```

`XX` is a two-digit hexadecimal checksum.

Responses:

```text
ACK
NACK
ACK:Seq
NACK:Seq
```

Serial1 telemetry (uplink to host DonkeyCar `ArdImu` / `Arduino` part, v1.7.13+):

```text
T<t>S<s>\n                                # MANUAL only, ~60Hz, no colon
M<m>:P<p>\n                               # All modes, on state change + 1Hz heartbeat
$IMU,seq,ts_ms,ax,ay,az,gx,gy,gz\n        # All modes, ~100Hz, m/s² + rad/s
```

Serial1 uplink is paused only while an OTA transfer is in progress.

## Quick Start

### Python dependencies

```bash
pip install pyyaml pyserial pytest
```

Firmware builds also require:

- Arduino CLI
- ESP32 Arduino core
- Arduino libraries such as FastLED, Adafruit INA219, Adafruit MPU6050, AsyncTCP, and ESPAsyncWebServer

The repository includes a local [`libraries/`](libraries/) directory. Build scripts prefer those local Arduino libraries when present.

### Web Console through the debug AP

When the device AP is enabled, connect the computer to `MUS4-DEBUG` and open:

```text
http://192.168.4.1/
```

The firmware serves common captive-portal probe paths and may trigger an automatic browser popup. On Windows, this popup is only reliable when the MUS4 AP is the active network path. If the computer is also connected to Ethernet, VPN, or another Internet-capable network, Windows may route `msftconnecttest.com` through that other interface and open Microsoft/MSN instead. In that case, keep the wired network connected if needed, but open `http://192.168.4.1/` manually.

### Windows + WSL accelerated build

This is the preferred validation path after firmware changes:

```powershell
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

Common commands:

```powershell
# Clean the WSL build directory and compile
.\arduino-cli-wsl.ps1 -Compile -Clean -Sketch MUS4_FW.ino

# Check WSL, rsync, arduino-cli, and related dependencies
.\arduino-cli-wsl.ps1 -Check

# Compile and check firmware size / partition usage
.\arduino-cli-wsl.ps1 -Compile -CheckPartition -Sketch MUS4_FW.ino

# Compile and upload through Web Console HTTP OTA
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost 192.168.3.144 -Sketch MUS4_FW.ino

# Upload an existing build_wsl artifact through HTTP OTA
.\arduino-cli-wsl.ps1 -Upload -HttpOta -Sketch MUS4_FW.ino
```

HTTP OTA uses the Web Console `/update` endpoint. The device must be authenticated and Park-locked, unless development mode allows the operation. When `-HttpOtaHost` is omitted, the script can read the first non-empty line from `.mus4_ota_target`.

### Arduino CLI wrapper

```bash
# Compile only
python arduino-cli.py -c --sketch MUS4_FW.ino

# Upload only; port is auto-detected from config.yaml
python arduino-cli.py -u --sketch MUS4_FW.ino

# Upload to a specific serial port
python arduino-cli.py -u --sketch MUS4_FW.ino --port COM9

# Compile + upload
python arduino-cli.py -cu --sketch MUS4_FW.ino

# Compile + upload + serial monitor
python arduino-cli.py -cus --sketch MUS4_FW.ino

# Upload a prebuilt firmware image
python arduino-cli.py -u -i build/MUS4_FW.ino.bin --sketch MUS4_FW.ino

# List detected serial ports
python arduino-cli.py --list-ports

# CI-friendly line-by-line logs
python arduino-cli.py -cu --no-progress --sketch MUS4_FW.ino

# ArduinoOTA upload
python arduino-cli.py --ota -i build/MUS4_FW.ino.bin --ota-host mus4-ota
```

## Tests

```bash
# Run all Python tests
pytest tests/

# Run selected test files
pytest tests/test_arduino_cli.py
pytest tests/test_wireless_console_policy.py
pytest tests/test_firmware_feature_flags.py
pytest tests/test_train_tub_driver.py
pytest tests/test_mus4_pilot_infer.py

# Provisioning agent tests
python provisioning_system/tests/test_agent.py -v
```

`provisioning_system/playwright_tests/` contains Playwright assets, but its current `npm test` script is a placeholder and exits with failure unless a real test script is added.

## Data Collection and Pilot Tools

```bash
# Inspect Web Console Tub JSON and generate a report only
python tools/train_tub_driver.py <tub.json> --report-only --dry-run

# Train the GRU baseline; leakage-prone fields are excluded by default
python tools/train_tub_driver.py <tub.json> --out-dir <model_dir> --overwrite

# Pilot inference defaults to dry-run and does not send vehicle control
python tools/mus4_pilot_infer.py --model-dir <model_dir> --esp32-url http://<device-ip>

# Live mode requires explicit risk acknowledgement and a serial port
python tools/mus4_pilot_infer.py --model-dir <model_dir> --serial-port COM9 --mode live --i-understand-risk
```

## Repository Layout

- [`MUS4_FW.ino`](MUS4_FW.ino): main firmware sketch and runtime state machine.
- [`SharedTypes.h`](SharedTypes.h): shared data structures and enums.
- [`BuildInfo.h`](BuildInfo.h): firmware name, version, and build metadata macros.
- [`TUI.h`](TUI.h) / [`TUI.cpp`](TUI.cpp): ANSI terminal dashboard rendering.
- [`Buzzer.h`](Buzzer.h) / [`Buzzer.cpp`](Buzzer.cpp): buzzer state machine.
- [`arduino-cli.py`](arduino-cli.py): cross-platform Arduino CLI wrapper.
- [`arduino-cli-wsl.ps1`](arduino-cli-wsl.ps1): Windows/WSL accelerated build and OTA wrapper.
- [`wireless_console_policy.py`](wireless_console_policy.py): Python mirror of Wi-Fi/TCP/Web Console permission policy.
- [`tools/`](tools/): Tub JSON training and Pilot inference tools.
- [`tests/`](tests/): Python tests for build tools, policy, feature flags, training, and Pilot inference.
- [`examples/`](examples/): standalone sensor and I2C example sketches.
- [`provisioning_system/`](provisioning_system/): independent Wi-Fi provisioning and Linux agent tooling.
- [`multi_agent_framework/`](multi_agent_framework/): independent Python multi-agent framework, not part of the main ESP32 firmware path.

## Runtime Architecture

1. RC receiver PWM input is captured from CH1-CH6 and filtered.
2. USB `Serial`, `Serial1`, TCP Console, and Web Console receive commands.
3. Control logic blends RC and Pilot inputs according to the current driving mode.
4. Drift Assist can add steering compensation when its runtime conditions are met.
5. Park / emergency braking can override throttle output and update LED indication.
6. ESP32 `ledc` outputs PWM to the steering servo and ESC.
7. TUI, I2C sensors, Web charts/logs, WebSocket telemetry, and BLE Gamepad operate as side systems that read current state and publish display or peripheral data.

See [`Doc/Arch/architecture.md`](Doc/Arch/architecture.md) for more details.

## Wi-Fi Console, Web Console, and OTA

When `ENABLE_WIFI_CONSOLE` is enabled, the firmware boots in **AP-only** (`WIFI_AP`) and follows an **AP/STA mutually-exclusive switching** lifecycle (since v1.7.18):

- AP SSID: `MUS4-DEBUG` (base name; since v1.7.22 the AP SSID is no longer decorated with a STA-derived short code or IP tail)
- TCP Console: port `2323`
- Web Console / Donkey Console: port `80`
- WebSocket telemetry: port `81`
- ArduinoOTA: host `mus4-ota`, port `3232`

Lifecycle:

- On boot, if STA credentials exist the firmware temporarily switches to `WIFI_AP_STA` and calls `WiFi.begin()`. Once STA reaches `WL_CONNECTED`, the firmware waits `WIFI_STA_GRACE_UP_MS=1000ms`, then `stopWifiApForStaOnly()` shuts down the SoftAP and switches to `WIFI_STA` (STA-only); access the device at `http://<sta_ip>/`.
- When STA drops in STA-only state, the firmware arms `WIFI_STA_GRACE_DOWN_MS=1000ms`. If the link does not recover, `restoreApAfterStaLost()` switches back to `WIFI_AP` and restarts AP services (AP-only); access the device at `http://192.168.4.1/`.
- STA failures (`no_ssid` / `auth_failed` / `timeout`) all converge to `restoreApAfterStaLost()` so the device never stalls in `WIFI_AP_STA` with both interfaces broken.
- The AP stays up while STA is still trying (not yet `WL_CONNECTED`), so users are never locked out during a failed attempt.
- The firmware does not auto-retry STA in AP-only state; users must reapply credentials manually.
- STA→STA switch: saving a new SSID triggers `applyWifiStaCredentials()` which calls `WiFi.begin()` again inside `WIFI_AP_STA`. After the new STA succeeds and clears the 1s grace, the firmware moves to the new STA-only.

mDNS: the Web Console is published as `http://<ap-name-lowercase>.local/` (e.g. `http://mus4-debug.local/`). The AP name is limited to letters, digits, and hyphens, and cannot start or end with a hyphen. If `.local` does not resolve, use the STA IP shown on the page.

Wireless command permissions are layered:

- `PING`, `STATUS`, `AUTH`, and `WIFI_STA_STATUS` are available without authentication.
- Control commands, terminal options, log routing, filter debug, and Wi-Fi STA configuration require authentication.
- Diagnostics, benchmark, regression, and steering calibration commands also require Park lock.
- `ENABLE_OTA` requires authentication and Park lock. `OTA_STATUS` and `DISABLE_OTA` require authentication.

Policy changes should be mirrored in [`wireless_console_policy.py`](wireless_console_policy.py) and covered by [`tests/test_wireless_console_policy.py`](tests/test_wireless_console_policy.py).

## Web Console UI 约定（座舱 / Apple 双风格）

四个内嵌页面（`/` Drifter Console、`/judge`、`/drift`、`/update`，源码在
[`libraries/mus4_web/src/WebConsoleAssets.h`](libraries/mus4_web/src/WebConsoleAssets.h) 的 4 个 `R"rawliteral"`）
共用一套 UI 风格开关，页头分段切换器（`座舱 / Apple`）即时切换，选择写入 `localStorage['mus4.ui.style']`
（值 `cockpit` | `apple`，**默认 `apple`**，首屏内联脚本解析防闪烁）；`<html data-ui>` 为风格开关，
`<html data-theme>` 为深浅色开关，两者正交，`?ui=` / `?theme=` URL 参数优先于 localStorage。

- **座舱象限（`data-ui="cockpit"` 或无 `data-ui`）**：与 DonkeyDrifter（DD）和 Drifter Console 历史皮肤逐值对齐的
  原始外观，即 `:root`（深色）与 `html[data-theme="light"]`（浅色）两个变量块，保持原值不动——深色页面
  `#101318`、面板 `#171c24`、描边 `#2b3441`/`#344154`、强调色 FILL `#5cc8ff`（近黑字 `#061019`）、状态色
  绿 `#39d98a` / 琥珀 `#ffcc66` / 红 `#ff6b6b`；浅色页面 `#eef1f5`、面板 `#fff`、描边 `#d5dce4`/`#ccd5df`。
  **座舱象限是冻结象限：任何 UI 改动都必须以 `html[data-ui="apple"]`（或 apple 专属 media 查询）限定，
  改完须用审计 harness 复测座舱计算样式与改动前逐值一致。**
- **Apple 象限（`data-ui="apple"`，默认）**：只新增 `html[data-ui="apple"]` /
  `html[data-ui="apple"][data-theme="light"]` 两个变量覆写块 + 一组同前缀的细修规则，不碰座舱值。
  浅色 canvas `#f5f5f7` / surface `#fff` / ink `#1d1d1f` / accent `#0066cc`（fill `#0071e3`）/ 状态填充
  `#34c759` `#ff9500` `#ff3b30` `#8e8e93`；深色 canvas `#000` / surface `#1c1c1e` / `#2c2c2e` / ink `#f5f5f7` /
  accent `#2997ff`（fill `#0a84ff`）/ 状态填充 `#30d158` `#ff9f0a` `#ff453a` `#636366`。
- **2026-09-18 Apple 深化（v1.9.3）**，四页统一：

  | 维度 | 口径 |
  |---|---|
  | 字体 | 单一系统字族（`-apple-system, BlinkMacSystemFont, system-ui, …`）；`button/input/select/textarea{font-family:inherit}`（原回落 Arial）；等宽栈统一 `ui-monospace, SFMono-Regular, Menlo, Consolas, monospace` |
  | 灰阶 | 浅色结构文字 `rgba(60,60,67,.72)`（≥4.5:1）、纯 meta `rgba(60,60,67,.62)`、正文次级 `rgba(60,60,67,.85)`；深色 `rgba(235,235,245,.72/.62/.85)`。**结构文字不得用 meta 档** |
  | 语义色双轨 | 新增 `--ok-text` / `--warn-text` / `--bad-text`（浅 `#1a7f37` `#c93400` `#d70015`，深 `#30d158` `#ff9f0a` `#ff453a`）**只用于文字**；点/条/描边/渐变等填充继续用 `--ok` / `--warn` / `--bad`。主按钮填充保持 Apple 系统蓝（浅 `#0071e3` 白字 4.70:1、深 `#0a84ff` 白字 3.65:1）——这是「对齐 Apple 原生」优先于 AA 的刻意取舍 |
  | 聚焦环 | 四页统一 `:focus-visible{outline:3px solid var(--accent);outline-offset:2px}`（**不透明**，浅 5.11:1 / 深 6.96:1）；覆盖 UA 默认黑描边；不再有 `outline:none` |
  | 触控目标 | 每个可点元素命中区 ≥44×44：视觉尺寸不变，用 `::after{width:max(100%,44px);height:max(100%,44px)}` 铺不可见命中区；相邻目标只在**不冲突的轴**扩展并保证可见间距 ≥8px；表单控件实际高度提到 44px |
  | 排版 | 四页 `h1` 统一 20px/600/-.02em；微标签去 uppercase、字距归 0、≥13px；大号数字 600 + `-.02em` + `tabular-nums`；中文行高 ≥1.35 |
  | 发丝线 | 列表分隔走 Apple separator 档（浅 `rgba(60,60,67,.29)`、深 `rgba(255,255,255,.16)`）；卡片/页头描边保持弱档（浅 `rgba(0,0,0,.08)`、深 `rgba(255,255,255,.10)`）；同端 ≤2 档 |
  | 材质 | 面板/卡片有可辨表面（`--card`/`--panel` + 16px 圆角 + 1px 发丝线）；浮层（modal/toast/帮助）半透明 + `backdrop-filter: saturate(180%) blur(20px)`；toast 改底部居中毛玻璃胶囊并上移避开 `.helpFab` |
  | 圆角 | 收敛为 `8 / 12 / 16 / 22 / 9999` 五档 |
  | 动效 | 统一缓动 `--ease-apple: cubic-bezier(.32,.72,0,1)`（交互 .15s / 状态 .2s / 材质 .32s）；禁用 `transition:all`；不用 `left/top/width` 做动画（`#driftNeedle` 改 `transform`）；常驻循环动画周期 ≥2s |
  | 兜底 | `prefers-reduced-motion`（停循环动画、过渡近乎即时，状态转环保留）、`prefers-reduced-transparency`（材质退实色）、`prefers-contrast: more`（文字提到 ink1/ink2、描边加深）、`forced-colors: active`（用 `CanvasText` 画回分隔线与状态点）；`-webkit-tap-highlight-color: transparent`；`viewport-fit=cover` + 贴底元素 `env(safe-area-inset-bottom)` |
  | 移动端页头 | ≤820px 收成「56px 不换行标题行 + 44px 横向可滚弱入口行」（`overflow-x:auto` + `scrollbar-width:none` + 右侧渐隐 `mask-image`），总高 ≤100px（原 390 下 204px、`#openDshBtn` 在屏外且无滚动提示） |

  验收口径（Playwright 实测，改动前后对照见审计 harness）：浅色低于 AA 的文本节点 console 88→5、judge 37→5、
  drift 24→4；390 下 console 命中区 <44px 的 41 项 → 0；`prefers-reduced-motion` 由 0 条规则 → 四页均生效；
  座舱象限量测逐值零差异。

## Documentation

- [`CLAUDE.md`](CLAUDE.md): repository guidance for Claude Code / coding agents.
- [`CHANGELOG.md`](CHANGELOG.md): release notes.
- [`Doc/Arch/architecture.md`](Doc/Arch/architecture.md): firmware loop, state machines, and data flow.
- [`Doc/Hardware/pin_definitions.md`](Doc/Hardware/pin_definitions.md): authoritative MUS4 v2.3 / v2.4.2 pin definitions.
- [`Doc/Hardware/CONFIG.md`](Doc/Hardware/CONFIG.md): hardware configuration notes.
- [`Doc/Tools/ArduinoCLI.md`](Doc/Tools/ArduinoCLI.md): `arduino-cli.py` usage.
- [`Doc/Tools/arduino-cli-wsl_manual.md`](Doc/Tools/arduino-cli-wsl_manual.md): WSL build wrapper manual.
- [`Doc/Tools/train_tub_driver.md`](Doc/Tools/train_tub_driver.md): Tub JSON reporting and GRU baseline training.
- [`Doc/Tools/mus4_pilot_infer.md`](Doc/Tools/mus4_pilot_infer.md): Pilot inference controller and safety gates.
- [`Doc/README/OPERATIONS.md`](Doc/README/OPERATIONS.md): runtime serial commands and data frames.
- [`Doc/Inspect/wifi-ap-sta-lifecycle-inspection.md`](Doc/Inspect/wifi-ap-sta-lifecycle-inspection.md): Wi-Fi AP / STA lifecycle, captive portal, switching flow, and troubleshooting notes.
- [`Doc/Plan/`](Doc/Plan/): design plans and historical implementation notes.

## Safety Notes

This firmware directly controls a steering servo and ESC. Changes to output mapping, Park, emergency braking, mode blending, or wireless control entry points must preserve PWM limits, permission checks, and fail-safe behavior. Serial, Web Console, and TCP Console input should always be treated as untrusted input boundaries.
