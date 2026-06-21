# CHANGELOG.md

## 2026-06-14 v1.7.6-Serial

- 固件版本号从 `v1.7.6` 更新到 `v1.7.6-Serial`。
- 调整 `Serial1` 遥测与 OTA 窗口共存策略：DEV mode 保持 OTA window 打开时继续输出 `Txx:Sxx`，仅在 OTA 实际传输中暂停。
- 将项目自有头文件集中整理到 `MUS4.h`，并按配置、数据结构、I/O、控制、安全、命令、Wi-Fi 等段落分隔。
- 将固件实现收敛为 `MUS4_IO.cpp`、`MUS4_Control.cpp`、`MUS4_Command.cpp`、`MUS4_Wifi.cpp` 四个 Arduino 风格功能文件，减少过度分散的源码入口。
- 更新 README、中文 README、运行说明和用户说明书；尚未实现或需要后续硬件验证的内容仅放入 `docs/Plan/ROADMAP.md`。
## 2026-06-21 v1.7.8

- 固件版本号从 `v1.7.7` 更新到 `v1.7.8`。
- 消除 DEV 模式对 Serial1 遥测的副作用：`shouldEmitSerial1Telemetry` 仅在 OTA 真正传输期间（`os.inProgress=true`）暂停 Serial1，DEV ON 时窗口长期打开不再阻塞 ESP32 与上位机通信。Park Guard 仍由 `forceWifiOtaParkLocked()` 在传输期内托底。
- 消除 DEV 模式对 AP 广播 SSID 的影响：`getActiveWifiApSsid()` 派生只看 STA 是否已连接，与 `wifiDevModeEnabled` 解耦；STA 连接后 AP 始终广播 `<前缀>-ESP-<STA短码>-<STA IP尾段>`（如 `MU03-ESP-HUA-3.43`），无论 DEV 开关状态。`saveDevModePreference()` 不再调用 `scheduleWifiApRestart()`，切换 DEV 不再丢一次 AP/Web Console 连接。
- 同步更新 `wireless_console_policy.py::should_emit_serial1_telemetry` 与 `tests/test_wireless_console_policy.py::test_serial1_telemetry_pauses_only_during_active_transfer`、`tests/test_firmware_feature_flags.py` 中 Serial1/AP SSID 相关断言。

## 2026-06-21 v1.7.7

- 固件版本号从 `v1.7.6` 更新到 `v1.7.7`。
- 修复 `libraries/mus4_web/src/WebConsoleServer.cpp` 中 `String::toUpperCase()` 在表达式拼接处的编译错误（ESP32 Arduino core 3.x 起返回 `void`），同步修正 `tests/test_firmware_feature_flags.py:497` 的源码断言。
- 收敛 DEV 模式安全边界：`isWirelessCommandAllowed` 重排序，DEV ON 仅显式放权 OTA + Web 配置 + 显示/日志切换 + WIFI_STA_*；控制命令与诊断命令（`Throttle:Steering`、`TEST`、`BENCH`、`REGRESS`、`STEER_CAL*`）严格要求认证；同步修正 `wireless_console_policy.py` 与 `tests/test_wireless_console_policy.py::test_web_dev_mode_does_not_bypass_authentication_for_control_or_diagnostic`。
- `processWirelessConsoleLine` 的 NACK 分流同步收敛：未认证用户（即使 DEV ON）一律返回 `NACK:UNAUTHORIZED`，不再返回 `NACK:PARK_REQUIRED`。
- 新增 `docs/Plan/DEV模式影响面与运行逻辑映射.md`，记录 DEV 开关在 v1.7.7 实现下的事实映射、放权清单、执行链路与历史偏差收敛过程。

## 2026-06-12 v1.7.6

- 固件版本号从 `v1.7.4` 更新到 `v1.7.6`。
- Web Console 屏保激活延时调整为 60 秒。
- Web Console 串口界面发送按钮与日志暂停按钮交换位置。
- Web Console 绘图区暂停、清空、全屏图标按钮上移至图例行左侧，与 Throttle / Steering / GyroZ 同处一行。

## 2026-06-11 v1.7.4

- 固件版本号从 `v1.7.3` 更新到 `v1.7.4`。
- 完成安全关键模块拆分：将 RC PWM 输入捕获迁入 `RcPwmCapture.h/.cpp`。
- 完成控制融合模块拆分：将驾驶模式切换、RC/Pilot 混控、Drift Assist 迁入 `ControlMixer.h/.cpp`。
- 完成安全状态机拆分：将 Park 状态机、紧急制动 FSM 迁入 `SafetyState.h/.cpp`。
- 完成执行器输出拆分：将 PWM 映射、限幅、`ledcWriteChannel` 迁入 `ActuatorOutput.h/.cpp`。
- `MUS4_FW.ino` 从 ~3700 行收敛到 ~556 行，缩减 85%。
- 清理死代码：`rise_time[]`、`lastParkState`、`adj()`、`MOTOR_OFFSET_V`/`SERVO_OFFSET_V`、波形数组、`counter`。
- 同步更新 `tests/test_firmware_feature_flags.py` 源码断言（75 项）与 `AGENTS.md` 模块清单。
- 更新 `Doc/Plan/MUS4_FW模块化拆分方案.md` 至 3.0 修订稿，标记全部计划内切片已完成。
- **修复 HTTP OTA 上传可靠性**：
  - `WebConsoleServer.cpp`：新增 query parameter `?auth=` 一次性认证，摆脱全局 session 依赖；将 OTA 错误消息从单一 `NACK:UPDATE_FAILED` 细化为 `NACK:AUTH_REQUIRED`/`PARK_REQUIRED`/`BEGIN_FAILED`/`WRITE_FAILED`/`END_FAILED`/`ABORTED`，便于诊断根因。
  - `arduino-cli-wsl.ps1`：上传前自动预检（`AUTH` + `ENABLE_OTA`）；curl 增加 `--connect-timeout 10`、`--max-time 180`、`--retry 2`、`--retry-delay 3`、`--retry-connrefused`，解决大文件在慢 Wi-Fi 下因 ESP32 5 秒超时断开导致的上传失败。

