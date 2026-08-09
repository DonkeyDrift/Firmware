# CHANGELOG.md

## 2026-08-09 v1.7.51

- 固件版本号从 `v1.7.50` 更新到 `v1.7.51`。
- feat(LED): 上电自检红/绿/蓝依次常亮各 1 秒（共 3 秒）；空闲（手动模式 + Park 锁定）灯色改为 Web Console 顶栏多选——红/绿/蓝可勾选多个，多色交替闪、单色亮灭闪（亮灭各 250ms）、全不选熄灭，选择 NVS 持久化、关机重启后恢复
  - `libraries/mus4_core/src/LedBlinkPreference.h` / `LedBlinkPreference.cpp`（新增）：空闲闪烁灯色偏好单一数据源——位掩码 bit0 红 / bit1 绿 / bit2 蓝，NVS 命名空间 `webui`、键 `ledblink`（UChar 0-7，缺省 7 三色全选）；API 为 `loadLedBlinkPreference()` / `saveLedBlinkPreference(mask)` / `getLedBlinkMask()`；ControlMixer 每 loop 轮询 `getLedBlinkMask()`、变化即重应用，POST 立即生效无需重启。
  - `libraries/mus4_ui/src/LedStatus.h` / `LedStatus.cpp`：新增 `runLedPowerOnSelfTest()`（红绿蓝各常亮 1 秒，阻塞式，`setup()` 中 FastLED 初始化后调用一次）与 `applyLedBlinkMask(mask)`（0 色熄灭、1 色 `setLEDToggle(color, Black)` 亮灭闪、2/3 色交替闪，间隔均为既有 250ms）；新增三参 `setLEDToggle(c1,c2,c3)` 三色循环重载，`scanLEDToggle()` 按 `toggleUse3Colors` 走双色/三色轮转。
  - `libraries/mus4_control/src/ControlMixer.cpp`：手动模式 + Park 锁定的空闲闪烁由硬编码绿/红交替改为 `applyLedBlinkMask(getLedBlinkMask())`，模式切换或掩码变化时重应用（`appliedBlinkMask` 静态缓存）。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：新增 `GET /api/led-blink`（`{"colors":0-7}`）与 `POST /api/led-blink`（缺参/非法 400 `invalid_value`，NVS 写失败 500 `{"saved":false}`，成功 `{"saved":true,"colors":x}`），错误路径与 `/api/mute`、`/api/language` 同款；处理器为薄封装，读写均委托 LedBlinkPreference。
  - `MUS4_FW.ino`：`setup()` 在 `loadMutePreference()` 后、`setupWifiConsole()` 前调用 `loadLedBlinkPreference()`；FastLED 初始化后调用 `runLedPowerOnSelfTest()`；toggle 状态新增 `toggleUse3Colors` / `toggleColor3`。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：顶栏静音按钮右、语言切换左新增 `#ledBlinkTabs` 三选项多选按钮（红/绿/蓝 Red/Green/Blue，复用 langTabs 胶囊样式，可多选），点击 XOR 取反位并 POST `/api/led-blink`，页面加载 `initLedBlink()` GET 恢复选中态；i18n 新增 `led.title`/`led.red`/`led.green`/`led.blue` 中英词条；相邻勾选按钮无缝连体成大胶囊（JS 对连体边界设 `marginLeft:-2px` 抵消容器原生 2px 间隙 + 段内边直角/段端圆角，非连体边界保留均匀细缝）；悬停高亮始终为独立小椭圆（`border-radius:999px!important` + 更高 z-index），仅在悬停按钮本身已勾选时相邻已选按钮才用伪元素延伸背景垫底填缝——悬停未选按钮不垫底，避免蓝色背景鼓包。
  - `docs/Hardware/pin_definitions.md`：WS2812B 颜色定义同步——上电自检三色各 1 秒、空闲灯色可配置（缺省三色交替、单色亮灭闪、全不选熄灭）、半自动/全自动 + Park 为模式色/红色交替闪烁。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.51。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.51；新增 `test_web_console_led_blink_color_selector`（按钮位置/位掩码/i18n/启动链/路由注册/NVS 持久化/掩码驱动/单色亮灭闪/连体 marginLeft 方案/悬停垫底限定断言）。
  - 验证：`tests/` 全量 pytest 通过（307 项）；编译通过；已 OTA 刷机（HTTP `/update` → `192.168.3.46`；首次 90s 超时致设备一度滞留 "OTA in progress"，重传成功）；Playwright 对实机页面截图复验全选连体、红蓝两丸分离、连体段内悬停填缝、悬停未选按钮无鼓包等状态全部正确。

## 2026-08-08 v1.7.50

- 固件版本号从 `v1.7.49` 更新到 `v1.7.50`。
- feat(WebConsole): 标题右侧新增 GitHub 仓库链接图标（原版本号位置）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：headerRow 在主标题 `<h1>` 后插入 `.ghLink` 链接（内嵌 20px GitHub Mark SVG，`fill="currentColor"`），新标签页打开 `https://github.com/DonkeyDrift/Firmware`（`target="_blank" rel="noopener"`，带 title/aria-label）；默认 `#8fa1b5`、hover ESP32 蓝 `#5cc8ff`，`transform:translateY(-1px)` 沿用原版本号视觉对齐。
  - `tests/test_firmware_feature_flags.py`：新增 `test_web_console_header_github_link_replaces_version_label`（href/新标签页/SVG 内联/位置在标题与 langTabs 之间/CSS 断言）。
- fix(WebConsole): DRIFT 卡片右上角 Tune 链接与状态灯互相遮挡 → 同行排列且视觉对齐
  - 场景：Tune 链接（right:8px/top:6px）与 `.stateDot`（right:12px/top:12px）挤在同一角落；中间方案曾直接删除该卡片状态灯、曾按像素硬调两者各自 `top`（9px/8.5px/8px，因不同浏览器 system-ui 字体度量差异无法收敛），均不理想。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：最终方案为新增 `.tunePair` 行内容器（`right:12px;top:11px`、11px 字号、10px 行高、nowrap）包住 Tune 链接与状态灯；状态灯脱离绝对定位改 `inline-block + vertical-align:middle`（圆心对齐文字 x 字高中心，由浏览器按当前字体度量计算，跨字体成立），无 transform 等额外位移，与其余四张卡片状态灯同为 cy=83 保持平行；Tune 链接用 `vertical-align:-1px` 下移 1px 使其油墨中心与圆点对齐（只动文字、不动圆点）；卡片末尾独立 `<span class="stateDot">` 移入该容器。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_drift_card_tune_link_left_of_state_dot` 随方案迭代重写（tunePair 结构、两段 CSS、卡片末尾无独立状态灯断言）。

## 2026-08-08 v1.7.49

- 固件版本号从 `v1.7.48` 更新到 `v1.7.49`。
- feat(WebConsole): 中英双语覆盖扩展到全部子页面（/judge、/drift、/update），修复主控制台切换语言后手柄校准读数不重渲染
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主控制台：`applyLanguage()` 末尾挂 `refreshJoystickCalStatus()`——切换语言立即重渲染"方向/油门"实时读数（此前停留在切换前语言直到下次刷新）；`#joystickCalStatus` 静态默认文本改为中文快照（默认中文界面）。
    - JUDGE 评分页：内嵌自包含 i18n 核心（与主控制台同款 `initLanguage()`——启动 GET `/api/language` 恢复设备语言，localStorage 兜底，页面不放切换 UI），新增 80 对 `judge.*` 键：维度名 `DIMENSION_KEYS` 键化、等级 `grade.*`、拖分原因 `reason.*`、碰撞提示、评分阈值表单 field/section、配置状态消息、按钮运行态等全部动态文案走 `t()`；`scoreState` 内部改存键而非中文字符串；`decodeBinaryDataPayload` 局部 `t` 改名 `ts` 消除对全局 `t()` 的遮蔽。
    - DRIFT 调参页：该页原本纯英文——en = 原文逐字快照、zh = 反向补译中文，新增 62 对 `drift.*` 键；`setDriftConfigStatus` 改键驱动（记录 `{key,kind,suffix}` 供语言切换重放），12 个调参字段 label/hint 与运行时状态值全覆盖。
    - UPDATE OTA 页：新增 10 对 `ota.*` 键（拖放提示、上传状态、错误前缀等）；`setStatus(t,c)` 形参改名 `text` 消除遮蔽；`<title>` 经 `document.title` 双语化（规避 favicon 测试的精确串断言）。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.49；JUDGE 页 `startBtn`/`getWeakestTrendReason` 断言同步为 i18n 形式；新增 `test_web_console_sub_pages_follow_device_language`（三页 i18n 核心、`/api/language` 接线、zh/en 键 parity 与前缀、无切换 UI）与 `test_web_console_language_switch_rerenders_joystick_cal_status` 两项源码断言。

## 2026-08-08 v1.7.48

- 固件版本号从 `v1.7.47` 更新到 `v1.7.48`。
- feat(mute): 静音按钮落地实际静音功能——蜂鸣器全局静音闸门 + 开机前加载偏好，静音选择关机重启后仍记住
  - `libraries/mus4_core/src/MutePreference.h` / `MutePreference.cpp`（新增）：系统级静音偏好单一数据源，API 为 `loadMutePreference()` / `saveMutePreference()` / `isSystemMuted()`；NVS 命名空间 `webui`、键 `muted`（UChar 0/1，缺省不静音）持久化，关机重启后恢复。
  - `libraries/mus4_ui/src/Buzzer.cpp`：`startMelody()` 开头按 `isSystemMuted()` 拒绝启动任何旋律——模式切换、Park 锁/解锁、Wi-Fi AP 启动/关闭、STA 连接/断开提示音全覆盖，含开机 AP 启动音；`update()` 检测到播放中被静音立即停音并复位状态机（Web Console 切静音即时生效，无需重启）。
  - `MUS4_FW.ino`：`setup()` 在 `setupWifiConsole()` 之前调用 `loadMutePreference()`（v1.7.45 在 `setupWebConsoleServer()` 里加载，晚于 Wi-Fi 初始化期间的 AP 启动音，开机音无法被静音）。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：`/api/mute` 处理器改调 MutePreference 核心 API，删除本地 `webUiMuted` 静态镜像与 `loadWebUiMutePreference()` / `saveWebUiMutePreference()`。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.48；静音持久化断言改指 `MutePreference.cpp` 并新增薄处理器断言；新增蜂鸣器静音闸门、开机加载顺序两项源码断言；发布标记断言不再钉死 build_info 当前版本——已失效的 v1.7.46 旧标记移除，v1.7.47 起改为校验 CHANGELOG 历史条目（否则每次发新版都误红）。
  - 边界说明：电调（ESC）上电/解锁提示音由电调硬件自身驱动电机绕组发声，固件无法经油门信号线消除；Ubuntu 上位机侧与浏览器侧经全仓排查无任何发声代码，静音状态由设备端持久化，各浏览器打开控制台时经 GET `/api/mute` 同步。

## 2026-08-08 v1.7.47

- 固件版本号从 `v1.7.46` 更新到 `v1.7.47`。
- feat(WebConsole): "功能说明"弹窗完全模仿 DonkeyDrifter Web UI 快捷键弹窗——新增功能分类小标题，关闭按钮统一为幽灵样式
  - `libraries/mus4_web/src/WebConsoleAssets.h`：6 条功能说明按"状态与日志 / 网络与诊断 / 数据与维护"三组分类（新增 `.helpSection` 结构），配 DonkeyDrifter 同款 uppercase 灰色小标题（`.helpSection h3`：12px / 500 / uppercase / .05em / `#8fa1b5`）；i18n 新增 `help.groupStatus` / `help.groupNetwork` / `help.groupData` 中英词条；`.helpClose` 从蓝底圆形改为幽灵按钮（28px 透明底 + `#a1a1aa` ×，hover `#27272a` 底白字），与 DonkeyDrifter 弹窗关闭按钮一致；新增 `.helpHead h2` 标题样式（16px / 700 / `#e8edf2`）；`.helpModal` 新增 `max-height:calc(100vh - 100px)` + `overflow-y:auto` 与 `color:#dbeafe`（右下角锚定、蓝边渐变面板不变）；`Serial Log` 条目随分类调整移至第一组。功能说明内容条目与既有 `help.*` 词条不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.47。
  - `tests/test_firmware_feature_flags.py`：新增 `test_web_console_help_modal_mirrors_donkeydrifter_layout`（右下角定位、渐变面板、幽灵关闭按钮、三组分类小标题及双语词条、内容条目不变断言）与 `test_firmware_version_bumped_to_v1_7_47_for_help_modal_donkeydrifter_layout`；版本号断言更新至 v1.7.47。
  - 验证：`tests/` 全量 pytest 通过；编译通过；已 OTA 刷机（HTTP `/update` → `192.168.3.46`），实机中英文两态截图确认右下角弹窗、三组分类小标题与幽灵关闭按钮渲染正确。

## 2026-08-08 v1.7.46

- 固件版本号从 `v1.7.45` 更新到 `v1.7.46`。
- feat(WebConsole): 主控制台中英文翻译全量完成，语言选择持久化到设备——首次启动默认中文（现有中英混合界面原样作为中文版），切英文后全部内容英文，关机重启仍记住选择
  - `libraries/mus4_web/src/WebConsoleAssets.h`：中文版 = 当前界面原样快照（既有中英混合文案一律不动），英文版全量翻译——新增 52 对 i18n key（zh/en 键完全对齐，各 140 键），覆盖图表/RC（`tub.recorded`、`rc.setSteeringMid/ThrottleMid`）、FAB/重连遮罩/日志（`fab.quick`、`reconnect.*`、`log.empty`）、AP 流程、扫描、上位机配网、STA 全流程、handoff/失败弹窗等动态文案，JS 内所有 textContent/alert/confirm 均改走 `t()`；顶栏 langTabs 占位控件正式接线（`data-lang` + `onclick=setLanguage`，默认中文选中态，"coming soon" title 改为 `data-i18n-title="language.title"`）；`applyLanguage` 新增 `[data-i18n-title]` 通用处理，active 同步选择器扩为 document 级 `button[data-lang]`（同时覆盖 langTabs 与 langMenu）；CSS `devHint`/`copyValue` 的 `:hover:after` content 按 `html[lang=zh/en]` 选择器双语化；新增 `initLanguage()`——启动时 GET `/api/language` 恢复设备语言偏好（失败回退 localStorage→zh，恰好一次 `applyLanguage`），`setLanguage()` 立即生效并 best-effort POST 回设备；`saveWifiAp`/`saveWifiSta`/`saveHostWifi` 错误分支局部变量 `t` 改名 `txt` 消除对全局 `t()` 的遮蔽。JUDGE/DRIFT/UPDATE 三页未动。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：新增 `GET /api/language`（返回 `{"lang":"zh"|"en"}`）与 `POST /api/language`（缺参/非法值 400 `invalid_value`，NVS 写失败 500 `{"saved":false}`，成功 `{"saved":true,"lang":"x"}`），错误路径与 `/api/mute` 同款；运行时状态 `webUiLang` 经 Preferences NVS（命名空间 `webui`、键 `lang`、String）持久化，缺省 `zh`（首次启动默认中文），`setupWebConsoleServer()` 注册路由时加载。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.46；langTabs 惰性占位断言改为接线断言（`test_web_console_language_tabs_wired_to_set_language`）；STA/handoff/recMeta/上位机配网/静音启动链等 9 处旧硬编码串断言同步为 i18n 形式；新增 `/api/language` NVS 持久化、版本号两项源码断言。

## 2026-08-07 v1.7.45

- 固件版本号从 `v1.7.44` 更新到 `v1.7.45`。
- feat(WebConsole): 顶栏语言切换左侧新增静音按钮（仅图标与状态持久化，实际静音功能后续实现）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：headerRow 在 langTabs 左侧插入 `#muteToggle` 喇叭图标按钮（内嵌 SVG 分 `.icoSound` 声波 / `.icoMute` 叉号两组，按 `.muted` 类切换显示，静音态图标变蓝）；headerRow 右推 `margin-left:auto` 由 `.langTabs` 移至 `.muteButton`，静音按钮成为右对齐组首个元素；新增 `initMute()`/`toggleMute()`/`renderMuteButton()`——页面加载时 GET `/api/mute` 恢复设备状态，点击 POST 写设备，仅成功时更新本地图标；i18n 新增 `mute.title`（静音/Mute）。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：新增 `GET /api/mute`（返回 `{"muted":0|1}`）与 `POST /api/mute`（缺参/非法值 400 `invalid_value`，NVS 写失败 500 `{"saved":false}`，成功 `{"saved":true,"muted":x}`），错误路径与 judge-config 同款；运行时状态 `webUiMuted` 经 Preferences NVS（命名空间 `webui`、键 `muted`、UChar）持久化，默认不静音，关机重启后恢复；`setupWebConsoleServer()` 注册路由时加载。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.45；langTabs 断言同步移除 `margin-left:auto`（右推改由 `.muteButton` 承担）；新增静音按钮 UI、`/api/mute` NVS 持久化、版本号三项源码断言。

## 2026-08-07 v1.7.44

- 固件版本号从 `v1.7.43` 更新到 `v1.7.44`。
- fix(WebConsole): Network 卡片无连接状态显示红色边框与红色状态点
  - 场景：STA-only 模式下 AP 关闭（`/api/status` 直报 `ap_ip=Disabled`），AP 分页显示 Disabled 但卡片仍是绿色边框绿点；HOST 分页未收到上位机上报显示 `--` 时为灰色（driftOff），都不足以表达"无连接"。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：新增 `.netDown` 样式类（边框与 `.stateDot` 均为 `#ff6b6b`，与 Park Locked 等既有红色一致）；`updateNetworkCard()` 中 HOST 分页未上报、AP 分页无有效 IP（`--`/`0.0.0.0`/`Disabled`，大小写不敏感）时改用 `netDown`，有连接时保持 `mode0` 绿色；STA 分页样式不变。
  - 顺带修复 v1.7.42 的遗漏：复制提示的有效 IP 判断抽出为 `netIpValid()`，`Disabled` 改为大小写不敏感比较——此前 AP 分页显示大写 `Disabled` 时悬停仍会浮现"点击复制 IP"。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.44；Network 卡片结构断言更新为 `netIpValid` 与 `netDown` 红框红点样式。

## 2026-08-07 v1.7.43