## 2026-06-10 v1.7.3

- 固件版本号从 `v1.7.2` 更新到 `v1.7.3`。
- 将 Web Console 的 Serial Log 显示区域限制为最多 16 行。
- 将 Web Console 屏保激活延时调整为 60 秒。

## 2026-06-10 v1.7.2

- 固件版本号从 `v1.6.3` 更新到 `v1.7.2`。
- 将 Web Console 品牌文案从 `DonkeyDrift Console` 调整为 `Drifter Console`。
- 将语言与帮助入口折叠为右下角单个发光圆点，点击后径向展开，减少默认遮挡数据区域。
- 保留语言选择持久化与中英文核心界面文案切换。

## 2026-06-07 v1.6.0

- 固件版本号从 `v1.5.23` 更新到 `v1.6.0`。
- 新增 Web Console AP tab 下的 AP SSID 配置弹窗，保存后持久化到 NVS。
- 保存 AP SSID 后自动重启 SoftAP，使新 SSID 无需整机重启即可生效。
- Network 齿轮按钮按当前 AP/STA tab 分流，STA tab 继续打开原 STA Wi-Fi 配置。

## 2026-06-05 v1.5.23

- 固件次版本号从 `v1.5.22` 更新到 `v1.5.23`。
- 将 Web Console 的 `RC Channels` 初始状态改为折叠，减少顶部状态区默认占用高度。
- 优化 `STATUS Details` 展开布局，宽屏显示 3 列，中等宽度 2 列，窄屏 1 列。

## 2026-06-05 v1.5.22

- 固件次版本号从 `v1.5.21` 更新到 `v1.5.22`。
- 为 Web Console 的 RC 通道与 STATUS 详情新增 `▸` / `▾` 折叠展示，STATUS 折叠时只保留标题。
- 将 Web Console 的 STATUS 文本展开视图改为 key/value 列表，并保留 `build` 等带空格引号值的完整内容。

## 2026-06-05 v1.5.21

- 固件次版本号从 `v1.5.20` 更新到 `v1.5.21`。
- 将 Web Console 顶部开发模式开关文案从 `DEBUG MODE` 调整为 `DEV MODE`，并同步认证失败提示。
- 收窄 Web Console 的 `MODE` 与 `PARK` 状态卡片到 `flex:0.30`，同时保留原字体大小与内边距。
- 调整 Web Console 标题行底边对齐，使版本号文字与 `MUS4 Web Console` 标题底边对齐。

## 2026-06-01 v1.5.20

- 固件次版本号从 `v1.5.19` 更新到 `v1.5.20`。
- 修复 STA 断开或重连时运行时断开操作扰动 SoftAP，导致 AP 需要多次重试才能连接的问题。
- 为 Windows 连通性探测提供本地 DNS 捕获和 `/connecttest.txt`、`/ncsi.txt` 响应，降低系统因“无 Internet”自动断开 MUS4-DEBUG AP 的概率。

## 2026-06-01 v1.5.19

- 固件次版本号从 `v1.5.18` 更新到 `v1.5.19`。
- Web Console 保存 STA Wi-Fi 后会等待连接结果；连接失败时在页面内悬浮窗显示原因和处理建议。
- 扩展 STA 状态输出，新增连接中状态、失败原因码和失败原因说明，便于 Web Console 与命令行排障。

## 2026-06-01 v1.5.18

- 固件次版本号从 `v1.5.17` 更新到 `v1.5.18`。
- 调整 DEBUG MODE 权限策略：Web Console 操作免 AUTH，但仍保留 Park Locked 安全限制，并在非 Park 或未授权时给出明确弹窗引导。
- 同步更新无线权限策略镜像测试，覆盖 STA 修改、OTA 和控制命令的开发模式免认证行为。

## 2026-06-01 v1.5.17

- 固件次版本号从 `v1.5.16` 更新到 `v1.5.17`。
- 将 Web Console 右上角开发模式开关标签从 `Auto OTA` 改为 `DEBUG MODE`，使其更准确表达开关含义。

## 2026-06-01 v1.5.16

- 固件次版本号从 `v1.5.15` 更新到 `v1.5.16`。
- 修复 Web Console 保存 STA Wi-Fi 配置时立即重连可能中断当前 HTTP 请求，导致浏览器提示 `Failed to fetch` 的问题。

## 2026-06-01 v1.5.15

- 固件次版本号从 `v1.5.14` 更新到 `v1.5.15`。
- 修复 Web Console 的 STA Wi-Fi 配置弹窗在用户输入 SSID 后离焦时，周期刷新可能把输入值覆盖为当前保存值或编译默认值的问题。

## 2026-05-30 v1.5.14

- 固件次版本号从 `v1.5.13` 更新到 `v1.5.14`。
- 新增 Web Console Tub JSON 连续记录与浏览器下载功能，便于采集 CH1-CH6 等遥测样本交给模型分析。
- 压缩 Tub 控件文案，降低 Web Console HTML 体积，规避 HTTP OTA 接近分区末尾写入失败。

## 2026-05-30 v1.5.13

- 固件次版本号从 `v1.5.12` 更新到 `v1.5.13`。
- 保留当前已验证的 WebSocket 曲线实时显示能力，便于通过 HTTP OTA 发布当前稳定固件。