- 固件版本号从 `v1.7.42` 更新到 `v1.7.43`。
- feat(WebConsole): STA 配网弹窗默认开启"上位机配网"
  - 场景：绑定新 Wi-Fi 时若只给车辆配网，Linux 上位机会留在旧网络导致两车失联；用户希望默认联动。此前每次打开 STA 弹窗都强制把开关复位为关（`checked=false`），需要手动打开。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`openWifiStaModal()` 改为打开弹窗时默认勾选"上位机配网"（`checked=true`）并联动 `onHostWifiToggle()`——连接按钮默认呈现为"发送到上位机"，保存时经串口把凭据发给上位机执行 nmcli 连接；用户仍可手动关回去退化为仅车辆配网。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.43；新增 `test_web_console_sta_modal_defaults_host_provisioning_on` 源码断言。

## 2026-08-07 v1.7.42

- 固件版本号从 `v1.7.41` 更新到 `v1.7.42`。
- fix(WebConsole): Network 卡片无可复制 IP 时悬停不再显示"点击复制 IP"
  - 场景：STA 分页未配置 STA 时显示 `disabled`（设备处于 AP 模式时的常见状态）、HOST 分页未收到上位机上报时显示 `--`，此时悬停 IP 显示区仍浮现"点击复制 IP"提示与手型光标，点击只会弹"复制失败"提示。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`updateNetworkCard()` 末尾按 `networkCopyIp` 有效性切换 `networkValue` 的 `copyValue` 类（条件与 `copyNetworkIp()` 点击守卫一致：`--`/`0.0.0.0`/`disabled`/空均不可复制）；去掉该类后悬停提示与手型光标不再出现，点击守卫逻辑不变。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.42；Network 卡片结构断言新增 `copyValue` 切换逻辑。

## 2026-08-07 v1.7.41

- 固件版本号从 `v1.7.40` 更新到 `v1.7.41`。
- fix(WebConsole): Network 卡片 HOST 分页隐藏齿轮设置按钮
  - 场景：Network 卡片右上角齿轮（`openNetworkSettings()`）是 AP/STA 网络设置入口；HOST 分页（v1.7.39 上位机 IP 显示，数据来自 Serial2）没有可配置的网络项，点击会错误地弹出 STA 配置弹窗。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：齿轮按钮新增 `id="networkGear"`；`updateNetworkCard()` 按选中分页切换 `networkGear.style.display`——HOST 分页隐藏，AP/STA 分页保持显示，按钮本身与两个设置弹窗逻辑不变。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.41；`test_host_ip_report_channel` 新增 HOST 分页隐藏齿轮的源码断言。
  - 验证：`pytest tests/` 292 项全部通过；`arduino-cli.py -c` 编译通过；HTTP OTA 刷至 STA 设备 192.168.3.46，设备重启后确认运行 v1.7.41、页面已包含按分页隐藏齿轮的逻辑，`host_ip` 上报链路（10 秒周期）恢复正常。

## 2026-08-06 v1.7.40

- feat(WebConsole): 四个 Web 页面（Console/Judge/Drift/OTA）统一嵌入头盔 favicon
  - `libraries/mus4_web/src/WebConsoleFavicon.h`（新增）：200x200 头盔 logo PNG 以 PROGMEM 字节数组嵌入固件，浏览器不再请求到 404 的默认 favicon。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：新增 `/favicon.png` 与 `/favicon.ico` 路由（同一 handler 提供嵌入 PNG，`Cache-Control: max-age=86400` 长缓存）。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：四个页面 `<head>` 增加 `<link rel="icon" type="image/png" href="/favicon.png">`。
  - `tests/test_firmware_feature_flags.py`：新增 `test_web_console_pages_share_embedded_png_favicon` 源码断言（逐页校验 link 标签位置、路由注册与 PNG 魔数）。

- refactor(WebConsole): 移除 UI 风格切换，Web Console 只保留 ESP32（Drifter Console）皮肤
  - `libraries/mus4_web/src/WebConsoleAssets.h`：删除顶栏 `skinSwitch` 分段切换按钮（`Drifter Console UI` / `DonkeyDrifter Web UI`）、整段 `<style id="donkeySkin">` DonkeyDrift 皮肤 CSS、`.skinSwitch`/`.skinSeg` 样式与 `.headerRow .otaLink` 覆盖（OTA 链接恢复 `margin-left:auto` 右对齐），以及 JS 侧 `THEME_STORAGE_KEY`、`readStoredUiTheme()`/`writeStoredUiTheme()`/`applyUiSkin()`/`setUiSkin()` 与 `theme.title` 双语词条；初始化链不再调用 `applyUiSkin()`。
  - `tests/test_firmware_feature_flags.py`：删除 `test_web_console_has_skin_switch_for_ui_skin_toggle` 与 `test_web_console_ui_theme_persists_via_local_storage_and_donkey_skin_css` 两个皮肤切换断言用例；`pytest tests/` 共 292 项全部通过。
  - 使用 `arduino-cli.py -c` 编译验证通过。
  - 配套：上位机 DonkeyDrift 仓库同步移除其 Web UI 的 SkinSwitcher 与 `theme-mus4` 皮肤（各自只保留自己的 UI，删除转换键）。

## 2026-08-05 v1.7.39

- fix(WebConsole): UI 风格切换按钮名称与 DonkeyDrifter Web UI 保持一致
  - `libraries/mus4_web/src/WebConsoleAssets.h`：将 `ESP32 UI` / `Donkey UI` 同步命名为 `Drifter Console UI` / `DonkeyDrifter Web UI`，与上位机 `web_ui/frontend/src/components/SkinSwitcher.tsx` 的分段文案一致。
  - `tests/test_firmware_feature_flags.py`：同步更新 Web Console 结构断言；`pytest tests/` 共 293 项全部通过。
  - 使用 Arduino CLI 干净编译通过（Flash 78%、全局 RAM 33%），并通过 HTTP OTA 上传至 STA 设备 `192.168.3.46`；设备重启后确认运行 `v1.7.39`（构建时间 `Aug 5 2026 13:07:48`）。

## 2026-08-03 v1.7.39

- 固件版本号从 `v1.7.38` 更新到 `v1.7.39`。
- feat(Serial2): 新增上位机 IP 上报通道，Drifter Console Network 卡片可查看上位机（Linux 主机）局域网 IP
  - 场景：车上 ESP32 与 Linux 上位机经 Serial2 常连，但 ESP32 侧一直不知道上位机的局域网 IP；想 SSH / 访问上位机服务时只能去路由器或上位机本机查。v1.7.38 刚打通 ESP32→上位机的配网推送，本次补上反向信息通道。
  - 协议：Serial2 新增上行帧 `HOSTIP|<ipv4>`（Linux → ESP32），与既有 `STATUS|`/`OK|`/`FAIL|` 上位机响应帧同通道、同处理方式；上位机侧由 `donkeycar/parts/provisioning.py` 的 `ProvisioningPart` 周期上报（默认 10 秒，UDP 路由查询取默认出口 IPv4，不发实际包），首次启动立即上报，ESP32 重启后 10 秒内自动恢复显示。
  - 固件改动：`MUS4_FW.ino` 的 `handleSerial2()` 新增 `HOSTIP|` 分支，经新增 `isValidIpv4Text()` 严格校验（4 段 0-255、总长 ≤15，非法帧直接丢弃）后存入运行时全局 `hostReportedIp`/`hostReportedIpMs`（定义于 `libraries/mus4_web/src/WebConsoleServer.cpp`，仅运行时保存、不写 NVS，避免显示过期网络配置），并写 `serial2` Web 日志。
  - 状态输出：`printWirelessStatus()`（`/api/status` 与无线控制台 `STATUS` 命令共用）新增 `host_ip=<ipv4>` 与 `host_ip_age_s=<秒>` 两个字段；未上报时 `host_ip` 为空。
  - Web Console：`libraries/mus4_web/src/WebConsoleAssets.h` 主控制台 Network 卡片新增 `HOST` 分页（与 AP/STA 并列），点击显示上位机 IP、元信息行显示来源 `Serial2`，点击 IP 支持复制（复用 `copyNetworkIp()`）；未上报时显示 `--`、卡片为熄灭态。HOST 分页仅在用户点击时选中，不影响 AP/STA 自动切换逻辑。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言；新增 HOSTIP 帧处理、`host_ip` 状态字段与 HOST 分页结构断言。


## 2026-08-02 v1.7.38

- 固件版本号从 `v1.7.37` 更新到 `v1.7.38`。
- feat(WiFi): 硬件 UI（本地串口控制台）应用 STA 配置时自动打开上位机配网
  - 场景：此前触发 Linux 上位机配网（`WIFI|ssid|password` 协议，v1.7.29）的唯一入口是浏览器 Web Console STA 弹窗的"上位机配网"开关；在车辆本地硬件 UI（USB Serial / Serial1 控制台）用 `WIFI_STA_SSID:` / `WIFI_STA_PASSWORD:` / `WIFI_STA_APPLY` 配置 Wi-Fi 时不会通知上位机，上位机仍停留在旧网络。
  - 改动：`libraries/mus4_wifi/src/WifiStaConfig.cpp` 的 `WIFI_STA_APPLY` 分支在 `applyWifiStaCredentials()` 成功后，自动把当前 STA 凭据经 Serial2 推给 Linux 上位机（`Serial2.printf("WIFI|%s|%s\n", ...)`，与 Web 路径同一协议帧），上位机配网 agent 收到即自动配网并回 `STATUS|/OK|/FAIL|`（由既有 `handleSerial2()` 状态解析链路处理）；控制台新增回执 `HOST_WIFI_PROVISIONING_SENT ssid="..."` 并写 `wifi` 日志，任何响应/日志均不回显明文密码。
  - 覆盖入口：本地 USB Serial、Serial1、TCP/Web 命令控制台（均经 `processWifiStaConfigCommand()`）；Web Console STA 弹窗走 `scheduleWifiStaApply()`  deferred 应用路径、不经过该分支，其"上位机配网"开关行为不变，不会重复推送。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言；STA 配置结构断言新增自动推送符号。

## 2026-08-02 v1.7.37

- 固件版本号从 `v1.7.36` 更新到 `v1.7.37`。
- fix(WiFi): 修复 STA 连接失败后历史回退被锁死的问题（连接失败自愈）
  - 场景：NVS 中 `sta_ssid`/`sta_pass` 不一致（如 keep_password 流程只更新了 `sta_ssid`、`sta_pass` 残留旧值；或路由器改密码后配置未同步）时，开机/看门狗周期里已配置 SSID 可见，固件用错误密码反复连接反复失败；而开机扫描会把该 SSID 的历史槽位标记为"已试"，运行期重试状态机找不到未试候选——历史记录里保存的正确密码永远无法启用，车辆只能停在 AP 模式。
  - 修复：`updateWifiStaHistoryRetry()` 新增自愈分支——`lastError` 非空（WPA2 密码错误在 ESP32 上多表现为 `timeout` 而非 `auth_failed`，故不按单一错误码判定）且历史中同一 SSID 存有不同密码时，清除该槽位的已试标记，让重试状态机用历史（最近一次成功连接）密码再试一次；历史密码与当前一致时不解锁，避免同一错误密码无限重试（用历史凭据重试后运行时密码与历史一致，条件自然失效，重试次数有界）。
  - 配套新增 `healWifiStaPreferenceAfterConnect()`：连接成功边沿，若连上的网络与 NVS `sta_ssid` 相同但 `sta_pass` 与本次成功密码不一致，把验证成功的密码同步回 `sta_pass`（修复 NVS 凭据对，下次开机直连不再先失败一轮）；连上的 SSID 与 NVS 配置不同（回退到其它网络）时不触碰 NVS，沿用 v1.7.35「回退仅改运行时」的设计边界。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言；新增连接失败自愈结构断言。

## 2026-08-02 v1.7.36

- 固件版本号从 `v1.7.35` 更新到 `v1.7.36`。
- feat(WebConsole): 主控制台新增"切换 UI 风格"分段选择条——点击分段直接切换为 DonkeyDrifter Web UI 皮肤（zinc/cyan 暗色），仅换视觉风格，功能与布局位置完全不变
  - `WebConsoleAssets.h` 主页面 `</head>` 前新增第三个 `<style id="donkeySkin">`：40 余条规则全部以 `body.donkey-skin` 前缀覆写配色/字体/圆角/边框色（背景 `#09090b`、面板 `#18181b`、边框 `#27272a`、主按钮 `#0891b2`、强调 `#22d3ee`），不含任何布局属性，现有 CSS 一行未动。
  - 头部右侧新增分段选择条 `skinSwitch`（位于 OTA 按钮左侧、DEV 开关左边，分段字号/字重/胶囊圆角对齐 `.otaButton`，分段条整组含边框与 OTA 按钮同为 24px 高、底边平行对齐）：pill 容器内并排 `ESP32 UI` / `Donkey UI` 两个分段，点击直接切到对应皮肤；当前生效分段实色填充与 OTA 按钮一致（默认皮肤蓝底 `#5cc8ff` + 深色字 `#061019`，donkey 皮肤 `#0891b2` + 白字）一眼可辨；容器以 `margin-left:auto` 接管右推（`.headerRow .otaLink` 边距补偿追加在第二块 `<style>` 末尾），`donkeySkin` 块内同步追加 `body.donkey-skin` 覆写。
  - 新增 `THEME_STORAGE_KEY='mus4.ui.theme'` 与 `readStoredUiTheme()/writeStoredUiTheme()/applyUiSkin()/setUiSkin()`：localStorage 持久化（隐私模式 try/catch 降级为默认皮肤），启动时恢复上次选择并同步两个分段的 active 态，默认原风格。
  - i18n 补丁式追加 zh『切换 UI 风格』/ en 'Switch UI style'（容器 aria-label）；分段文字 'ESP32 UI'/'Donkey UI' 为专有名词不翻译。
  - 仅主控制台 `/` 生效，`/judge`、`/drift`、`/update` 保持原皮肤；遥测曲线 canvas 等 JS 绘制颜色不随皮肤切换。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言；新增 2 个结构断言（`skinSwitch` 头部位置/双分段/active 配色、`mus4.ui.theme` 持久化与 `body.donkey-skin` CSS 前缀规则）。

## 2026-07-31 v1.7.35

- 固件版本号从 `v1.7.34` 更新到 `v1.7.35`。
- feat(WiFi): STA 连接历史——自动记录最近 5 个成功连接的 WiFi，开机/断线按优先级自动连接，Web Console 列表管理
  - 新增 `libraries/mus4_wifi/src/WifiStaHistory.{h,cpp}`：NVS（命名空间 `mus4`，键 `sta_h0s/sta_h0p`…`sta_h4s/sta_h4p`）持久化最近 5 条成功连接的 STA 凭据，槽 0 为最近一次（MRU 置顶、去重更新密码、满 5 条淘汰最旧）；旧单槽 `sta_ssid/sta_pass` 在首次启动时自动迁移为槽 0；`WIFI_STA_CLEAR` / Web 清除 / BOOT 长按清配网时连带清空历史。
  - 记录时机：`updateWifiSta()` 首次拿到有效 IP 的成功分支与 `updateWifiStaHistoryRetry()` 的连接上升沿双重记录，只记"真正连上"的网络；密码随最近一次成功连接更新。
  - 开机自动连接：`setupWifiConsole()` 扫描块扩展——已配置 SSID 不可见（或 STA 未配置）而历史条目可见时，按优先级（槽 0→4）挑最佳可见条目接管本次开机连接（仅改运行时凭据，不动 NVS 配置），并用其信道预对齐 AP。
  - 断线重连：新增 `updateWifiStaHistoryRetry()` 状态机——断线落地 `restoreApAfterStaLost()` 后以 3s 节奏异步扫描，对未试过的可见历史条目按优先级逐个重试，全部试完停在 AP-only（与旧行为一致）；连上可用网络即停止，不做后台扫描切换。
  - Web API：新增 `GET /api/wifi-sta/history`（公开，只回 `rank/ssid/password_set`，不含密码明文）与 `POST /api/wifi-sta/history/delete`（需认证或 DEV 模式；只删历史记录、不动当前连接——删除正在使用的 WiFi 不断线，仅今后不再自动连接它）。
  - Web UI：STA 配置弹窗改双栏布局，右栏新增"已保存的 WiFi"列表（#优先级徽标 + SSID + 删除按钮，宽屏并列/窄屏堆叠）；删除前 confirm 确认，删除当前连接条目时 toast 提示"仅移除记录，不影响本次连接"；中英 i18n 各新增 5 个键。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言；`FIRMWARE_SOURCE_PATHS` 注册新模块；新增 8 个 WiFi 历史结构断言（NVS 键与容量常量、模块拆分、sketch 挂接点、状态机安全约束、API 安全契约、UI DOM/i18n）。

## 2026-07-12 v1.7.34

- fix(Serial1): 修复 `Serial1.setTxBufferSize(1024)` 调用顺序错误
  - 原代码在 `Serial1.begin()` 之后调用 `setTxBufferSize()`，导致 ESP32 Arduino 内核仍使用默认 256B TX 环形缓冲区，1024B 设置不生效。
  - 100Hz `$IMU` + 60Hz `T<t>S<s>` 并发写入时，256B 缓冲区溢出，造成逗号/换行符丢失，上位机出现 `$IMU` 解析失败和 `-0T2S-1`、`-0.15800.1293` 等帧污染。
  - 将 `setTxBufferSize(1024)` 与新增的 `setRxBufferSize(1024)` 均移到 `Serial1.begin()` 之前。
  - `loop()` 中 `Serial1.write()` 增加返回值检查，用于调试统计发送截断次数。
  - 同步更新 DonkeyDrifter Python 端：`Arduino_readline` 对 $IMU 帧增加字段正则校验与 T/S 污染检测，被污染帧静默丢弃且不再刷屏报错。

## 2026-06-30 v1.7.33

- 固件版本号从 `v1.7.31` 更新到 `v1.7.33`。
- feat(Serial2): 新增 Serial2 (GPIO18/19) ping-pong 双向联通协议
  - `setup()` 中初始化 Serial2（115200 8N1，GPIO19 RX / GPIO18 TX）
  - 新增独立 `handleSerial2()` 函数：PING→PONG 响应、任意文本 ECHO 回显、每秒 BEAT 心跳
  - `FirmwareConfig.h` 新增调试开关注释 `ENABLE_SERIAL2_ECHO_TO_SERIAL0`（默认关闭）
  - Serial2 独立于 `dispatchCommandLine`，PING 不会被误解析为车辆控制命令
  - 新增 `docs/guide/esp32-serial-topology.md` 串口拓扑参考文档

## 2026-06-28 v1.7.31

- 固件版本号从 `v1.7.30` 更新到 `v1.7.31`。
- feat(actuator): 舵机/电调中点运行时可配置 + 延迟生效 + Web UI Set 按钮
  - `servo_mid_v`/`motor_mid_v` 从编译期常量改为 NVS 持久化变量（默认 7372 = 1500µs）
  - 新增 `SERVO_MID`/`MOTOR_MID` 命令：无参查询当前中点，带参设置并持久化到 NVS
  - 延迟生效机制：Set 新中点后等待转轮/油门归零才同步到 PWM 映射，避免突变
  - Web UI (Drifter Console) RC Channels 面板新增 Mid S/Mid T 显示和 Set 按钮
  - WebSocket 二进制帧新增 sm/mm 字段，JSON API 同步新增
  - 新增 docs/Inspect/joystick-calibration-analysis.md（7 张 mermaid 图）
  - 同步更新 wireless_console_policy.py、test_wireless_console_policy.py、test_firmware_feature_flags.py

## 2026-06-28 v1.7.30

- 固件版本号从 `v1.7.29` 更新到 `v1.7.30`。
- feat(Judge 调参与解释增强): 围绕 `/judge` 页面继续补齐现场调参与实时解释链路，提升设备侧持久化调参、移动端可读性以及评分解释能力。
  - 新增第二批设备侧持久化评分参数：在现有 `collisionThreshold`、`bigTurnThreshold`、`windowSize` 基础上，继续持久化 `collisionPenalty` 与 6 个评分维度的敏感度权重；配置仍通过 `/api/judge-config` 读写与恢复默认，设备侧使用 NVS 保存。
  - 优化 `/judge` 调参区 UI：将参数拆分为“基础阈值”和“评分参数”两组，补充中文说明、配置摘要以及更明确的保存/恢复反馈，改善手机与现场调参时的可读性。
  - 增强实时评分构成反馈：6 个评分维度新增短窗口趋势标记 `↑ / ↓ / →`，总分区新增“当前最低项”和“最近拖分项”，帮助快速判断当前短板与近期拖分来源。
  - 增强最近拖分项原因说明：总分区新增“拖分原因”，按当前拖分维度给出两段式直接判断型说明与调参建议，例如提示下调对应敏感度或检查大弯阈值。
- 同步更新 `tests/test_firmware_feature_flags.py`：补充 Judge 第二批持久化参数、调参区 UI、实时评分构成反馈与拖分原因说明相关断言。

## 2026-06-26 v1.7.29

- 固件版本号从 `v1.7.28` 更新到 `v1.7.29`。
- fix(OTA 与 Web Console 并发): 打开 Web Console 主页面后，浏览器轮询 `/api/status`、`/api/log`、`/api/data` 及 WebSocket 重连会占用同步 WebServer 单客户端处理能力和 LWIP TCP 资源，导致后续 OTA 大文件上传中途被 reset（实测并发 60 次轮询后 OTA 在 56% 失败，`curl (56) Connection was reset`）。
  - `WebConsoleServer.cpp::setupWebConsoleServer` 增加 middleware：HTTP OTA 上传期间（`otaRuntime.inProgress == true`）对除 `/update` 外的所有请求快速返回 `503 OTA in progress`，强制浏览器立即关闭 keep-alive 连接、释放 TCP socket。
  - `WebConsoleServer.cpp::updateWebConsoleServer` 在 OTA 期间跳过 `sampleWifiWebData()`，减少主循环开销和堆分配，把 CPU 尽量留给 `handleClient()` 驱动 TCP。
  - `WebConsoleAssets.h` 中 OTA 按钮去掉 `target="_blank"`，避免主页面在后台持续轮询；`/update` 页面上传成功后自动返回 `/`。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言，并新增 OTA 期间 middleware 503 拦截与 OTA 按钮不弹窗断言。

## 2026-06-26 v1.7.28

- 固件版本号从 `v1.7.27` 更新到 `v1.7.28`。
- fix(OTA 可重复性): 第一次 OTA 成功后，后续 OTA（包括按 Reset 后）无法再次上传。
  - `MUS4_FW.ino::setup` 在 `cleanupInvalidOtaPartition()` 之后调用 `esp_ota_mark_app_valid_cancel_rollback()`，把当前启动分区标记为 VALID，取消 bootloader 的 OTA 回滚计时器。避免新固件长期处于 `PENDING_VERIFY` 状态，导致下一次 reset 被回滚到旧固件。
  - `WebConsoleServer.cpp` 新增 `resetOtaAfterFailedUpload()`：HTTP OTA 上传任何阶段失败（begin/write/end/aborted）后，统一调用 `Update.abort()` 释放 Updater 内部 buffer，并把 `os.inProgress` / `parkGuardActive` / `closeWsPending` / `windowOpen` 等状态重置干净；DEV 模式下保留 OTA 窗口并刷新 TTL，非 DEV 模式下关闭窗口。防止失败一次后 Update 对象卡住或 OTA 状态长期占用，导致后续 OTA 请求无法开始。
  - 上传开始时先防御性 `Update.abort()`，避免异常残留导致 `Update.begin()` 报 "already running"。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言，并新增 OTA 失败后状态重置与 bootloader 回滚取消断言。

## 2026-06-26 v1.7.27

- 固件版本号从 `v1.7.26` 更新到 `v1.7.27`。
- fix(OTA 稳定性): HTTP OTA 与 ArduinoOTA 上传期间，Flash erase/write 会让主循环暂时无法读取 TCP 数据，导致同步 WebServer / ArduinoOTA 的 read timeout 被触发，上传中途断连。
  - `WebConsoleServer.cpp::handleWifiWebUpdateUpload` 在 `UPLOAD_FILE_START` 时将 `wifiWebServer.client().setTimeout(30000)`，把默认 5s 的读超时延长到 30s。
  - `MUS4_FW.ino::setupWifiOtaCallbacks` 增加 `ArduinoOTA.setTimeout(30000)`，把 ArduinoOTA 默认 1s 的 read timeout 延长到 30s。
  - 给 Flash 写入和 TCP 零窗口恢复留出足够余量，避免 "http update aborted" / ArduinoOTA `OTA_RECEIVE_ERROR`。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言，并新增 HTTP OTA 与 ArduinoOTA timeout 设置断言。

## 2026-06-26 v1.7.26

- 固件版本号从 `v1.7.25` 更新到 `v1.7.26`。
- perf(WebSocket 多客户端): 曲线/日志 WebSocket 通道从「仅允许 1 个客户端、第二个强制关闭」改为支持最多 `WIFI_WEB_SOCKET_MAX_CLIENTS`（默认 2）个并发客户端。
  - `WebTelemetry.cpp` 移除单 client id 状态，改用 `wifiWebSocket.binaryAll()` / `textAll()` 广播同一份序列化好的 payload，避免每个客户端重复打包二进制帧。
  - 连接事件拒绝超过上限的新客户端，主循环通过 `cleanupClients(WIFI_WEB_SOCKET_MAX_CLIENTS)` 维持上限；OTA 期间使用 `closeAll()` 一次性清场。
  - 这样第二个浏览器标签/设备不再被踢到 HTTP `/api/data` 轮询，消除因 HTTP 数据接口反复扫描 256 点序列化 JSON 带来的主循环卡顿。
- fix(OTA 上传): 在 `handleWifiWebUpdateUpload` 的 `UPLOAD_FILE_WRITE` 分支里加入 `yield()`，让出 CPU 给 Wi-Fi/AsyncTCP/idle task，降低长时间连续写 Flash 触发 Task WDT 的概率。
- 同步更新 `tests/test_firmware_feature_flags.py`：版本号断言、WebSocket 广播路径断言、OTA 关闭断言，并新增多客户端上限断言。

## 2026-06-26 v1.7.25

- 固件版本号从 `v1.7.24` 更新到 `v1.7.25`。
- fix(OTA 稳定性): OTA 窗口打开或 HTTP OTA 上传开始时，设置 `OtaRuntimeState.closeWsPending` 标志；主循环在 `updateWifiWebSocket()` 中检测到该标志后关闭并发的 WebSocket 遥测连接，避免 WS 数据流与 OTA 传输挤占 AsyncTCP 资源导致上传中断。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言与 OTA 关闭 WebSocket 断言到 v1.7.25。

## 2026-06-26 v1.7.24

- 固件版本号从 `v1.7.23` 更新到 `v1.7.24`。
- fix(手柄校准浮窗): 修复 Web Console 中“开始校准/重试/保存”按钮在未认证时静默无响应的问题。前端现在会识别 `NACK:UNAUTHORIZED`，弹出 AP 密码输入框自动发送 `AUTH` 命令；若认证失败或 Park 未锁定，则通过 `showCommandError` 明确提示用户。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言与前端校准错误处理断言到 v1.7.24。

## 2026-06-26 v1.7.23

- 固件版本号从 `v1.7.22` 更新到 `v1.7.23`。
- feat(手柄/摇杆校准): 新增统一双轴（方向 + 油门）零位与正负最大值校准模块 `JoystickCalibration`，NVS 持久化，旧方向盘校准数据自动迁移，Drift Console 新增校准向导 UI 与 `/api/joystick-cal` 端点。
- feat(OTA 稳定性): 启动时自动检测并擦除状态为 `INVALID`/`ABORTED` 的 OTA 分区，避免上一次中断/失败的 OTA 遗留数据导致后续 OTA 稳定失败。
- refactor(Web Console 曲线区域): 将全屏/退出全屏按钮从底部工具栏移动到曲线画布右下角，并新增 `.chartCanvasWrap` 容器使其随画布缩放始终保持在该位置。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言到 v1.7.23。

## 2026-06-24 v1.7.22

- 固件版本号从 `v1.7.21` 更新到 `v1.7.22`。
- refactor(AP SSID 派生退役): AP/STA 自 v1.7.18 起已互斥切换（STA 上线后 AP 关闭），AP 与 STA 永远不会同时广播，历史在 STA 连接后给 AP SSID 追加「STA 短码 + IP 尾两段」的派生逻辑失去意义。
- **删除**：
  - `libraries/mus4_wifi/src/WifiManager.cpp` 中 `wifiStaSsidShortUpper()` / `wifiStaIpTailText()` / `buildWifiDevApSsid()` 三个 static 辅助函数；`getActiveWifiApSsid()` 简化为直接返回基础 `wifiApSsid`。
  - Web Console 中文与英文 AP 配置面板文案中「开启 DEV 模式且 STA 连接成功后，AP 名称会自动追加 STA SSID 前 3 位大写和 IP 后两段」的说明。
  - `tests/test_firmware_feature_flags.py::test_wifi_ap_ssid_prefix_is_limited_to_six_chars_with_dev_mode_suffix` 中对派生函数与文案的断言。
- `WIFI_AP_SSID_SUFFIX` (`"-ESP"`) 与 `WIFI_AP_SSID_PREFIX_MAX_LEN`（6 字符）常量保留——它们是 AP SSID 命名规则的基础，与派生无关。
- 同步更新 `tests/test_firmware_feature_flags.py` 版本号断言到 v1.7.22。

## 2026-06-24 v1.7.21

- 固件版本号从 `v1.7.20` 更新到 `v1.7.21`。
- fix(STA 实际连不上 → 全部 timeout): 实机验证 v1.7.20 后即便正确 SSID/密码也持续 timeout。串口日志显示路径走到 `[wifi] STA apply: switching to AP_STA` → `STA connecting:` → 15s 后 `STA failed: timeout`。根因：v1.7.18 把开机模式从历史的 `WIFI_AP_STA` 改为 `WIFI_AP`，`applyWifiStaCredentials` 每次都得做 `WIFI_AP → WIFI_AP_STA` 的反复切换；ESP-IDF 在这条切换路径上 STA netif 重建有 race，导致 `WiFi.begin()` 拿不到信道、握手永不开始。历史 v1.7.17 全程 `WIFI_AP_STA` 已验证 newhome_iot 等路由器可正常连接。
- **修复**：
  - `setupWifiConsole()` 开机模式改回 `WIFI_AP_STA`。互斥语义只在 `stopWifiApForStaOnly()` 落 STA-only 和 `restoreApAfterStaLost()` 回 AP-only 两个事件点切 mode；开机阶段如果 STA 未配置，STA 部分不会 begin，对外等效于 AP-only。
  - `applyWifiStaCredentials()` 在 `WiFi.mode(WIFI_AP_STA)` 之后插入 `delay(50)`，给 STA netif 留出初始化窗口；常态（已是 AP_STA）不触发 mode 切换，没有额外开销。
  - `wifiInApOnlyMode` 初始化对齐开机模式（开机 = `WIFI_AP_STA` → false）；`updateWifiConsole()` 的 retry 闸门改为「STA 未在线即可重试」，覆盖开机 AP 起不来和 AP-only 下 AP 异常两种情况。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `test_apply_wifi_sta_credentials_restores_ap_before_begin` 新增 `delay(50)` 断言。
  - `setupWifiConsole` 中 `WiFi.mode(WIFI_AP_STA)` 出现位置断言更新。
  - 版本号断言更新到 v1.7.21。

## 2026-06-24 v1.7.20

- 固件版本号从 `v1.7.19` 更新到 `v1.7.20`。
- fix(STA 失败不回 AP): 实机验证发现在 STA-only 状态下保存错误密码，15 秒后 STA timeout 但**设备并未回到 AP-only**——`updateWifiSta()` 的三个失败分支（`WL_NO_SSID_AVAIL` / `WL_CONNECT_FAILED` / connect timeout）只调用 `setWifiStaLastError()` 把 `staConnecting=false`，**没有触发任何模式切换**。结果设备卡在 `WIFI_AP_STA` 但 SoftAP 又已经在 `stopWifiApForStaOnly()` 阶段被 `softAPdisconnect(true)` 关掉，同时 ESP32 内置的 STA 自动重连还在后台用错密码反复重试。
- **修复**：三条失败路径（`no_ssid` / `auth_failed` / `timeout`）写完 `lastError` 后立即调 `restoreApAfterStaLost()` 切回 `WIFI_AP` 并 `startWifiApServices()`，与 down-grace 后的 `sta_lost` 路径收敛到同一刀。`restoreApAfterStaLost()` 内部 `esp_wifi_disconnect()` 把自动重连关掉，避免 RF 调度持续被错密码扰动。
- `restoreApAfterStaLost()` 签名由 `(bool withErrorReason)` 改回无参——错误码 / 日志文案改由 4 处调用方按场景写入（`sta_lost` / `no_ssid` / `auth_failed` / `timeout` / `WIFI_STA_CLEAR`），收敛点更清晰。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `restoreApAfterStaLost(bool)` → `restoreApAfterStaLost()` 签名断言更新。
  - 新增 `test_update_wifi_sta_failure_paths_restore_ap` 保护 4 条失败 / 清除路径都调 `restoreApAfterStaLost()`。
  - 版本号断言更新到 v1.7.20。

## 2026-06-24 v1.7.19

- 固件版本号从 `v1.7.18` 更新到 `v1.7.19`。
- fix(STA 应用时丢失 AP 兜底): 实机验证发现在 STA-only 状态下保存一个错误的 STA 密码会让设备彻底失联（AP 不广播，STA 失败也不恢复 AP）。根因：v1.7.18 的 `applyWifiStaCredentials()` 只在 `wifiInApOnlyMode=true` 时才 `WiFi.mode(WIFI_AP_STA)`，STA-only 状态下保持 `WIFI_STA`、且 SoftAP 已被 `softAPdisconnect(true)` 关停。此时 `WiFi.begin()` 失败后 `updateWifiSta()` 走的是「未连接 → 等待 connect timeout」分支，**没有任何路径恢复 AP**——`restoreApAfterStaLost()` 仅在曾经 `wifiStaConnected=true` 后断链时触发。
- **修复**：`applyWifiStaCredentials()` 改为：发起 `WiFi.begin()` 前先无条件确认 `WiFi.getMode()==WIFI_AP_STA` 且 `WiFi.softAPIP()!=0.0.0.0`；若任一不满足，立即 `WiFi.mode(WIFI_AP_STA)` + `startWifiApServices("AP restored for STA apply")`。这样 STA 连接失败时 AP 仍是兜底入口，与「STA 正在尝试连接期间 AP 保留」的设计一致。
- 串口烧写恢复路径：当 AP/STA 都不可达时，可走 USB 串口（COM20/COM21）的 `python arduino-cli.py -u -i build_wsl/MUS4_FW.ino.bin --port COMxx` 重新烧写。

## 2026-06-23 v1.7.18

- 固件版本号从 `v1.7.17` 更新到 `v1.7.18`。
- refactor(wifi 生命周期): AP+STA 长期共存稳定性差（共享 RF / 内部调度冲突，表现为 Web Console 卡顿、TCP Console 掉线、WebSocket 推送被抢占），把 `WIFI_AP_STA` 改为**AP/STA 互斥切换**：
  - **STA 进入 `WL_CONNECTED` 后等待 `WIFI_STA_GRACE_UP_MS=1000ms`**，由 `updateWifiSta()` 调用新增的 `stopWifiApForStaOnly()` 主动 `WiFi.softAPdisconnect(true)` + `WiFi.mode(WIFI_STA)`，落地为 STA-only。
  - **STA 脱离 `WL_CONNECTED` 后等待 `WIFI_STA_GRACE_DOWN_MS=1000ms`**，由 `updateWifiSta()` 调用新增的 `restoreApAfterStaLost(bool)` 切回 `WIFI_AP` 并 `startWifiApServices()`，恢复 AP-only；grace 期间链路恢复则取消重启。
  - **STA 正在尝试连接（未确认成功）期间 AP 仍保留**，避免连接失败时把用户踢出。
  - **AP 模式下不再后台轮询重连 STA**，必须用户在 AP 页面重新保存或重连。
  - **STA→STA 切换**（旧设计的 `wifiStaHandoff*` 三态共存）退役：`startWifiStaHandoff` / `finishWifiStaHandoff` / `clearWifiStaHandoff` 改为 no-op，新 SSID 由 `applyWifiStaCredentials()` 走「短暂回到 `WIFI_AP_STA` → 1s grace 后切 STA_ONLY」的统一链路；JSON `handoff_*` 字段保留以兼容前端解析，`handoff_active` 永远为 false。
  - `setupWifiConsole()` 开机直接进入 `WIFI_AP`，仅在 `wifiStaConfigured` 时由 `applyWifiStaCredentials()` 切回 `WIFI_AP_STA`。
  - `updateWifiConsole()` 不再在 STA-only 状态下周期重启整个 console（否则会把 AP 又拉起来破坏互斥）。
  - 新增 `WifiRuntimeState` 字段 `staUpGraceDeadlineMs` / `staDownGraceDeadlineMs` / `inApOnlyMode`，与之配套的 extern 别名在 `MUS4_FW.ino` 中补齐。
- Web Console STA Modal 文案微调：`AP 保持开启` → `AP 将在 1 秒后关闭，请用新 IP 继续`。
- fix(web console gating): `updateWebConsoleServer()` 的 `if (!ws.consoleStarted) return;` 闸门改为 `if (!ws.consoleStarted && !ws.staConnected) return;`——v1.7.18 互斥切换下 `wifiConsoleStarted` 的语义聚焦到「AP 服务是否就绪」，STA-only 状态下它会被 `stopWifiApForStaOnly()` 置 false，但 HTTP 必须继续在 STA 接口响应，否则浏览器通过 STA IP 访问会被立刻 TCP RST。本次先用最小改动恢复服务面，后续可考虑把 `wifiConsoleStarted` 进一步拆成 `apServicesReady` / `webServerRunning` 两个独立标志。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `test_web_console_keeps_ap_running_after_successful_wifi_sta_connection` 重写为 `test_web_console_closes_ap_after_sta_grace`，断言 `stopWifiApForStaOnly` 体内 `WiFi.softAPdisconnect(true)` + `WiFi.mode(WIFI_STA)` + `WIFI_STA_GRACE_UP_MS=1000`。
  - `test_sta_disconnect_keeps_soft_ap_clients_connected_and_services_available` 重写为 `test_sta_disconnect_restores_ap_after_grace`，断言 `restoreApAfterStaLost(bool)` 体内 `WiFi.mode(WIFI_AP)` + `startWifiApServices` + grace deadline 武装。
  - `test_soft_ap_disconnect_is_limited_to_explicit_ap_restart` 放宽：`softAPdisconnect(true)` 出现两次（restart / stopForStaOnly），允许 `WiFi.mode(WIFI_STA)` 出现。
  - `test_wifi_mdns_lifecycle_follows_sta_connection` / NetBIOS / LLMNR 改为断言停止动作在 `restoreApAfterStaLost` 体内。
  - 文案断言更新到 `AP 将在 1 秒后关闭`；版本号断言更新到 v1.7.18。

## 2026-06-21 v1.7.17

- 固件版本号从 `v1.7.16` 更新到 `v1.7.17`。
- fix(WebSocket race 收尾): 上一刀 v1.7.16 把 `mus4LogLine` 从 AsyncTCP task 撤出，但 `sendWifiWebSocketHello` 在 `WS_EVT_CONNECT` 回调里仍然在 AsyncTCP task 上写**同一个**共享 `static String wifiWebSocketPayload`，与 main loop 上的 `sendWebLogToSocket` 写同一个 String 并发 realloc —— race 没消除，bad magic 与 reboot 复现。
- **修复**：
  - **彻底删掉** `static String wifiWebSocketPayload` —— 不再有跨函数/跨上下文共享的 text 缓冲。
  - `sendWifiWebSocketHello(uint32_t clientId)` 与 `sendWebLogToSocket(...)` 都改为在函数体内声明**栈上局部 `String payload`** 并 `reserve()`，Arduino `String::operator+=` 的 realloc 只触碰本函数私有堆块。
  - `handleWifiWebSocketEvent::WS_EVT_CONNECT` 不再调 `sendWifiWebSocketHello`；改翻新增的 `volatile uint32_t pendingWsConnectClientId` 与原有 `pendingWsConnectEvent` 标志。
  - `updateWifiWebSocket()` 在 main loop 里消费 `pendingWsConnectEvent` 时**先 `sendWifiWebSocketHello(pendingWsConnectClientId)` 再 `mus4LogLine("web", "ws connected")`** —— 所有 text JSON 写入永远只在 main loop 单一上下文发生。
  - `setupWifiWebSocket` 删去无用的 `wifiWebSocketPayload.reserve(1536)`。
- 同步更新 `tests/test_firmware_feature_flags.py`：
  - `test_websocket_event_callback_does_not_invoke_log_sink_in_async_task` 新增 `handleWifiWebSocketEvent` 内**不得**出现 `sendWifiWebSocketHello(` 的负断言，以及 `updateWifiWebSocket` 体内**必须**出现 `sendWifiWebSocketHello(` 的正断言。
  - 新增 `test_websocket_text_payloads_never_share_a_static_string`：禁止 `static String wifiWebSocketPayload` 与 `wifiWebSocketPayload.reserve` 出现；强制 `sendWifiWebSocketHello` / `sendWebLogToSocket` 体内出现 `String payload`。
  - 版本号断言更新到 v1.7.17。

## 2026-06-21 v1.7.16

- 固件版本号从 `v1.7.15` 更新到 `v1.7.16`。
- fix(WebSocket race / heap 腐蚀): 实机上 v1.7.15 部署后浏览器 Web Console 报 `ws parse error: Error: bad magic` + 设备周期性 reboot（`[3632][web] ws connected` 时间戳回零）。Explore 子代理静态分析定位到 main loop 与 AsyncTCP task 之间对 `wifiWebSocketPayload` 这个 `static String` 的无锁并发写：
  - `handleWifiWebSocketEvent` 在 `WS_EVT_CONNECT` / `WS_EVT_DISCONNECT` 里直接调 `mus4LogLine("web", "ws connected/disconnected")`，sink 在 AsyncTCP task 上下文写共享 String；同时 main loop 上的 `appendWebLog`（T..S.. / M:P 等）也在 sink 这条路径写同一个 String。两个上下文并发 realloc 撕裂堆元数据 → AsyncTCP 内部 `_queueMessage` 拿到的 message buffer 内容/opcode 被踩 → 浏览器解码到前 2 字节非 `'M','4'` 的"binary"帧（bad magic）→ 不久 `std::__throw_bad_alloc` 或 LoadProhibited 触发 panic reboot。
  - 二次风险：`pushWifiWebSocketData` / `sendWebLogToSocket` 持裸 `wifiWebSocketClient` 指针并 deref，AsyncTCP task 上 `WS_EVT_DISCONNECT` 把指针置 nullptr 之间存在 TOCTOU，叠加 `cleanupClients()` 真正 free 客户端对象后是 use-after-free。
- **修复**：
  - `libraries/mus4_web/src/WebTelemetry.cpp::handleWifiWebSocketEvent` 不再调 `mus4LogLine`；新增 `volatile bool pendingWsConnectEvent` / `pendingWsDisconnectEvent` 标志，由 AsyncTCP task 翻起，main loop 在 `updateWifiWebSocket()` 里读到后再 `mus4LogLine`。sink (`sendWebLogToSocket`) 现在永远只在 main loop 单一上下文运行。
  - 全部走 id 路径：`sendWifiWebSocketHello(uint32_t clientId)`、`sendWebLogToSocket`、`pushWifiWebSocketData` 改用 `wifiWebSocket.text(id, ...)` / `wifiWebSocket.binary(id, ..., len)`，让 ESPAsyncWebServer 内部 `_ws_clients_lock` 兜底 dangling client；不再持 `wifiWebSocketClient->...` 调用。
  - `pushWifiWebSocketData` 入口的 `canSend() || queueIsFull()` 检查改为只调 `wifiWebSocket.availableForWrite(id)`（同样锁下检查 client 存活 + 队列容量），消除裸指针 deref。
- 同步更新 `tests/test_firmware_feature_flags.py`：新增 `test_websocket_event_callback_does_not_invoke_log_sink_in_async_task` 与 `test_websocket_send_paths_use_id_not_raw_client_pointer`；版本号断言更新到 v1.7.16。

## 2026-06-21 v1.7.15

- 固件版本号从 `v1.7.14` 更新到 `v1.7.15`。
- fix(稳定性): 排查实机上 v1.7.14 部署后再次出现的 `Failed to fetch`（`/api/sta`、`/api/data`、`/api/log` 三 API 同时无响应）+ `ws disconnect/connect` 循环 + 设备周期性自重启（`[3440][web] ws connected` 时间戳回零）现象，定位到两个叠加因素并修复：
  - 根因 A：v1.7.13 的 100Hz `$IMU` 帧用 `String("$IMU,") + ... + String(x, 4)` 拼装，每秒约 900 次堆 `malloc/free`，十几秒后堆碎片化与 AsyncTCP 内部 PCB tx queue 抢资源 → 某次 `malloc` 失败触发 AsyncWebSocket 异常 → ws 断连风暴 → 最终 OOM 重启。**修复**：`MUS4_FW.ino` 改为 `char imuBuf[96]` + `snprintf` + `Serial1.write` 一次写入，每帧零堆分配。
  - 根因 B：60Hz `T..S..` 通过 `appendWebLog → sendWebLogToSocket` 每秒推 60 条 JSON 到浏览器 WS，叠加 ~60Hz 曲线二进制帧后顶满 AsyncWebSocket 8 槽队列（即使 v1.7.14 已经把 `$IMU` 不入 web log）。**修复**：新增 `TELEM_WEB_LOG_INTERVAL_MS=100`（10Hz），通过 `lastTelemWebLogMs` 节流 T..S.. 写 Web 日志的频率；Serial1 上行给上位机仍是 60Hz，HTTP `/api/log?source=serial1` 的 64 条环形缓冲不受影响，前端日志窗口实际可见的 T..S.. 由约 ~每秒 60 条降到 ~10 条。
- 同步更新 `tests/test_firmware_feature_flags.py::test_serial1_uplink_matches_host_pilot_protocol` 新增 `char imuBuf[96]` / `snprintf` / `Serial1.write` 三项正向断言，`appendWebLog("serial1", imuBuf)` 负断言，与 `lastTelemWebLogMs` / `TELEM_WEB_LOG_INTERVAL_MS` 节流断言；版本号断言更新到 v1.7.15。

## 2026-06-21 v1.7.14

- 固件版本号从 `v1.7.13` 更新到 `v1.7.14`。
- fix(web ws 稳定性): 排查 v1.7.13 上车后 Web Console 持续出现的 `ws disconnected` / `ws connected` 循环和曲线卡顿：
  - 根因 A：100Hz 的 `$IMU` 行被同时镜像到 `appendWebLog("serial1", imuLine)`，经 `sendWebLogToSocket` 包成 ~90 字节 JSON 推到浏览器 WS，配合 60Hz `T..S..` 与曲线二进制帧顶爆 AsyncWebSocket 发送队列（默认 8 条），触发 `queueIsFull()` → 主动断连 → 浏览器重连 → 再次堵塞，1–3s 一轮。修复方式：`$IMU` 不再写 Web 日志，Web Console 通过 WebSocket 二进制 schema v2 的 `latest` 区获取 IMU（v1.7.11 已实装的 `gx/gy/ax/ay/az`）。`T..S..` / `M:P` 仍保留日志旁路。
  - 根因 B：mDNS / NetBIOS / LLMNR 三种主机名发现协议在弱 Wi-Fi 下的多播查询风暴 + mDNS 每 60s 周期重启会和 AsyncWebSocket 抢资源。v1.7.14 起 `FirmwareConfig.h` 新增 `DISABLE_WIFI_NAME_DISCOVERY` 总开关并默认启用：`startWifiMdnsIfNeeded()` 首行短路返回，`ENABLE_WIFI_NETBIOS_DISCOVERY` / `ENABLE_WIFI_LLMNR_DISCOVERY` 由该开关 gating（默认不再定义，对应 NetBIOS/LLMNR 包处理函数体在编译期被剪掉）。`wifiMdnsStarted` 恒为 false，因此 `WifiManager.cpp` 末尾的 60s mDNS 周期重启块自然不触发，无须额外改动。STATUS / `/api/sta` 中的 `mdns_host` / `mdns_url` / `mdns_started` 字段保留，Web UI 网络面板表现为 `mdns_started=0` 与空 `mdns_url`，便于将来注释掉 `DISABLE_WIFI_NAME_DISCOVERY` 一行恢复。
- 同步更新 `tests/test_firmware_feature_flags.py` 新增 `test_wifi_mdns_startup_short_circuits_when_name_discovery_disabled` 与对 `appendWebLog("serial1", imuLine)` 的负断言；改写 `test_wifi_discovery_compile_switches_exist` 验证 NetBIOS/LLMNR 受 `DISABLE_WIFI_NAME_DISCOVERY` gating；版本号断言更新到 v1.7.14。

## 2026-06-21 v1.7.13

- 固件版本号从 `v1.7.12` 更新到 `v1.7.13`。
- feat(Serial1 协议): 对齐上位机 DonkeyCar 真车 101 的 `ArdImu` / `Arduino` part 与 GRU drift pilot 推理链路：
  - **MANUAL 上行人工油门/转向帧**：`T<t>:S<s>\n` → `T<t>S<s>\n`（去掉历史冒号分隔符），匹配上位机 `Arduino` part 的正则解析。
  - **MANUAL 上行新增 `M<m>:P<p>\n`**：m∈{0,1,2}（MANUAL/SEMI/FULL），p∈{0,1}（UNLOCKED/LOCKED），状态变化时立即发，否则 1Hz 心跳；新增 `MODE_PARK_HEARTBEAT_MS=1000`。
  - **所有模式上行新增 `$IMU,seq,ts_ms,ax,ay,az,gx,gy,gz\n`**：MPU6050 6 轴 m/s²+rad/s（由 `Adafruit_MPU6050` 直接产出，无 ESP32 端二次换算），seq 用 `uint16_t` 自然回绕仅作丢帧检测，无校验；新增 `IMU_TELEMETRY_INTERVAL_MS=10`（~100Hz）；MPU 不在线（`mpu6050Data.valid==false`）时静默不发。
  - **OTA 闸门复用 `shouldEmitSerial1Telemetry(otaRuntime)`**：三类上行帧在 OTA 真正传输期间一并暂停，避免与 OTA 抢占 UART；OTA 结束自动恢复。下行 `<thr>:<str>[:seq][*CRC]` 解析不变。
- 同步更新 `wireless_console_policy.py` 新增 `format_serial1_manual_frame` / `format_serial1_mode_park_frame` / `format_imu_telemetry_line` 三个镜像格式化函数（桌面侧 Tub 回放、单元测试无需启动固件即可拼出与 ESP32 一致的字节流）。
- 同步更新 `tests/test_firmware_feature_flags.py::test_serial1_telemetry_has_dedicated_web_log_buffer`、新增 `test_serial1_uplink_matches_host_pilot_protocol` 与 `test_wireless_console_policy_mirrors_serial1_uplink_format`；`tests/test_wireless_console_policy.py` 新增 6 项镜像格式化测试；190 项 pytest 全绿。

## 2026-06-21 v1.7.12

- 固件版本号从 `v1.7.11` 更新到 `v1.7.12`。
- feat(web tub): 前端 `TUB_SCHEMA` 从 `mus4.web_data_point.tub.v1` 升级到 **v2**，显式宣告 Tub JSON 字段集合扩展，避免下游训练脚本误把 v1（缺 IMU 五轴）与 v2 混在同一批次。
- feat(tools train): `tools/train_tub_driver.py::PREFERRED_FEATURE_ORDER` 在 `gzf` 之后追加 `gx/gy/ax/ay/az`，GRU baseline 默认特征列含完整 IMU 五轴；`DEFAULT_EXCLUDE_COLUMNS` 保持不变（新字段是真实物理观测，非泄漏列）。
- 同步更新 `tests/test_firmware_feature_flags.py::test_tub_schema_bumps_to_v2_with_imu_five_axes` 与 `tests/test_train_tub_driver.py::test_preferred_feature_order_contains_imu_five_axes`、`test_select_feature_columns_includes_imu_five_axes_when_present` 及版本号断言。

## 2026-06-21 v1.7.11

- 固件版本号从 `v1.7.10` 更新到 `v1.7.11`。
- feat(web 遥测 WS): WebSocket 二进制遥测帧升级到 schema **v2**，在 `latest` 区 `gz` 之后追加 `gx/gy/ax/ay/az` 五个 float32。前端 `decodeBinaryDataPayload` 同步把版本校验从 `version!==1` 改为 `version!==2`，并解出新字段写入 `latest`，确保 WS 路径下 `tp(latest)` 也能让 Tub 录制拿到完整 IMU 通道。
- `wifiWebSocketBinaryPayload` 缓冲扩容 `256 → 384` B：header+latest v2 ≈100 B + 8 个点 × 24 B = 192 B 合计 ≈292 B 已突破原 256 B 上限，扩到 384 留出余量。
- 同步更新 `tests/test_firmware_feature_flags.py::test_websocket_binary_frame_schema_v2_carries_imu_five_axes` 与版本号断言。

## 2026-06-21 v1.7.10

- 固件版本号从 `v1.7.9` 更新到 `v1.7.10`。
- feat(web 遥测): `/api/data` 的 `latest` 对象新增 `gx`/`gy`/`ax`/`ay`/`az` 五个 IMU 缩写键，三位小数与现有 `gz` 精度一致；polling 路径下浏览器 `tp(latest)` 写入 `tubSamples` 后下载的 `mus4-tub.json` 立即携带漂移建模所需通道。`/api/data` 的 plot 点数组未扩，避免每帧广播放大。
- 同步更新 `tests/test_firmware_feature_flags.py::test_http_api_data_latest_exposes_imu_five_axes` 与版本号断言。

## 2026-06-21 v1.7.9

- 固件版本号从 `v1.7.8` 更新到 `v1.7.9`。
- refactor(web 遥测): `WebDataPoint` 扩展承载 IMU 五轴 (`gyroX/gyroY/accelX/accelY/accelZ`)，`sampleWifiWebData` 把 `mpu6050Data` 已采样的五轴一并写入；HTTP / WS / Tub 三条对外链路本刀暂不暴露新字段，仅做后端缓冲铺垫，对外行为零变化。
- 同步更新 `tests/test_firmware_feature_flags.py::test_web_data_point_carries_imu_five_axes_for_tub_export` 与版本号断言。

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
