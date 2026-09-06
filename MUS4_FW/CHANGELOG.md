# CHANGELOG.md

## 2026-09-06 v1.8.70

- fix(WebConsole): ZCode 远控链接宽容解析——兼容 fragment 形式参数与 remoteControlToken 链接，无效存档不再回填 prompt 诱导回车
  - 背景：用户反馈粘贴 ZCode 桌面端「复制链接」给出的链接被误判「链接无效」——桌面端链接的参数可能在 `#` fragment 之后（形如 `https://zcode.z.ai/remote/v4#sid=…&hash=…`），而 v1.8.69 的校验只认 query 参数；另一诱因是早期存入的无效裸链接被原样回填进 prompt 预填，诱导用户直接回车再次校验失败。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 CONSOLE 切片）：
    - JS 新增 `zcodeRemoteNormalize(raw)` 统一归一化：trim + 去首尾引号（含「」“”‘’）→ `new URL()` 解析（失败返回空）→ 必须 `https:` → fragment 里有 `=` 就把参数归并进 query（query 已有同名参数不覆盖）并清空 hash → 有 `remoteControlToken` 参数原样返回（token 链接不刷 t）→ 否则必须含 `sid`+`hash` 且把 `t` 刷成 `Date.now()`。
    - `zcodeRemoteFreshUrl()` 与 `zcodeRemotePrompt()` 均改走 `zcodeRemoteNormalize`：prompt 预填改为存档归一化有效才预填、无效存档预填空串（不再诱导回车）；保存的是归一化后的值（fragment 形式链接存档后即为 query 形式 + 新鲜 t）。
    - 安全不变：真实远程链接是凭证，只存浏览器 localStorage；代码与测试内仅出现占位示例，不含真实链接。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.69 → v1.8.70。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本断言 v1.8.70、CHANGELOG 顺序链补 v1.8.70；`test_web_console_header_entry_buttons` ZCode 断言块更新：删除已失效的 `url.indexOf('https://')===0` 断言（新代码用 `u.protocol!=='https:'`），新增 `zcodeRemoteNormalize`/`u.hash.slice(1)`/`remoteControlToken`/`window.prompt(t('zcode.remotePrompt'),cur)` 断言，sid/hash/t 刷新相关断言保留，注释块同步描述新行为。node 打桩功能验证 32 例通过（query/fragment/token/垃圾输入/引号容忍/无效存档预填空串等），pytest 与固件编译验证通过。

## 2026-09-06 v1.8.69

- feat(WebConsole): DC「ZCode」按钮点击即新鲜、正常点击零弹框——单击用已存凭证现拼带全新时间戳的远控链接，复制到剪贴板后直接新标签打开，并后台唤醒 PC 上的 Z Code 桌面端
  - 背景：v1.8.68 把 ZCode 按钮改为打开 localStorage 里保存的远控链接，但链接里的 `t` 生成时间戳会过期（z.ai 远控页明示"不要复用旧复制的链接，请扫最新二维码"），旧链接打开即显示「手机连接已失效」；无存档时还会 prompt 弹框。本次让每次点击都现拼一条带全新 `t` 的链接（sid/hash 为持久化设备凭证不变），只有点击后才向 Z Code 发请求，且复制到剪贴板再打开，保证每次打开都不失效、不再弹框。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 CONSOLE 切片）：
    - JS 新增 `zcodeRemoteFreshUrl()`：读 localStorage 键 `zcodeRemoteUrl`，`new URL()` 解析后校验含 `sid` 与 `hash` 参数（缺一视为无存档，返回空走录入），`searchParams.set('t', String(Date.now()))` 现拼新鲜链接返回。
    - JS 新增 `zcodeRemoteCopy(url)`：复制到剪贴板——`navigator.clipboard.writeText` 优先，http 非安全上下文降级隐藏 textarea + `document.execCommand('copy')`；成功 `showToast`（`zcode.remoteCopied`），失败仅记日志不阻塞跳转。
    - JS 新增 `zcodeRemoteWake()`：点击时 best-effort `POST http://<host_ip>:8090/api/launch/zcode-remote` 唤醒/拉起 PC 上的 Z Code 桌面端（`_launcherIp` 为空跳过、失败静默、不 await 不阻塞跳转）。
    - `openZCode()` 改造：单击（260ms 双击去抖不变）→ `zcodeRemoteFreshUrl() || zcodeRemotePrompt()` → 有值则复制 + `window.open(url,'_blank','noopener')` + `zcodeRemoteWake()`；正常点击零弹框。
    - `zcodeRemotePrompt()`：预填值改为当前存档（不再预填必失效的裸占位链接）；校验升级为必须 `https://` 且含 `sid=`、`hash=` 参数，失败 alert 不保存不打开——杜绝再存进必失效的裸链接。
    - i18n：`zcode.remoteHint`/`zcode.remotePrompt`/`zcode.remoteInvalid` 中英词条更新（引导粘贴桌面端「复制链接」给出的完整链接），新增 `zcode.remoteCopied`（远控链接已复制到剪贴板 / Remote control link copied to clipboard）；按钮静态 title 同步更新。
    - 安全不变：真实远程链接是凭证，只存浏览器 localStorage；代码与测试内仅出现占位示例（`https://zcode.z.ai/remote/v4?sid=…&hash=…`），不含真实链接。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.68 → v1.8.69。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本断言 v1.8.69、CHANGELOG 顺序链补 v1.8.69；`test_web_console_header_entry_buttons` ZCode 断言块改写为 v1.8.69 新行为（`zcodeRemoteFreshUrl`/sid/hash 校验/`set('t',String(Date.now()))`/`zcodeRemoteCopy`/clipboard+execCommand/`zcodeRemoteWake`/`:8090/api/launch/zcode-remote`/新中英词条；旧端点残留断言改为带收尾引号的 `:8090/api/launch/zcode'` 以区分新 `-remote` 端点）。pytest 与固件编译验证通过。

## 2026-09-06 v1.8.68

- feat(WebConsole): DC 顶栏「ZCode」按钮行为原地替换为远程控制链接跳转——单击打开 localStorage 链接、双击重录；v1.8.67 并列新增的 #openZCodeRemoteBtn 撤下（有意行为变更）
  - 背景：用户明确不要两个 ZCode 按钮并存——撤下 v1.8.67 新增的「ZCode Remote」按钮，把旧的 launcher 版「ZCode」按钮（#openZCodeBtn，原 POST :8090/api/launch/zcode 拉起 TUI 网页终端）原地替换为远程链接入口，id 与标签「ZCode」均不变。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 CONSOLE 切片）：
    - headerRow：`#openZCodeBtn` 保留原位（Kimi Code Web 与 DeepSeek Harness 之间），`onclick="openZCode()"` 指向新实现并新增 `ondblclick="editZCodeUrl()"`，title 提示走 `data-i18n-title="zcode.remoteHint"`（单击打开 ZCode 远程控制，双击更新链接 / Click to open ZCode Remote Control, double-click to update the link）；v1.8.67 的 `#openZCodeRemoteBtn` DOM 整段移除（含 lucide link 图标）。
    - JS：旧 launcher 版 `openZCode()`（about:blank 句柄 + AbortController 15s 超时 + toast 报错）整体删除，替换为远程链接实现——单击读 localStorage 键 `zcodeRemoteUrl`，有值 `window.open(url,'_blank','noopener')`；无值 `window.prompt` 录入（预填现有值或占位示例 `https://zcode.z.ai/remote/v4`），trim 后校验 `https://` 前缀，失败 alert 且不保存不打开；双击重新 prompt 更新；单击经 260ms 定时器延迟以区分双击。
    - i18n：移除 `button.openZCodeLaunching` / `toast.zCodeFailed` / `toast.zCodeTimeout`（中英，随 launcher 行为下线）与 v1.8.67 的 `button.openZCodeRemote`；保留 `zcode.remoteHint` / `zcode.remotePrompt` / `zcode.remoteInvalid`（中英各 3 条）；`button.openZCode`（ZCode）标签不变。
    - 窄屏布局：`#openZCodeRemoteBtn{order:11}` 规则移除，`.br2` 复原 12→11，第 2 行恢复 5 个入口按钮。
    - 安全不变：真实远程链接是凭证，只存浏览器 localStorage；代码与测试内仅出现占位示例，不含真实链接。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.67 → v1.8.68。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本断言 v1.8.68、CHANGELOG 顺序链补 v1.8.68；`test_web_console_mobile_header_layout` 移除 #openZCodeRemoteBtn order 断言、复原 `.br2{order:11}` 并加"不存在"断言；`test_web_console_header_entry_buttons` 原 v1.8.42 launcher 断言块改写为新行为断言（ondblclick、data-i18n-title、localStorage 读写、noopener、https:// 校验、中英词条，及 launcher/并列按钮残留不存在），v1.8.67 ZCode Remote 断言块改写为 v1.8.68 撤下防回潮断言，`.navTabWeak` 计数恢复 3，DOM 序链复原。node 打桩功能验证与固件编译验证通过。

## 2026-09-06 v1.8.67

- feat(WebConsole): DC 顶栏新增「ZCode Remote」入口按钮——单击新标签页打开 localStorage 里的 ZCode 远程控制链接，双击重新录入
  - 背景：ZCode 桌面端可生成远程控制链接（形如 https://zcode.z.ai/remote/v4#配对凭证，链接本身是凭证）；用户希望在 DC 头部一键打开。与既有「ZCode」按钮（#openZCodeBtn，走 launcher :8090/api/launch/zcode 拉起 TUI 网页终端）是两个功能，本版为并列新增、旧按钮不动。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 CONSOLE 切片；JUDGE 无 headerRow、DRIFT 的 headerRow 仅标题，不属同一结构，未动；body.embedded 下 headerRow 本就隐藏）：
    - headerRow：`#openDshBtn` 之后、GitHub 链接之前新增 `#openZCodeRemoteBtn`（.navTabWeak 弱化标签 + lucide link 14px 图标，沿用 KCW/ZCode/DSH 同款结构）；`onclick="openZCodeRemote()"`、`ondblclick="editZCodeRemoteUrl()"`，title 提示走 `data-i18n-title="zcode.remoteHint"`。
    - 交互（原生 JS）：单击读 localStorage 键 `zcodeRemoteUrl`，有值直接 `window.open(url,'_blank','noopener')`；无值 `window.prompt` 录入，校验须以 `https://` 开头（失败 alert 提示且不保存不打开），合法则 trim 后存入并打开；双击重新 prompt 更新（预填现有值，无值时预填占位示例 `https://zcode.z.ai/remote/v4`）。单击经 260ms 定时器延迟以区分双击（否则 dblclick 前的两次 click 会误开两个标签页）。
    - 窄屏布局（max-width:820px）：第 2 行末尾追加 `#openZCodeRemoteBtn{order:11}`，`.br2` 换行分隔顺移 11→12，其余 order 不变。
    - i18n 中英各 4 词条：`button.openZCodeRemote`（ZCode 远程 / ZCode Remote）、`zcode.remoteHint`（单击打开 ZCode 远程控制，双击更新链接 / Click to open ZCode Remote Control, double-click to update the link）、`zcode.remotePrompt`、`zcode.remoteInvalid`。
    - 安全：真实远程链接是凭证，只存用户浏览器 localStorage；代码与测试内仅出现占位示例，不含真实链接。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.66 → v1.8.67。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本断言 v1.8.67、CHANGELOG 顺序链补 v1.8.67（并补 v1.8.66→v1.8.65 缺失链节）；`test_web_console_mobile_header_layout` 第 2 行补 `#openZCodeRemoteBtn{order:11}` 与 `.br2{order:12}` 断言；`test_web_console_header_entry_buttons` 新增 ZCode Remote 断言块（DOM 序链 dsh<zcodeRemote<gh、onclick/ondblclick、data-i18n-title、localStorage 读写、noopener、https:// 校验、占位示例、中英词条、lucide link 图标），`.navTabWeak` 计数 3→4。另用 node 对新增 JS 函数做打桩功能验证 11/11 通过；固件编译验证通过。

## 2026-09-03 v1.8.66

- fix(security): 隐私泄露清理——真实 Wi-Fi 凭据与本机 agent 私人文件移出版本控制，防止继续随公开仓库扩散
  - 背景：安全审计发现本仓库（GitHub 公开）三分支当前树仍带着真实 STA Wi-Fi 凭据与多个本机 agent 工作目录。本次全部解除跟踪；历史提交中的旧内容无法经删文件抹除，根治依赖更换路由器密码（已另行提醒用户）。
  - `MUS4_FW/WirelessSecrets.h`：`git rm --cached` 解除跟踪（`MUS4_FW/.gitignore` 本已列出该文件、此前被跟踪导致 ignore 无效）；模板 `libraries/mus4_core/src/WirelessSecrets.example.h` 本就在库（占位符内容），`MUS4_FW.ino` 与 `libraries/mus4_wifi/src/WifiStaConfig.cpp` 经 `__has_include` 条件包含，缺该文件可正常编译。本地构建/OTA 前复制模板填入真实凭据即可。
  - 同批解除跟踪的本机私人文件：`MUS4_FW/ArduFlux.json`（IDE 配置）、`MUS4_FW/.trae/`、`MUS4_FW/.superpowers/`、`MUS4_FW/.claude/`（agent 工作目录，含内网 IP 与本机路径）、`MUS4_FW/provisioning_system/playwright_tests/playwright-report/`（测试产物）——均只解除跟踪、本地文件保留。
  - `MUS4_FW/provisioning_system/tests/test_agent.py`：单测中硬编码的真实家庭 Wi-Fi SSID/密码改为与真实凭据无关的占位值 `TestSSID`/`testpass123`（断言逻辑不变）。
  - `.gitignore` 加固：根级新增 `.env`/`.env.*`/`*.pem`/`*.key`/`id_rsa*`/`known_hosts`/`*.ovpn`/`*.p12`/`*.keystore`/`credentials*`/`secrets*`、`.trae/`/`.claude/`/`.superpowers/`/`.agents/`、`sdkconfig`；`MUS4_FW/.gitignore` 新增 `.trae/`/`.claude/`/`.env`/`sdkconfig`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.65 → v1.8.66。
  - 测试同步：`MUS4_FW/tests/test_firmware_feature_flags.py` 版本断言 v1.8.66、CHANGELOG 顺序链补 v1.8.66；`test_agent.py` 全量通过；固件编译验证通过。固件行为无功能变化（仅版本号字符串）。

## 2026-08-23 v1.8.65

- style(WebConsole): CC 内嵌设置视图 UI 对齐 DonkeyDrifter 原生风格（Issue #234 后续）
  - 背景：用户要求 CC 设置视图里的小标题（RC Channels / 转向修正 / 油门策略 / 评分阈值调参 / 评分参数 / 评分维度）与卡片观感向 DD 的 Drive / Tub Manager / Trainer 页看齐。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（三切片 CONSOLE/DRIFT/JUDGE 各注入 `#dd-embed-native` 样式块，全部 `body.embedded` 作用域，独立页不动）：
    - 小标题 DD 化：6 个板块标题 `font-weight:600` + `letter-spacing:-0.02em` + 15px，深色 `#e4e7eb` / 浅色 `#1a2330`；标题前加 18px 线性图标（lucide sliders-horizontal，`mask`+`currentColor` 跟随标题色），既有悬停灰字副标题（titleHint/hintSpan）保留——与 DD `SectionCardTitle` 同构。
    - 卡片 DD 化：`.panel`/`.rcCell`/`.summaryItem`/`.field`/`.card` 圆角统一 8px（DD `rounded-lg`）；浅色主题下单元格/字段卡白底 `#fff` + 细框 `#ccd5df`（对齐 DD `theme-light` 卡片），输入框同步；深色沿用既有近似 DD 深色面板。
    - 子 iframe 主题透传：`initEmbedTuneFrames()` 给 `/drift?embedded=1`、`/judge?embedded=1` 的 src 追加 `&theme=<父页 data-theme>`，使漂移/Judge 子页跟随主视图（DD 经 `?theme=` 传入）的显式主题，避免系统主题与 DD 手动主题不一致时子页错色。
    - CSS 嵌套修复：首轮 OTA 后 playwright 验证发现 `<style id="dd-embed-native">` 被前一个 `<style>` 块吞没（HTML 原始文本模式不认嵌套 style 标签，整块 CSS 被当作文本），base 规则（font-weight:600 等）丢失；修复为在 `<style id="dd-embed-native">` 前插 `</style>` 关闭前块，并把尾部多余的 `</style></style>` 改为单个 `</style>`（三切片同款）。
    - `::before` 图标选择器修复：同轮验证发现 `::before` 伪元素只加在选择器列表最后一项，前 5 个标题选择器没带 `::before` 导致 mask 图标不渲染；修复为每个选择器都加 `::before`（三切片同款）。
    - 初始化顺序修复：`initEmbedTuneFrames()` 原在 `initTheme()` 之前调用，读取 `document.documentElement.getAttribute('data-theme')` 时主题尚未应用（恒取系统默认 light），子 iframe 永远加载 `theme=light`；修复为把 `initEmbedTuneFrames()` 移到 `initTheme()` 之后（主页 console 切片，drift/judge 子页本就有 `readUrlTheme` 不受影响）。
    - 控制台切片 `readUrlTheme()`：新增函数读 `?theme=light|dark` URL 参数（drift/judge 切片本就有，console 切片此前缺失），`initTheme()` 改为 `uiTheme=readUrlTheme()||'auto'`——使 DD 经 iframe URL 传入的显式主题在控制台主页也生效。
  - DD 侧 `CarSettingsPanel.tsx`：iframe src 由 `?embedded=1&settings=1` 改为 `&lang=<lang>&theme=<light|dark>`（lang 取 useTranslation、theme 取 useResolvedTheme），theme 变化经 `key` 触发 iframe 重载；注释与测试同步。
  - 测试：`tests/test_firmware_feature_flags.py` 新增 `#dd-embed-native` 注入断言（三切片各一）、`initEmbedTuneFrames` 主题透传断言、版本断言 v1.8.65、CHANGELOG 顺序链补 v1.8.65；CSS 断言带 `!important`、console 切片 `initTheme` 断言改 `readUrlTheme()||'auto'`。
  - 车上验证：playwright（headless Chromium）24/24 全过——两主题下主页面 font-weight=600、`::before` mask 图标渲染（SVG data URI）、深色 color=#e4e7eb / 浅色 #1a2330、`.panel` border-radius=8px、`html[data-theme]` 跟随 `?theme=`；drift/judge 子 iframe 主题跟随父页（dark→dark / light→light）、子 iframe 标题 font-weight=600 + mask 图标同在。

## 2026-08-23 v1.8.64

- feat(WebConsole): DD Car Connector 内嵌设置视图布局调整——RC Channels 置顶为第一板块、收紧漂移/Judge 子 iframe 间的过大间隙（配合 DD 侧 iframe 去掉 `&wifi=1`，配网板块不再进入该视图）
  - 布局顺序：`?embedded=1&settings=1` 最终可见顺序由「漂移 → Judge → RC Channels」改为「RC Channels → 漂移 → Judge」；settings+wifi 视图（DD 已不用）变为 AP、STA、RC、设置，不苛求。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主页尾部 init 同步段（wifi 分支之后、rcFold 压平块之前）新增：`settings=1` 时把整个 `#diagnosticsPanel`（settings 作用域只露出 `#rcFold`）`insertBefore` 到 `#settingsView` 之前，带 `settingsView&&diagPanel&&settingsView.parentNode` 空引用保护；同步段在 preinit 隐藏期内执行，无重排闪烁。
    - `initEmbedTuneFrames()` 的 `fit()` 高度公式修复间隙根因：原取 `Math.max(body.scrollHeight, body.offsetHeight, documentElement.scrollHeight, documentElement.offsetHeight)`，而 `documentElement.scrollHeight` 被 iframe 自身视口高度钳制（≥占位高 820px）——内容较短的漂移子页实测底部多出 ~158px 空白。改为只量 `body`（scrollHeight/offsetHeight 取大）+ `getComputedStyle` 的上下外边距，+2px 余量不变。
    - drift 子页：`body.embedded{margin-top:0;margin-bottom:10px}`——embedded 下上边距清零（与上方 RC 板块的间距由父页面板 padding 提供）、下边距 12px→10px 对齐板块间距。
    - judge 子页：`body.embedded{max-width:none}` → `body.embedded{max-width:none;margin-top:0}`——`margin:16px auto` 的 16px 顶边距在融合页里与漂移 iframe 底部空白叠加成大间隙，embedded 下清零；另加 `body.embedded .panel{margin-top:0}`——embedded 下头部/hero/gyroZ 面板全隐藏后，「评分阈值调参」面板 `margin:12px 0` 的 12px 顶边距成为首个可见面板的上间隙来源，一并清零。
    - playwright 实测（车上 v1.8.63 → 本版）：「保存漂移配置」按钮行底部到 Judge 首个板块顶部间距 174px → 22px（目标 ≤24px、与其他板块间距 10~14px 一致：面板 padding 10 + 下边距 10 + 2px 余量）。
  - DD 侧（DonkeyDrift 仓库 `Tony-cc-declutter` 分支同步提交）：`web_ui/frontend/src/components/CarSettingsPanel.tsx` iframe src 去掉 `&wifi=1`（`?embedded=1&settings=1&wifi=1` → `?embedded=1&settings=1`）+ 顶部注释更新；`CarSettingsPanel.test.tsx` 断言同步。车端独立 DC 页面与固件 `wifi=1` 能力本身不动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.63 → v1.8.64（先合并本地 Tony 的 v1.8.62/v1.8.63 后 +1；枚举全部本地分支最高为 v1.8.63）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_web_console_settings_view_shows_rc_channels_panel` 新增 diagnosticsPanel 前移断言（空引用保护、执行顺序在 wifi 分支后/rcFold 压平前）；`test_web_console_settings_view_embeds_tune_sections` 新增 fit() 新公式与旧 documentElement 公式移除断言、judge `margin-top:0`、drift embedded 边距断言；版本断言 v1.8.64、CHANGELOG 顺序链补 v1.8.64~v1.8.58 行。

## 2026-08-23 v1.8.63

- fix(WebConsole): STA 上位机配网两处修复——配网成功后「发送到上位机」按钮变为「完成」；密码框为掩码占位时先取真实明文再发送，修复上位机收到"点点点"导致 nmcli 配网失败
  - 背景①（按钮文案）：上位机配网成功（状态条显示「已连接 IP: x.x.x.x」）后，主按钮仍停留在「发送到上位机」，用户不知道流程已结束、以为要再点一次。
  - 背景②（掩码密码）：ESP32 已存过 STA 密码时，密码框由 `renderStaPasswordState()` 填入 `*` 掩码占位（`staPasswordPlaceholder=true`、`staPasswordDirty=false`）；`saveHostWifi()` 此前直接取 `staPassword.value` 组 `WIFI|ssid|pwd` 串口帧发给上位机，用户不改密码直接发送时上位机收到字面量 `********`，nmcli 用掩码当密码配网必败。ESP32 直连路径 `saveWifiSta()` 有 `keep_password` 兜底，但上位机 nmcli 必须要明文，无等价兜底。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `saveHostWifi()`：`!staPasswordDirty && staPasswordPlaceholder`（框内是掩码占位、用户未改过）时先取真实明文再发送——优先 `fetch('/api/wifi-sta/password?ssid='+encodeURIComponent(ssid))`（v1.8.57 引入的历史条目密码分支，SSID 被手改后仍能取到对应网络的密码），未命中回退 `fetchSavedStaPassword()`（当前已存 STA 密码，覆盖"打开弹窗直接发送"主路径）；两路都失败时报错并中止发送，不再发出掩码。
    - `pollHostWifiStatus()` connected 分支：状态转「已连接」时把 `staConnectBtn` 文案改为新 i18n `wifi.hostDoneBtn`（完成 / Done）、`onclick` 改为 `closeWifiStaModal`——点击即关闭弹窗；CC 内嵌视图（`body.wifi` CSS 使弹窗 `display:block` 常驻）点击仅移除 `show` 类、面板不消失。
    - 新增 `resetStaConnectBtn()` 辅助函数（上位机模式下把按钮恢复为「发送到上位机」+`saveHostWifi`），在三处"用户要配另一个网络"的入口调用：`selectWifiHistory()`（点历史条目）、`selectWifiSsid()`（扫描弹层选 SSID）、密码框 `input` 监听；弹窗重开/开关切换由 `onHostWifiToggle()` 既有逻辑复位。
    - i18n：新增 `wifi.hostDoneBtn` 中英键。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.62 → v1.8.63。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本与 CHANGELOG 顺序断言升至 v1.8.63；新增 `test_web_console_host_wifi_done_button_and_plaintext_password`（`wifi.hostDoneBtn` 中英键、connected 分支按钮改写、掩码占位时两路明文拉取与中止、`resetStaConnectBtn` 定义与三处调用）。

## 2026-08-23 v1.8.62

- fix(WebConsole): AP 名称配置弹窗的前缀规则提示行改为按需显示——默认隐藏，仅在用户输入了不符合规范的前缀字符时提示
  - 背景：`#apNotice` 提示行（「前缀仅限大小写字母和数字，不超过6位；后缀固定为"-ESP"。保存后会重启 AP…」）此前在弹窗中常显，占视觉空间；规则本身已由输入框实时剔除非法字符兜底，常显提示没有必要。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `#apNotice` 加 `style="display:none"` 默认隐藏；`openWifiApModal()` 每次打开弹窗时重置为隐藏态。
    - `updateApPreview()`：比对本次输入的原始值与剔除非法字符（`[^A-Za-z0-9]`）后的值——剔除了字符（`raw!==clean`）即显示提示行，未剔除（全合法或清空）则保持隐藏；整段显隐逻辑包在 `if(!apSaving)` 内，避免保存流程中输入把保存中/失败消息抹掉。
    - `saveWifiAp()`：无效输入（`wifi.apInvalid`）、保存中（`wifi.apSavingNotice`）、保存失败（`wifi.apInvalid`/`wifi.apSaveFailed`）三处设置文案时同时强制显示提示行，保存流程消息可见性不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.59 → v1.8.62（v1.8.60、v1.8.61 由并行会话 `Tony-dc-embedded-declutter` 分支使用、未合入本地 Tony，跳号避开碰撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本与 CHANGELOG 顺序断言升至 v1.8.62；`test_web_console_ap_ssid_modal_and_api_are_present` 新增 `#apNotice` 默认隐藏 marker（`style="display:none"`）、非法字符比较逻辑（`raw!==clean`）与显隐赋值（`apNotice.style.display`）断言。

## 2026-08-23 v1.8.61

- fix(WebConsole): 修复 DC 首屏揭开太慢、CC iframe 场景偶发卡死纯黑屏——`initLanguage()` 改「同步先应用+揭示，后台再对齐服务器偏好」，揭示不再依赖任何网络请求
  - 根因：v1.8.60 的 preinit 隐藏机制把揭示时机挂在异步 `fetch('/api/language')` 之后——`initLanguage()` 先 await 语言请求再 `applyLanguage()`+移除 preinit；ESP32 单线程 Web 服务在 CC 场景要串行伺候 139KB 主页 + drift/judge 两个子 iframe + 三个语言请求 + 状态轮询 + WS，语言请求排队期间整页 `visibility:hidden`（透出 DD 深色背景即成纯黑屏）；playwright 实测裸车页与 DD 8001 iframe 在车空闲时正常（约 0.75~1s 揭示），拥塞时揭示被无限拖后，今日还观察到一次车辆自发重启（约 1 分钟自恢复），加载中断进一步放大该问题。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（三切片 `WIFI_WEB_CONSOLE_HTML`/`WIFI_WEB_DRIFT_HTML`/`WIFI_WEB_JUDGE_HTML` 同款修改，UPDATE 页无 preinit 机制不动）：
    - `initLanguage()` 重写为两段：同步段 `readUrlLanguage()` → `readStoredLanguage()` → `detectBrowserLanguage()` 取语言后立即 `applyLanguage(lang)` + `document.body.classList.remove('preinit')`——首帧即正确语言（localStorage 缓存或 navigator 检测覆盖绝大多数情况），揭示不再等任何网络请求；函数保持 `async` 签名但内部已无 await。
    - 后台对齐段：仅当无 URL `?lang=` 参数时（保持原语义优先级 URL 参数 > 服务器偏好 > 本地缓存/浏览器检测），异步 `fetch('/api/language',{cache:'no-store'})`，成功且服务器语言与当前 `uiLang` 不同才 `writeStoredLanguage(srv)`+`applyLanguage(srv)` 重新应用（`auto` 仍映射 `detectBrowserLanguage()`，保留原 writeStoredLanguage 副作用）；`.catch(()=>{})` 吞掉失败——服务器偏好像原来一样最终生效，但不再阻塞首屏。
    - `<body>` 后早期内联脚本与 2s 兜底定时器保留不动（init 出错或 JS 异常时仍强制揭示）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.60 → v1.8.61（全本地分支枚举最高为 v1.8.60，+1）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_web_console_preinit_reveal_no_first_paint_flash` 语义更新：断言揭示在同步 `applyLanguage` 后立即发生（不再有 `await fetch` 先于揭示）、后台对齐段存在（`if(!urlLang){fetch(...)` + 差异才 re-apply + catch 吞掉）、URL 参数优先保持、2s 兜底与早期脚本不动；版本断言 v1.8.61、CHANGELOG 顺序链补 v1.8.61 行。

## 2026-08-23 v1.8.60

- fix(WebConsole): 消除 DC 页面首屏两种闪烁——CC 内嵌视图先闪完整控制台再切设置视图、先闪英文再刷中文
  - 根因：三个 HTML 切片（`WIFI_WEB_CONSOLE_HTML`/`WIFI_WEB_DRIFT_HTML`/`WIFI_WEB_JUDGE_HTML`）的 `embedded`/`settings`/`wifi` body 类与 `initLanguage()` 都在尾部 init 脚本才执行——139KB 主页在类应用前已按完整控制台渲染（首闪）；`initLanguage()` 是 async，先 `fetch('/api/language')` 再 `applyLanguage(lang)` 同步替换全部 data-i18n 文本，fetch 往返期间整页英文（二闪）。CC 视图里的漂移/Judge 子 iframe（`/drift?embedded=1`、`/judge?embedded=1`）同模式同问题。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（三切片同款修复，UPDATE 页不动）：
    - `<body>` 标签后（任何可见元素之前）插入微型同步脚本：按 `location.search` 立即给 body 加 `preinit` + `embedded` 类（主页另有 `settings`/`wifi` 类；drift/judge 无 settings/wifi 视图只加 embedded），并 `setTimeout(remove preinit,2000)` 兜底 + try/catch（防 init 出错页面永远空白）。首帧即有正确视图类，完整控制台不再闪现。
    - 各切片 CSS 前部新增 `body.preinit{visibility:hidden}`——选 visibility 而非 display:none：隐藏期间布局/ResizeObserver/canvas DPR 测量照常工作。
    - `initLanguage()` 内 `applyLanguage(lang)` 之后追加 `document.body.classList.remove('preinit')`——中文应用前页面不可见，英文不再闪现；fetch 失败走 catch→localStorage→applyLanguage 照常显示，2s 兜底强制显示。
    - 尾部 init 里原有 `classList.add('embedded')` 等赋值保留（幂等保险），wifi 分支 `openWifiApModal()/openWifiStaModal()` 等逻辑不动。
  - `initTheme()` 核查：三切片均为同步函数（`uiTheme=...;applyTheme()` + matchMedia 监听，无 fetch），且尾部 init 中同步先于 `initLanguage()` 的 await 完成，preinit 隐藏期已覆盖——无主题闪烁隐患，未改动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.59 → v1.8.60（v1.8.59 由本地 Tony 的 STA 密码框修复占用，+1 防撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——新增 `test_web_console_preinit_reveal_no_first_paint_flash`（三切片早期脚本紧贴 `<body>` 后且内容含 preinit/embedded 赋值与 2s 兜底、主页另有 settings/wifi 赋值而 drift/judge 没有、preinit CSS 在首个 style 块内、initLanguage 内 reveal、尾部原有类赋值与 wifi 弹窗逻辑保留、UPDATE 页不含 preinit）；版本断言 v1.8.60、CHANGELOG 顺序链补 v1.8.60 行。

## 2026-08-23 v1.8.58

- fix(WebConsole): 「上位机配网」状态条永远显示「等待...」——STA 板块打开时立即拉取并 5s 慢轮询 /api/host-wifi-status，IDLE 状态改显示上位机在线/等待上报真实状态（Issue #234 后续）
  - 根因：`openWifiStaModal()` 强制 `hostWifiToggle.checked=true` 并调 `onHostWifiToggle()`，但该函数 checked 分支只做 `bar.style.display='block'`，从不 fetch `/api/host-wifi-status`——2s 轮询只在 `saveHostWifi()`（点「发送」）后才启动。CC 内嵌视图（?embedded=1&settings=1&wifi=1）里 STA 板块常开，label 永远停在 HTML 占位文字 `wifi.hostStatus.idle`（等待...）。实际上位机在线且周期上报（Serial2 `HOSTIP|` 帧，运行时全局 `hostReportedIp`/`hostReportedIpMs`），`/api/status` 已有 `host_ip`/`host_ip_age_s`，但 host-wifi-status 接口与前端都没用它。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：`handleWifiWebHostWifiStatus()` 的 JSON 新增 `"host_ip"` 与 `"host_ip_age_s"` 字段（数据来自 `hostReportedIp`/`hostReportedIpMs`，age 算法与 `/api/status` 一致：`hostReportedIpMs ? (millis() - hostReportedIpMs) / 1000UL : 0UL`）；`json.reserve(160)` 加大到 224。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `onHostWifiToggle()` checked 分支末尾追加 `pollHostWifiStatus();stopHostWifiPoll();hostWifiPollTimer=setInterval(pollHostWifiStatus,5000)`——打开 STA 板块立即拉真实状态 + 5s 慢轮询保持新鲜（先停旧定时器防重复）；unchecked 分支既有 `stopHostWifiPoll()` 不动。
    - `closeWifiStaModal()` 补 `stopHostWifiPoll()`（原关弹窗不停轮询有小泄漏；CC 内嵌视图不关弹窗，常开轮询正是预期行为）。
    - `pollHostWifiStatus()` IDLE 分支重写：`host_ip` 非空且 `host_ip_age_s<=60` 视为上位机在线——label 显示新 i18n `wifi.hostStatus.hostOnline`（上位机在线 / Host online），`hostWifiStatusIp` 显示 `IP: x.x.x.x`，error span 隐藏；否则 label 显示新 i18n `wifi.hostStatus.waitingHost`（等待上位机上报 / Waiting for host report），ip/error span 隐藏。connecting/connected/failed 分支保持现有行为（含 connected/failed 里 `stopHostWifiPoll()` 自愈）；原 `{IDLE:...}` 状态映射表移除 IDLE 项，IDLE 改由上述专用分支处理。
    - i18n：`wifi.hostStatus.idle` 键删除（JS 与 HTML 占位均已不再引用），新增 `wifi.hostStatus.hostOnline` 与 `wifi.hostStatus.waitingHost` 中英各一份；状态条 HTML 占位 `data-i18n` 由 `wifi.hostStatus.idle` 改为 `wifi.hostStatus.waitingHost`（首次拉取前的占位语义对齐）。
    - 状态条 label 移除 `data-i18n`（首轮车上 playwright 验证发现的覆盖竞态：`initLanguage()` 异步 fetch `/api/language` 后 `applyLanguage()` 按 `data-i18n` 重写全部占位元素 textContent，会把首轮轮询已显示的「上位机在线」覆盖回「等待上位机上报」、最长 5s 后才由下一轮轮询自愈；label 为状态驱动元素（与 `refreshDynamicLabels()` 管辖的按钮同类），不走 data-i18n，占位文字保留硬编码「等待上位机上报」，首轮轮询 <1s 即被真实状态替换）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.53 → v1.8.58（并行会话接连占号：v1.8.54 扫描弹层浅色修复、v1.8.57 STA 历史回填，v1.8.55/v1.8.56 被对方先占后弃而跳过，撞号四次 +1）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——新增 `test_web_console_host_wifi_status_bar_shows_host_report_state`（后端新 JSON 字段/reserve 224、前端 onHostWifiToggle 立即拉取+5s 轮询、closeWifiStaModal 停轮询、IDLE 分支 host_ip/age<=60 判断、新 i18n 键存在且 idle 键删除）；版本断言 v1.8.58、CHANGELOG 顺序链补 v1.8.58 行。

## 2026-08-22 v1.8.59

- fix(WebConsole): 抑制苹果设备在 STA Wi-Fi 密码框输入时弹出「存储密码？」——密码框声明为非登录新密码并加密码管理器忽略属性
  - 背景：STA 配网弹窗的密码框是裸 `type="password"` 输入框，Safari/iOS 把它当作登录凭据，输入后弹出系统级「是否存储此密码」；该密码是 Wi-Fi 预共享密钥、不是网站账号，保存提示无意义且打扰。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`#staPassword` 加 `autocomplete="new-password"`（告知浏览器这是设置新密码而非登录——Safari/Chrome 对 new-password 字段不弹保存提示）+ `data-1p-ignore="true"`（1Password）+ `data-lpignore="true"`（LastPass）+ `data-form-type="other"`（Dashlane 等）；`#staSsid` 加 `autocomplete="off" autocapitalize="none" spellcheck="false"`（避免被识别为登录用户名字段，顺带关闭 iOS 首字母大写与拼写检查）。眼睛切换显隐逻辑不受影响。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.57 → v1.8.59（v1.8.58 由并行会话 `Tony-dc-embedded-declutter` 分支使用，跳号避开碰撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本与 CHANGELOG 顺序断言升至 v1.8.59；STA 弹窗测试新增两条输入框完整属性断言（autocomplete/autocapitalize/spellcheck/密码管理器忽略属性）。

## 2026-08-22 v1.8.57

- feat(WebConsole): STA Wi-Fi 配置右侧「已保存网络」历史列表支持点击回填——点击一行即把该网络的 SSID 与密码填入左侧表单，直接点「连接」即可切换
  - 背景：历史列表（`wifiHistoryList`）此前只展示 + 单条删除，切换已存网络要手动重输 SSID 和密码。历史条目本就存了密码（NVS `sta_h{0..4}p`），但公开的历史列表 API 按设计只输出 `password_set` 标志、从不出明文。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：`handleWifiWebStaPassword()` 新增 `?ssid=` 参数分支——传 SSID 时经 `findWifiStaHistoryEntry()` 返回该历史条目的 `password_set`/`password_len`/密码明文（与该端点既有「当前配置密码」分支同一鉴权：控制台认证或 DEV 模式，安全等级不变）；未命中历史返回 404 `not_found`。不传 `ssid` 时行为完全不变。历史列表公开 GET 仍只输出 `password_set` 标志、不含明文。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - JS 新增 `selectWifiHistory(ssid,passwordSet)`：填 `staSsid`；`password_set` 为真时 `fetch('/api/wifi-sta/password?ssid='+encodeURIComponent(ssid))` 取回密码填入密码框（填真实密码而非占位星号，`staPasswordDirty=true` 使保存时显式发送该密码，避免错用当前配置的 keep_password），开放网络则清空密码框；重置眼睛可见态/占位态/已取密码缓存，焦点落到「连接」按钮。
    - `refreshWifiHistory()`：历史行加 `onclick` 调 `selectWifiHistory()`、加 `title` 悬停提示；删除按钮 `onclick` 改带 `ev.stopPropagation()`，点 🗑 不再误触发行回填。
    - CSS：`.histRow` 加 `cursor:pointer` 表明可点。
    - i18n：新增 `wifi.historyFill` 中英键（「点击填充 SSID 与密码」/「Click to fill SSID and password」）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.54 → v1.8.57（v1.8.55、v1.8.56 先后被并行会话 `Tony-dc-embedded-declutter` 分支提交使用，两次跳号避开碰撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本与 CHANGELOG 顺序断言升至 v1.8.57；`test_web_console_wifi_sta_history_ui` 新增回填断言（`selectWifiHistory` 定义/行 onclick/stopPropagation/`?ssid=` 拉取/cursor:pointer/`wifi.historyFill` 中英键）；`test_web_console_sta_password_endpoint_is_protected_and_public_state_has_no_secret` 新增 `?ssid=` 分支断言（arg 读取/history 查找/404/明文仅在该鉴权端点输出）。

## 2026-08-22 v1.8.54

- fix(WebConsole): 修复 STA Wi-Fi 配置「搜索网络」扫描弹层在浅色模式下仍为深色——浅色主题 CSS 选择器列表漏了一个逗号
  - 背景：主控制台浅色主题块中 `html[data-theme="light"] .scanPopover` 与紧随其后的 `html[data-theme="light"] .foldHead` 之间漏写逗号，被拼成无效选择器 `.scanPopoverhtml[data-theme="light"] .foldHead`（`scanpopoverhtml` 元素不存在），整条声明对扫描弹层与折叠头均不生效；浅色模式下 STA 配网点 ⌕ 打开的扫描弹层仍是深色底 `#111820` + 蓝边 `#5cc8ff`。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`.scanPopoverhtml[data-theme="light"] .foldHead` 改为 `.scanPopover,html[data-theme="light"] .foldHead`（插入一个逗号），恢复 `.scanPopover` 浅色覆盖（浅底 `#f4f6f9`、浅边框 `#d5dce4`、深文字 `#1f3a52`），同时恢复 `.foldHead`（RC Channels / STATUS Details 折叠头）的浅色底色。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.48 → v1.8.54（v1.8.49~v1.8.53 由并行会话 `Tony-dc-embedded-declutter` 分支使用、未合入本地 Tony，跳号避开碰撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本与 CHANGELOG 顺序断言升至 v1.8.54；新增 `test_sta_scan_popover_light_theme_selector` 断言修复后的 `.scanPopover,html[data-theme="light"] .foldHead{` 选择器存在且无效拼接 `scanPopoverhtml` 不再出现。

## 2026-08-22 v1.8.53

- style(WebConsole): CC 内嵌设置视图删掉漂移子页顶部的 Status 状态栏面板（embedded 作用域限定，Issue #234 后续）
  - 背景：CC 车辆设置内嵌视图（?embedded=1&settings=1&wifi=1）里漂移子 iframe 顶部有一条 Status 状态栏（Enabled/Active/Yaw Error/Throttle Mode 四项实时状态，中文显示 关/待命/0.00/直通）——显示类内容不属于设置（DD 主视图已有遥测），用户要求「把最上面的状态栏删掉」；车端独立 /drift 页面保持原样完整显示。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（WIFI_WEB_DRIFT_HTML 切片）：做法与 v1.8.51 judge 页 `#judgeHero`/`#gyroChartPanel` 隐藏完全一致——漂移页 body 起点 headerRow 之后的第一个 `.panel`（Status 面板，含 `drift.status.label` 标题与 `stateEnabled`/`stateActive`/`stateYawError`/`stateThrottleMode` 四个 summaryItem）加 `id="driftStatusPanel"`；既有 `body.embedded .headerRow{display:none}` 规则旁新增 `body.embedded #driftStatusPanel{display:none}`。漂移页 JS 仍正常更新这些元素，纯 CSS 隐藏不影响运行；CC 内嵌视图里 `initEmbedTuneFrames()` 的 ResizeObserver 会自动收掉子 iframe 高度。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.52 → v1.8.53。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_web_console_settings_view_embeds_tune_sections` 的 DRIFT 段断言新增 `id="driftStatusPanel"` 与 `body.embedded #driftStatusPanel{display:none}` 两项；版本断言 v1.8.53、CHANGELOG 顺序链补 v1.8.53 行。

## 2026-08-22 v1.8.52

- style(WebConsole): CC 内嵌设置视图 RC Channels 折叠头压平为静态标题（常开不可折叠）+ 配网板块上移为内嵌视图最顶部板块（Issue #234 后续）
  - 背景：① CC 内嵌设置视图里 RC Channels 是可折叠面板，折叠头按钮样式（背景/边框/箭头/hover）在内嵌场景下显得多余，用户要求压平成静态标题、内容常开不可折叠（标题文字与标题级悬停灰字 hint 保留）；② 内嵌视图（?embedded=1&settings=1&wifi=1）当前可见顺序为车辆设置→RC Channels→配网，用户要求配网板块（AP 名称配置 + STA Wi-Fi 配置）移到最顶部。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - RC 折叠头压平（`body.settings`/`body.wifi` 作用域限定，只影响内嵌 settings/wifi 视图）：新增 `body.settings #rcFold .foldHead,body.wifi #rcFold .foldHead{background:transparent;border:none;cursor:default;pointer-events:none}`、对应 `:hover` 背景透明规则、`... .foldHead .titleHint{pointer-events:auto}`（保留标题级 hintSpan 悬停灰字）与 `... .foldIcon{display:none}`（隐藏 ▸ 箭头）；作用域规则带 ID，特异性高于 `.foldHead`/`.foldHead:hover` 及 light 主题变体，无需单独 light 变体。
    - JS 初始化：旧 `if(...settings=1...||...wifi=1...)toggleFold('rcFold')` 替换为强制展开且不可折叠逻辑——`#rcFold` 加 `open` class、foldHead 移除 `onclick` 属性（防点击/键盘触发折叠）、`aria-expanded` 置 `true`，全程空引用保护；`toggleFold()` 函数本身保留，车端独立 DC 页（无 URL 参数）rcFold 仍默认收起、点击可展开/收起。
    - 配网板块上移：`wifi=1` 初始化分支（`openWifiApModal();openWifiStaModal();` 之后）把 `#wifiApModal`、`#wifiStaModal` 依次 `insertBefore` 到 `#settingsView` 之前（先 AP 后 STA，最终顺序 AP、STA、settingsView），合并的配网板块成为内嵌视图最顶部板块；三者均为 body 直接子节点，加空引用保护；普通 DC 页无 `wifi=1` 不受影响。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.51 → v1.8.52。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_web_console_settings_view_shows_rc_channels_panel` 更新：旧 `toggleFold('rcFold')` 初始化断言改为断言其不存在，新增强制展开 JS（`classList.add('open')`/`removeAttribute('onclick')`/`aria-expanded='true'`）、4 条压平 CSS 规则与配网板块 `insertBefore` 上移断言；版本断言 v1.8.52、CHANGELOG 顺序链补 v1.8.52 行。

## 2026-08-22 v1.8.51

- style(WebConsole): CC 内嵌设置视图融合漂移/Judge 为一页（去分类标题、Judge 撑满宽度、隐藏显示类区域）+ 删除 RC Channels 字段级标题的悬停灰字特效（Issue #234 后续）
  - 背景：① RC Channels 里 CH1 Steering/CH2 Throttle 等是字段标签不是小标题，悬停灰字特效（v1.8.46 引入）不应加在它们上面；② 用户要求 CC 设置页不按「漂移设置/Judge 设置」分类，融合成一个有逻辑顺序的页面；③ Judge 板块有 760px 居中限宽，没撑满。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - RC 面板字段级特效删除：12 个 rcCell 标题（CH1~CH6 / OUT Steering / OUT Throttle / Mid S / Mid T / Min T / Max T）的 `.titleHint`+`.hintSpan` 包装解包回纯 `<b>`，对应 12 组中英 i18n 键（`rc.hint.ch1`~`ch6`/`outSteering`/`outThrottle`/`midS`/`midT`/`minT`/`maxT`）删除；保留标题级特效——手柄校准弹窗大标题（`cal.title.hint`）与 RC Channels 折叠头（`rc.hint.panel`），/drift 页与 /judge 页标题级 hint 不动（审计确认无其它字段级混入）。
    - 融合：embedTuneSections 去掉「漂移设置」「Judge 设置」两个 h3 分类标题（连同 `.embedTuneSection h3` CSS 与 light 变体、`.embedTuneSection` 间距规则），两个子 iframe 前后相接成一页——逻辑顺序：先车辆动态参数（漂移：状态/转向修正/油门策略），后评判规则（Judge：评分阈值/基础阈值/评分参数/评分维度）。
    - Judge 页 embedded 作用域：`body.embedded{max-width:none}` 放开 760px 限宽撑满 iframe；`#judgeHero`（得分/碰撞 hero）与 `#gyroChartPanel`（gyroZ 曲线）两个显示类区域加 id 并在 embedded 时隐藏——CC 设置页只留设置表单（显示类内容不属于设置，且 DD 主视图已有遥测图表）；车端独立 /judge 页面不动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.50 → v1.8.51。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_joystick_cal_and_rc_panel_title_hints` 改为断言 12 处 rcCell 无 titleHint 包装、12 组 i18n 键已删、标题级 2 处（弹窗+折叠头）保留、控制台 titleHint 计数 14→2；`test_web_console_settings_view_embeds_tune_sections` 更新——h3 移除、judge embedded 撑宽/隐藏规则断言；版本断言 v1.8.51。

## 2026-08-22 v1.8.50

- fix(WebConsole): CC 车辆设置内嵌视图的漂移/Judge 子 iframe 按内容自动撑高——消除 Judge 设置的内部滚动条，整页统一滚动（Issue #234 后续）
  - 背景：v1.8.47 的内嵌子 iframe 用固定高度（drift 820px / judge 1000px），Judge 页内容超出后出现独立的内部滚动条，用户要求在整页里完全显示、统一滚动。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - JS 新增 `initEmbedTuneFrames()`：子 iframe load 后按内容高度自动撑高——同源 iframe 直接读 `contentDocument` 的 body/documentElement scroll/offset 高度取最大（+2px 防 1px 滚动），立即 + 400ms + 1600ms 三次 fit 覆盖异步内容，并用 `ResizeObserver` 持续跟踪内容高度变化；仅高度变化时才写 style，收敛无循环。
    - 懒加载：两个子 iframe 的 `src` 改为 `data-src`，仅 `body.embedded.settings`（CC 车辆设置内嵌视图）才由 init 赋值加载——其它视图（车端独立 DC 主页、DD 嵌入主视图）不再隐藏加载 /drift /judge，消除子页 250ms 轮询/WS 对 ESP32 的无效占用（v1.8.47 引入的隐患）。
    - CSS：`.embedTuneFrame` 加 `overflow:hidden`；820px/1000px 固定高度保留为加载前占位。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.49 → v1.8.50。
  - 测试同步：`tests/test_firmware_feature_flags.py` `test_web_console_settings_view_embeds_tune_sections` 更新——iframe 断言改 `data-src`，新增 `initEmbedTuneFrames()` / `ResizeObserver` / 懒加载守卫（`classList.contains('embedded')&&document.body.classList.contains('settings')` 才调用）断言；版本断言 v1.8.50。

## 2026-08-22 v1.8.49

- feat(WebConsole): CC 车辆设置内嵌视图删「车辆设置」标题与「调校」行框——手柄校准按钮移至 DD CC 页顶栏，经 postMessage 打开校准弹窗（Issue #234 后续）
  - 背景：内嵌设置视图里漂移/Judge 设置已默认展开（v1.8.47），调校行只剩手柄校准一个按钮；用户要求把该按钮移到 DD CC 页顶部「重新扫描」右边，并删掉「车辆设置」标题与「调校」行框。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - CSS（`body.embedded.settings` 作用域限定）：新增 `body.embedded.settings #settingsView .setTitle{display:none}` 与 `body.embedded.settings #settingsView .setRow{display:none}`——「车辆设置」标题与「调校」行（含两个跳转按钮与手柄校准按钮）整行隐藏；v1.8.47 的 `#driftSettingsBtn/#judgeSettingsBtn{display:none}` 规则已被 setRow 整行隐藏覆盖，移除。车端独立 DC 页面与独立设置视图（无 embedded）保持原样。
    - JS：既有 `window.addEventListener('message', ...)`（dd-console-mute-changed / dd-open-wifi-sta / dd-open-wifi-ap）追加 `dd-open-joystick-cal` 分支——收到 DD CC 页顶栏「手柄校准」按钮的 postMessage 后调用 `openJoystickCalModal()`，弹窗在内嵌 iframe 可视区内居中打开。
  - DD 侧配套（DonkeyDrift 仓库 `Tony-cc-declutter` 分支）：`CarSettingsPanel.tsx` 顶栏「重新扫描」右侧新增「手柄校准」按钮，点击 postMessage `{type:'dd-open-joystick-cal'}` 到内嵌 iframe（沿用 DrifterConsolePage 静音同步的同款 postMessage 通道）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.47 → v1.8.49（v1.8.48 由并行会话的 Judge 页主题跟随条目占用，跳号避撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py` `test_web_console_settings_view_embeds_tune_sections` 更新——按钮隐藏规则断言替换为 setTitle/setRow 整行隐藏断言，新增 `dd-open-joystick-cal` 监听分支断言；版本断言 v1.8.49。

## 2026-08-22 v1.8.48

- feat(WebConsole): Drift Judge 页（/judge）跟随 Drifter Console 深浅色主题，全部标题改为悬停灰字提示（titleHint），样式对齐 /drift 调参页
  - 背景：/judge 页 CSS 全部硬编码深色且无 `?theme=` 支持，从 DD Car Connector 内嵌视图或控制台「Judge 设置」进入始终是黑界面；大小标题也没有 v1.8.36 漂移页的悬停灰字提示。本次只补主题跟随与标题提示两点；页内「返回 Drifter Console」链接保留（本次未要求删除），页内本无主题切换按钮、不新增。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主题跟随：`<head>` 内新增防闪烁脚本（读 `?theme=` URL 参数，缺省按系统 `prefers-color-scheme`，与 /drift 页逐字一致）；`<style>` 新增 `html[data-theme="light"]` 全量浅色覆盖（body/a/button/button.alt/muted/label/sub/mini/sectionDesc/fieldHint/summaryItem .k/scoreExplain/dimTrend/panel/heroCard/card/summaryItem/field/statusPill/collision/field input/sectionTitle/fieldTitle/dimName/bar/dimBar/chartWrap/tuneSection）；JS 新增 `uiTheme`/`readUrlTheme()`/`systemTheme()`/`resolvedTheme()`/`readParentTheme()`/`applyTheme()`/`initTheme()` 内存态主题（不写 localStorage），`initTheme()` 以 `?theme=` 参数优先、无参数且处于同源 iframe（CC 内嵌视图 `/judge?embedded=1`）时读父页 `documentElement.dataset.theme` 跟随 Drifter Console 当前主题、再缺省 auto 跟随系统；初始化序列调用 `initTheme()`；`drawChart()` 图表底色/网格线/曲线色改按 `resolvedTheme()` 取色（浅色 `#f4f6f9`/`#d5dce4`/`#0c9bd6`，深色不变）。
    - 控制台入口：设置视图调校行「Judge 设置」按钮 `location.href='/judge'` 改为 `location.href='/judge?theme='+resolvedTheme()`（与漂移设置入口同款）。
    - 标题悬停灰字：`<style>` 新增 `.titleHint`/`.hintSpan`（含 `html[data-theme="light"] .hintSpan` 变体，与 /drift 页规则逐字一致）；6 处标题包为 titleHint——h1「Drift Judge」（原副标题转为 hintSpan）、gyroZ 曲线（新增 `judge.gyroChartHint` 中英键「实时偏航角速度曲线」/「Live gyroZ trace」）、评分阈值调参（`judge.tuneDesc` 由 muted 副行转为 hintSpan）、基础阈值（`judge.section.thresholdsDesc` 由 sectionDesc 转为 hintSpan）、评分参数（`judge.section.scoringDesc` 同）、评分维度（`judge.dimDesc` 由 muted 副行转为 hintSpan）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.46 → v1.8.48（v1.8.47 由并行会话 `session-dc-embedded-declutter` 分支使用、未合入本地 Tony，跳号避开碰撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本与 CHANGELOG 顺序断言升至 v1.8.48；新增 `test_judge_page_theme_and_title_hints` 断言防闪烁脚本/浅色覆盖/内存态主题 JS（无 localStorage、无切换按钮、`readParentTheme()` 同源父页主题跟随）/drawChart 主题取色/6 处 titleHint 与 hintSpan/悬停 CSS/`judge.gyroChartHint` 中英键/控制台 `/judge?theme=` 入口。

## 2026-08-22 v1.8.47

- feat(WebConsole): CC 车辆设置内嵌视图（`?embedded=1&settings=1&wifi=1`）默认展开漂移设置与 Judge 设置——两个跳转按钮改为同页内嵌子 iframe 板块（Issue #234 后续）
  - 背景：CC 内嵌设置视图里「漂移设置 / Judge 设置」此前是两个跳转按钮，点击会把整个 iframe 导航走；用户要求这两个设置默认展开、直接可见可调。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主页 settingsView 调校行：「漂移设置」「Judge 设置」按钮加 `id="driftSettingsBtn"` / `id="judgeSettingsBtn"`；行后新增 `<div class="embedTuneSections">`，含两个内嵌子板块（h3 标题复用既有 i18n 键 `button.driftSettings` / `settings.judge` + `<iframe class="embedTuneFrame" src="/drift?embedded=1">` / `src="/judge?embedded=1"`）。
    - 主页 CSS（`body.embedded.settings` 作用域限定）：`.embedTuneSections{display:none}` 默认隐藏、`body.embedded.settings .embedTuneSections{display:block}` 只在 CC 车辆设置内嵌视图显示；同作用域隐藏 `driftSettingsBtn`/`judgeSettingsBtn` 两个跳转按钮（「手柄校准」按钮保留，弹窗本就内联可用）；iframe 全宽无边框圆角 10px、深色底 `#101318`，driftFrame 高 820px / judgeFrame 高 1000px。
    - /drift 页：新增 `body.embedded .headerRow{display:none}`，init 开头加 `embedded=1` 检测置 `body.embedded` class——子 iframe 内隐藏自身头部。
    - /judge 页：头部 panel 加 `id="judgeHeadPanel"`，新增 `body.embedded #judgeHeadPanel{display:none}`，init 开头同样加 embedded class 检测。
    - 车端独立 DC 页面与独立设置视图完全不动：内嵌板块仅在 `body.embedded.settings` 显示；主题/语言无需传参——子 iframe 与车主页同源同 localStorage，主题走系统检测自动跟随。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.45 → v1.8.47（v1.8.46 由并行会话的 titleHint 条目占用，跳号避撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 新增 `test_web_console_settings_view_embeds_tune_sections`（内嵌板块 HTML/CSS 规则、两个按钮 id 与隐藏规则、drift/judge 子页 embedded 守卫与 init 检测——按 `WIFI_WEB_*_HTML` 切片断言避免与主页同名规则混淆）；版本断言 v1.8.47。

## 2026-08-22 v1.8.46

- feat(WebConsole): 手柄校准弹窗与 RC Channels 校准面板全部标题加悬停灰字提示（titleHint），样式对齐 /drift 调参页与 DD group-hover
  - 背景：漂移调参页（v1.8.36）已把大标题/各级小标题改为悬停右侧滑出灰字提示；手柄校准弹窗（`cal.title` 大标题）与 RC Channels 校准面板（`#rcFold` 折叠头 + CH1~CH6 / OUT Steering / OUT Throttle / Mid S / Mid T / Min T / Max T 共 12 个 `.rcCell` 小标题）仍是旧样式，用户要求同款。弹窗与面板本就连同控制台主页面自动跟随深浅色，无需自带切换按钮；本次仅补提示样式。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：主控制台 `<style>` 新增 `.titleHint`/`.hintSpan`（含 `html[data-theme="light"] .hintSpan` 变体，与 /drift 页规则逐字一致）；`#joystickCalModal` 标题、`#rcFold` 折叠头与 12 个 `.rcCell` 标题全部包为 `.titleHint`+`.hintSpan`；新增 14 组中英 i18n 键（`cal.title.hint`、`rc.hint.panel`、`rc.hint.ch1`~`rc.hint.ch6`、`rc.hint.outSteering`/`outThrottle`/`midS`/`midT`/`minT`/`maxT`）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.42 → v1.8.46（v1.8.43~v1.8.45 由并行会话 `session-dc-embedded-declutter` 分支使用、未合入本地 Tony，跳号避开碰撞）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本与 CHANGELOG 顺序断言升至 v1.8.46；`test_web_console_groups_rc_and_status_into_collapsible_sections` 的 rcFold 折叠头断言改为 `.titleHint` 包装后的新串；新增 `test_joystick_cal_and_rc_panel_title_hints` 断言 14 处 titleHint 包装、CSS 规则与 14 组中英 i18n 键齐全。

## 2026-08-22 v1.8.45

- style(WebConsole): wifi 内嵌融合单卡去掉 STA 卡上边框——消除 AP/STA 之间的黄色横杠
  - 背景：浅色主题下 `.dialog` 边框色为 `#d99a17`（金色），AP/STA 融合时只去了 AP 卡底边，STA 卡顶边残留，两卡之间出现一条黄杠（DD Car Connector 车辆设置内嵌视图可见）。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`body.wifi #wifiStaModal .dialog` 规则加 `border-top:none`，AP/STA 两卡背景无缝相接成真正单卡；深色主题同步生效（同规则覆盖）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.44 → v1.8.45。
  - 测试同步：`tests/test_firmware_feature_flags.py` 融合单卡断言更新为含 `border-top:none` 的 STA 规则；版本断言 v1.8.45。

## 2026-08-22 v1.8.44

- feat(WebConsole): DD 内嵌主视图（`?embedded=1`）隐藏设置类板块/入口——已全部移至 DD Car Connector 的车辆设置（Issue #234 后续）
  - 背景：DD Car Connector 已 1:1 内嵌车端设置（`?embedded=1&settings=1&wifi=1`：调校/RC 校准/AP/STA 配网），用户要求 DD 的 Drifter Console 嵌入页（`?embedded=1` 主视图）不再重复出现这些设置；车端独立 DC 页面（无参数直连）保持不动。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - CSS 新增 4 条守卫规则 `body.embedded:not(.settings):not(.wifi) #rcFold / #diagSettingsRow / #networkGear / #driftTuneLink{display:none}`——隐藏 RC Channels 校准面板、手柄校准+漂移设置按钮行、Network 卡 ⚙ 配网入口、Drift 卡 Tune 链接。`:not(.settings):not(.wifi)` 守卫必不可少：CC 的车辆设置 iframe URL 同样带 `embedded=1`，无守卫会把 CC 视图里的 rcFold 一起藏掉。
    - 诊断面板里原无 id 的「Calibrate Joystick + 漂移设置」按钮行加 `id="diagSettingsRow"` 供定点隐藏。
    - 主视图保留：状态卡（Mode/Park/Drift/Voltage/Network 显示）、遥测图表、串口终端、STATUS Details——纯显示/驾驶内容不动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.42 → v1.8.44（v1.8.43 为其它会话基于旧基点的 RC 面板+配网融合构建、正在车上运行且未以该形态回本仓库，跳号避免撞号；其内容已含在本地 Tony 中，本次构建基于本地 Tony 1e5e398、不落后）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 新增 `test_web_console_embedded_main_view_hides_settings_panels`（4 条守卫规则、按钮行 id、禁止不带 `:not` 的裸 `body.embedded` 隐藏规则）；版本断言更新至 v1.8.44。

## 2026-08-22 v1.8.42

- feat(WebConsole): DC 头部在 Kimi Code Web 与 DeepSeek Harness 之间新增「ZCode」入口按钮——点击经 launcher 端点 `POST /api/launch/zcode` 拿到网页终端 URL（`/terminal?cmd=...&title=ZCode&icon=zcode.png`），在新标签页的网页终端里运行 ZCode（TUI 编码 agent）
  - 背景：DC 头部已有 Kimi Code Web / DeepSeek Harness 两个弱化入口；ZCode 为 TUI agent，复用 launcher 的 `/terminal?cmd=` 网页终端机制，入口即开即得、端点毫秒级返回，无需冷启动等待。该改动最初在功能分支 Tony-zcode-entry 上完成但未及提交，后被基于更新 Tony 的 v1.8.41 编译刷车覆盖丢失，本次在最新 Tony 上恢复并重发。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 头部按钮：`openKimiCodeWebBtn` 与 `openDshBtn` 之间插入 `openZCodeBtn`（`class="navTabWeak"`，lucide Code2 内联 SVG 图标 14×14 `stroke="currentColor"`，两条 chevron 加一条斜线 path，与邻居同款；`<span data-i18n="button.openZCode">ZCode</span>`）。
    - JS：`openDsh()` 之后新增 `openZCode()`，逐行镜像 `openDsh()`（防重入标志 `zCodeLaunching` → `window.open('about:blank')` 占住新标签 → 禁用按钮切 Launching 文案 → AbortController + fetch POST `http://<launcherIp>:8090/api/launch/zcode` → 成功 `newTab.location.href=j.url` → 失败关标签 + showToast → finally 恢复）；超时 15s（端点无子进程、即时返回，不同于 kimi 的 120s 冷启动）。
    - i18n：zh/en 各在 dsh 组之后新增 `button.openZCode` / `button.openZCodeLaunching` / `toast.zCodeFailed` / `toast.zCodeTimeout` 四键（措辞镜像 dsh 组）。
    - 移动端 CSS（`@media (max-width:820px)` 的显式 `order` 链）：`#openZCodeBtn{order:9}` 插入 kimi(8) 之后，`#openDshBtn` 及后续元素 order 顺移 +1（v1.8.7 同款处理，避免窄屏下新按钮以默认 order:0 掉到头部最前）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.39 → v1.8.42（v1.8.40/v1.8.41 由其它会话基于更新 Tony 编译刷车使用，未回本仓库，故跳号对齐车上实际版本之后）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_web_console_header_entry_buttons` 位置链断言改为 `h1 < donkey < drifter < kimi < zcode < dsh < gh`，新增 `openZCodeBtn` 按钮/`openZCode()` 函数/`/api/launch/zcode` 路径/15000 超时/i18n 四键中英文案/lucide Code2 图标路径等断言块，`class="navTabWeak"` 计数 2 → 3；`test_web_console_mobile_header_layout` 同步 order 顺移断言；版本与 CHANGELOG 顺序断言升至 v1.8.42。
  - `arduino-cli.py -c` 编译通过，OTA 刷车验证生效（api/status version=v1.8.42，首页 HTML 含 zcode 入口）。

## 2026-08-22 v1.8.41

- feat(WebConsole): `?settings=1` / `?wifi=1` 内嵌视图新增 RC Channels 校准面板（Issue #234），DD Car Connector 内嵌视图可同屏调整 RC 校准
  - 背景：DD Car Connector 的车辆设置内嵌视图（iframe `?embedded=1&settings=1&wifi=1`）此前只有「调校 + AP 名称配置 + STA Wi-Fi 配置」；用户要求把 Drifter Console 的 RC Channel 诊断面板（含舵机/油门中点 Set 按钮、油门 Min/Max 滑块等校准控件）也搬进内嵌视图，主题/静音/语言、OTA、DEV 仍不搬。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - CSS：`body.settings .grid{display:none}` / `body.wifi .grid{display:none}` 整藏规则改为选择性显示——`body.settings .grid,body.wifi .grid{display:block}`；`... .grid>section.panel{display:none}` 藏全部面板 section；`... #diagnosticsPanel{display:block}`（ID 高特异性）只留 Diagnostics 面板；`... #diagnosticsPanel>div{display:none}` 藏其直接子 div；`... #diagnosticsPanel #rcFold{display:block}`（双 ID 最高特异性）只露出 `#rcFold`（RC Channels），`#statusFold`（STATUS Details，纯显示）、手柄校准按钮行、`#joystickCalStatus` 仍隐藏。另补 `body.settings #serialPanel,body.wifi #serialPanel{display:none}`——基础规则 `#serialPanel{display:flex}`（ID 特异性 (1,0,0) 高于 `.grid>section.panel` 的 (0,3,2)）会穿透整藏把终端面板顶出来，须按 ID 单独藏（无头浏览器实测发现）。
    - HTML：`#settingsView` 板块（车辆设置标题 + 调校行）由 `.grid` 之后移到 `.grid` 之前，内嵌视图顺序变为「车辆设置/调校 → RC Channels → AP 名称配置 → STA Wi-Fi 配置」；主控台页面 settingsView 默认 `display:none`，移动对主页面零影响。调校行「漂移设置」按钮沿用 v1.8.36 的 `location.href='/drift?theme='+resolvedTheme()`（rebase 时保留主题跟随改动）。
    - JS init：新增 `if(...settings=1...||...wifi=1...)toggleFold('rcFold')`——内嵌视图加载时自动展开 RC Channels 折叠面板，校准控件直接可见（主控台页面无参数、不受影响）。
    - rcFold 留在 `.grid` 内不移位，`updateState` 按 id 实时更新通道值/中点值逻辑零改动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.38 → v1.8.41（v1.8.37/v1.8.38 均被并行会话条目顺移占用，跳号避开当日多会话版本碰撞；v1.8.40 为本功能中间构建，未合入）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本与 CHANGELOG 顺序断言升至 v1.8.41；新增 `test_web_console_settings_view_shows_rc_channels_panel` 断言上述 CSS 规则（含 `#serialPanel` 按 ID 整藏）、init 自动展开与 settingsView 前移位置。
  - `arduino-cli.py -c` 编译通过，OTA 刷车验证 v1.8.41 生效（无头浏览器实测内嵌视图：settingsView/rcFold/两个配网板块可见，statusFold/chartPanel/serialPanel 隐藏，RC 通道值实时刷新）。
- style(WebConsole): `?wifi=1` 内嵌视图中「AP 名称配置」与「STA Wi-Fi 配置」两个板块融合为单卡片（Issue #234 后续）
  - 背景：DD Car Connector 内嵌视图里 AP/STA 两个配网板块是两张独立卡片（14px 圆角 + 黄边 + 阴影，中间 14px 间隔），用户要求放在一起、融合成一个板块。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（纯 CSS，仅 `body.wifi` 作用域，主控台页面的配网弹窗不受影响）：AP 卡 `margin:0;border-bottom:none;border-radius:14px 14px 0 0;padding-bottom:8px;box-shadow:none`；STA 卡 `margin:0 0 14px;border-radius:0 0 14px 14px;padding-top:4px`——两卡上下拼接成一张 14px 圆角单卡，两个 h2（AP 名称配置 / STA Wi-Fi 配置）保留为卡内分节标题。
  - `libraries/mus4_core/src/BuildInfo.h`：分支内曾记为 v1.8.41 → v1.8.42；合入 Tony 时 v1.8.42 已由 ZCode 入口条目（上方）占用，本改动作为 Issue #234 同一提交的一部分并入本 v1.8.41 条目，版本不再变动。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本与 CHANGELOG 顺序断言保持 v1.8.42（ZCode）> v1.8.41（本条目）链；`test_web_console_settings_view_shows_rc_channels_panel` 增加融合 CSS 断言。
  - `arduino-cli.py -c` 编译通过，OTA 刷车 + 无头浏览器截图实测融合效果。

## 2026-08-22 v1.8.39

- fix(WebConsole): 删除漂移调参页（/drift）自带的深浅色切换按钮——该页应完全跟随 Drifter Console 主题（经 `?theme=` 参数传递），不应有自己的切换开关
  - 背景：漂移页 v1.8.36 加了与控制台同款的太阳/月亮切换按钮，但该页主题是控制台三处入口以 `?theme=` 参数带过来的，页内再放一个切换键会让两边状态脱节，用户要求删掉、完全跟随控制台。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：删除漂移页 `#themeToggle` 按钮（含太阳/月亮 SVG）、JS `toggleTheme()`/`setTheme()`、zh/en `drift.theme.title` i18n 键、`.themeButton` 及 `html[data-theme="light"] .themeButton*` 全部 CSS 规则；保留防闪烁脚本、`?theme=` 优先 + 系统兜底的 `initTheme()`、整套浅色覆盖与标题悬停灰字提示；控制台主页的切换按钮不受影响。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.38 → v1.8.39。
  - 测试同步：`tests/test_firmware_feature_flags.py` `test_drift_page_theme_and_title_hints` 主题按钮断言改为否定式（`themeToggle`/`toggleTheme`/`setTheme`/`drift.theme.title` 均 `not in page`）；`test_web_console_theme_toggle` 注释更新（控制台按钮断言保留）；版本与 CHANGELOG 顺序断言升至 v1.8.39。
  - `arduino-cli.py -c` 编译通过，OTA 刷车验证生效。

## 2026-08-21 v1.8.38

- style(WebConsole): 遥测曲线 Y 轴标签字体由 `bold 11px sans-serif` 改为 `12px sans-serif`（去掉粗体，与图例标签 `.legend span{font-size:12px}` 一致）
  - 背景：用户反馈 Y 轴数字（1 / 0.75 / 0.5 等）字体偏粗，要求改为与右下角 Steering / GyroZ 标签相同的字体风格。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`draw()` 函数中 `ctx.font='bold 11px sans-serif'` → `ctx.font='12px sans-serif'`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.37 → v1.8.38。
  - 注：本条目原为本地 Tony 上的 v1.8.37，Tony 本地/远端分叉 reconciled 合并时顺移至 v1.8.38（v1.8.36 由远端漂移页主题条目占用）。
  - `arduino-cli.py -c` 编译通过，OTA 刷车验证生效。

## 2026-08-21 v1.8.37

- fix(WebConsole): 修复 Diagnostics 面板内 rcFold foldBody 末尾多余 `</div>` 导致浏览器提前关闭 `.grid` 容器，使手柄校准按钮、方向状态文本、STATUS Details 折叠面板被移到 `<body>` 下而非 `#diagnosticsPanel` 内部，造成左侧未与 Mode 卡片和遥测曲线对齐
  - 根因：`libraries/mus4_web/src/WebConsoleAssets.h` 第 71 行末尾有 6 个 `</div>` 但只需 5 个（内层 flex / rcCell / 外层 flex / foldBody / rcFold），多出的 `</div>` 被 HTML 解析器用于关闭 `.grid` 容器，后续元素溢出到 `<body>`。
  - 修复：删除末尾多余的 1 个 `</div>`（`</div></div></div></div></div></div>` → `</div></div></div></div></div>`）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.36 → v1.8.37。
  - 注：本条目原为本地 Tony 上的 v1.8.36，Tony 本地/远端分叉 reconciled 合并时顺移至 v1.8.37（v1.8.36 由远端漂移页主题条目占用）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本与 CHANGELOG 顺序断言升至 v1.8.37。
  - `arduino-cli.py -c` 编译通过，OTA 刷车验证生效。

## 2026-08-21 v1.8.36

- feat(WebConsole): 漂移调参页（/drift）跟随 Drifter Console 深浅色主题 + 全部标题改为悬停灰字提示 + 删除「返回 Drifter Console」链接
  - 背景：漂移调参页此前仅深色硬编码，与控制台深浅色不一致；页头有「返回 Drifter Console」链接用户要求删掉；标题下的描述文字（Status/Steering Correction/Throttle Strategy 及 h1 版本小字）常显占空间，用户要求参考 DonkeyDrifter 的小标题样式——光标悬停时灰字从标题右侧滑出。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主题：`<head>` 加防闪烁脚本（优先读 `?theme=light|dark` URL 参数，缺省按系统 `prefers-color-scheme`）；新增整套 `html[data-theme="light"]` 浅色覆盖（body/panel/summaryItem/field/input/button/a/muted/hintSpan/themeButton）；新增主题切换按钮（`#themeToggle` 太阳/月亮 SVG，与控制台同款样式）；JS 新增 `readUrlTheme/systemTheme/resolvedTheme/applyTheme/toggleTheme/setTheme/initTheme`（内存态、不写 localStorage，与控制台 v1.8.27 决策一致），`initTheme()` 加入初始化链。
    - 跟随控制台：控制台三处 /drift 入口携带当前主题——Drift 卡 Tune 链接加 `id="driftTuneLink"`，`applyTheme()` 内同步其 `href='/drift?theme='+resolvedTheme()`；Diagnostics 漂移设置按钮 `window.open('/drift?theme='+resolvedTheme(),'_blank')`；设置视图漂移设置按钮 `location.href='/drift?theme='+resolvedTheme()`。
    - 标题悬停提示：新增 `.titleHint`（inline-flex 容器）+ `.hintSpan`（`max-width:0;opacity:0` 收起，`.titleHint:hover` 时 `max-width:340px;opacity:1;margin-left:12px` 滑出，`transition:all .3s ease-in-out`）样式；h1+版本小字、Status、Steering Correction、Throttle Strategy 共 4 处标题改造，原常显描述文字移入 hintSpan。
    - 删除 `<a href="/" data-i18n="drift.backLink">` 返回链接及 zh/en `drift.backLink` 键；新增 `drift.theme.title` zh「主题」/ en「Theme」。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.35 → v1.8.36。
  - 测试同步：`tests/test_firmware_feature_flags.py`——Tune 链接断言加 `id="driftTuneLink"`；`test_drift_settings_button_next_to_joystick_calibration` 断言改为携带主题参数的 `window.open('/drift?theme='+resolvedTheme(),'_blank')`；新增 `test_drift_page_theme_and_title_hints`（防闪烁脚本/浅色覆盖/切换按钮/内存态/三处入口主题参数/4 处 titleHint+hintSpan/backLink 删除断言）；版本与 CHANGELOG 顺序断言升至 v1.8.36。

## 2026-08-21 v1.8.35

- style(WebConsole): 删除 Drifter Console 主控台页面 4 个面板组的外框（border / background / border-radius），保留内含子元素各自样式
  - 背景：用户反馈 Drifter Console 页面中 4 个 `.panel` 面板组（状态卡片 Mode/Park/Drift/Voltage/Network、遥测曲线 chartPanel、终端 serialPanel、RC Channels diagnosticsPanel）外面的边框框线使界面冗余，要求删掉外框、保留内部卡片/曲线/终端/RC 单元格各自的样式。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 深色主题 `.panel` 规则：`background:#171c24;border:1px solid #2b3441;border-radius:8px;padding:10px` → `background:transparent;border:none;border-radius:0;padding:10px`（保留 padding 提供内容呼吸空间，`.grid` 的 `gap:10px` 提供面板间距）。
    - 浅色主题覆盖 `html[data-theme="light"] .panel`：`background:#fff;border-color:#d5dce4` → `background:transparent;border:none`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.34 → v1.8.35。
  - 不影响范围：`/drift`（Drift Assist Tuning）和 `/judge`（Drift Judge）页面有各自独立的 `.panel` CSS，不受影响；面板内部 `.stateCard`、`.rcCell`、`.foldHead`、chart canvas、`#terminalWrap` 等保留各自边框/背景。
  - `arduino-cli.py -c` 编译通过，OTA 刷车验证 v1.8.35 生效。

## 2026-08-21 v1.8.34

- fix(WebConsole): Diagnostics 面板「漂移设置」按钮硬编码过时 IP `http://192.168.3.150/drift` 导致点击后页面打不开——改为相对路径 `window.open('/drift','_blank')`，跳转到 ESP32 自身的 `/drift` 页面
  - 背景：v1.8.28 新增 `?settings=1` 设置视图时，Settings 视图的漂移设置按钮已正确使用 `location.href='/drift'` 跳转到 ESP32 自身页面；但 Diagnostics 面板（主视图）的同名按钮仍沿用 v1.7.56 时代的硬编码上位机 IP `192.168.3.150`，该 IP 已过时且上位机无 `/drift` 路由，两辆车点击后均打不开页面。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：第 64 行 `onclick="window.open('http://192.168.3.150/drift','_blank')"` → `onclick="window.open('/drift','_blank')"`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.33 → v1.8.34。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_drift_settings_button_next_to_joystick_calibration` 的 docstring 与断言由 `http://192.168.3.150/drift` 改为 `window.open('/drift','_blank')`；版本一致性与 CHANGELOG 顺序断言升至 v1.8.34。

## 2026-08-21 v1.8.33

- feat(WebConsole): 新增 `?wifi=1` 内嵌配网板块视图——把 STA/AP 配网弹窗以静态板块形式直接呈现，供 DD Car Connector 1:1 内嵌（Issue #234）
  - 背景：DD 的 Car Connector 此前只在顶部放「STA 配置 / AP 名称」两个按钮，点按经 postMessage 打开车端配网弹窗；用户希望像 Drifter Console 那样把完整配网板块（SSID 输入 / 扫描 / 密码 / 上位机配网 / 历史 / AP 前缀预览）直接铺在页面里，而不是一个按键。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - CSS 新增 `body.wifi` 规则——隐藏 `.grid` / `.headerRow` / `.fabToggle` / `.fabActions`；把 `#wifiApModal`、`#wifiStaModal` 由 `position:fixed` 遮罩弹窗改为 `position:static;display:block;background:transparent` 的静态板块，`.dialog` 改 `width:100%;max-width:640px;margin:0 0 14px`；隐藏两个配网弹窗的首个「取消」按钮（板块模式下无需关闭）。
    - JS init 增加 `if(location.search.indexOf('wifi=1')>=0){document.body.classList.add('wifi');openWifiApModal();openWifiStaModal();}`——加载时自动 open 两个配网表单并填充字段（复用既有 refresh 逻辑，1:1 车端表单）。
    - `?wifi=1` 与 `?settings=1` 可叠加：DD 侧 iframe 用 `?embedded=1&settings=1&wifi=1` 同屏显示「调校（漂移 / Judge / 手柄校准）」+「AP 名称配置」+「STA Wi-Fi 配置」三个板块；DEV / OTA 仍不在该视图内（顶栏 `body.embedded` 隐藏、settingsView 无 system 行）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.32 → v1.8.33。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本与 CHANGELOG 顺序断言升至 v1.8.33；`python3 -m pytest tests/ -q` 全绿。
  - 注：配套 DD 侧 CarSettingsPanel iframe 改用 `&wifi=1`、删除 postMessage 按钮在 DonkeyDrift 仓库同日条目；收尾后 OTA 刷车验证。

## 2026-08-21 v1.8.32

- fix(WebConsole): 移除 DC 头部「C Code」入口按钮及其全部配套代码（JS / i18n / 移动端 order / 测试断言），恢复为 Kimi Code Web + DeepSeek Harness 两个弱化入口
  - 背景：C Code 入口（v1.8.31 引入）经实际使用后用户决定移除；Claude Code 无官方 web UI，复用 launcher 网页终端的方案体验不佳，遂删除。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：删除 `#openCCodeBtn` 按钮 HTML（含 lucide Terminal 图标）、`openCCode()` JS 函数（含 `cCodeLaunching` 防重入标志）、zh/en 各 4 个 i18n 键（`button.openCCode` / `button.openCCodeLaunching` / `toast.cCodeFailed` / `toast.cCodeTimeout`）；移动端 `@media(max-width:820px)` order 链删除 `#openCCodeBtn{order:9}` 并将后续元素 order 各减 1（`#openDshBtn` 10→9、`.br2` 11→10、`.headerRow .otaLink` 13→12、`#muteToggle` 14→13、`#devModeToggle` 15→14、`.br3` 16→15、`#themeToggle` 17→16、`#langToggle` 18→17）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.31 → v1.8.32。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_web_console_header_entry_buttons` 位置链断言改回 `h1 < donkey < drifter < kimi < dsh < gh`（删除 `ccode_pos`），删除 C Code 按钮/函数/路径/超时/i18n 断言块，`class="navTabWeak"` 计数 3 → 2，删除 Terminal 图标 `M12 19h8` 断言；`test_web_console_mobile_header_layout` order 断言同步还原；版本与 CHANGELOG 顺序断言升至 v1.8.32。两文件 165 项单测全部通过，`arduino-cli.py -c` 编译通过。
  - 注：配套 DD 侧入口移除在 DonkeyDrift 仓库同日条目（C Code 入口移除）；收尾后 OTA 刷车验证。

## 2026-08-21 v1.8.31

- feat(WebConsole): DC 头部在 Kimi Code Web 与 DeepSeek Harness 之间新增「C Code」入口按钮——点击经 launcher 新端点 `POST /api/launch/claude-code` 拿到网页终端 URL（`/terminal?cmd=cd <工作区> && claude`），在新标签页的网页终端里运行 Claude Code
  - 背景：DD 标签栏与 DC 头部已有 Kimi Code Web / DeepSeek Harness 两个弱化入口，用户要求在其间加入 C Code（Claude Code）；Claude Code 无官方 web UI，故复用 launcher 的 `/terminal?cmd=` 网页终端机制（菜单 8/9/10 同款），入口即开即得、端点毫秒级返回，无需冷启动等待。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 头部按钮：`openKimiCodeWebBtn` 与 `openDshBtn` 之间插入 `openCCodeBtn`（`class="navTabWeak"`，lucide Terminal 内联 SVG 图标 14×14 `stroke="currentColor"`，与邻居同款；`<span data-i18n="button.openCCode">C Code</span>`）。
    - JS：`openKimiCodeWeb()` 与 `openDsh()` 之间新增 `openCCode()`，逐行镜像 `openDsh()`（防重入标志 `cCodeLaunching` → `window.open('about:blank')` 占住新标签 → 禁用按钮切 Launching 文案 → AbortController + fetch POST `http://<launcherIp>:8090/api/launch/claude-code` → 成功 `newTab.location.href=j.url` → 失败关标签 + showToast → finally 恢复）；超时 15s（端点无子进程、即时返回，不同于 kimi 的 120s 冷启动）。
    - i18n：zh/en 各在 kimiCodeWeb 组与 dsh 组之间新增 `button.openCCode` / `button.openCCodeLaunching` / `toast.cCodeFailed` / `toast.cCodeTimeout` 四键（措辞镜像 dsh 组）。
    - 移动端 CSS（`@media (max-width:820px)` 的显式 `order` 链）：`#openCCodeBtn{order:9}` 插入 kimi(8) 之后，`#openDshBtn` 及后续元素 order 顺移 +1（v1.8.7 加 dsh 时的同款处理，避免窄屏下新按钮以默认 order:0 掉到头部最前）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.30 → v1.8.31（避让并行会话已合入的 v1.8.30）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`test_web_console_header_entry_buttons` 位置链断言改为 `h1 < donkey < drifter < kimi < ccode < dsh < gh`，新增 `openCCodeBtn` 按钮/`openCCode()` 函数/`/api/launch/claude-code` 路径/15000 超时/i18n 四键中英文案/lucide Terminal 图标路径（`M12 19h8`）等断言块，`class="navTabWeak"` 计数 2 → 3；`test_web_console_mobile_header_layout` 同步 order 顺移断言；版本与 CHANGELOG 顺序断言升至 v1.8.31。两文件 165 项单测全部通过，`arduino-cli.py -c` 编译通过。
  - 注：配套 launcher 端点与 DD 侧按钮在 DonkeyDrift 仓库同日条目（C Code 入口）；收尾后 OTA 刷车验证。

## 2026-08-21 v1.8.30
- fix(WebConsole): Car Connector 内嵌 DC 的 `?settings=1` 设置视图删掉「系统（OTA/开发模式）」与「Wi-Fi 配网」两行，改为由 DonkeyDrifter 侧把「连接/配网」融合成一个板块
  - 背景：用户反馈 Car Connector 里嵌入的 DC 设置视图，「系统」行（OTA + 开发模式）太突兀、不该占一整行；「配网」则与 DD 侧 Car Connector 顶部的「连接（设备发现/选择）」在功能上重复，应融合成一个板块。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `#settingsView` 删除「Wi-Fi 配网」setRow 与「系统」setRow，只保留「调校」（漂移设置 / Judge 设置 / 手柄校准）。
    - `window.addEventListener('message',…)` 新增 `dd-open-wifi-sta` → `openWifiStaModal()`、`dd-open-wifi-ap` → `openWifiApModal()`，供 DD 侧顶部配网按钮经 postMessage 打开车端 STA/AP 配置弹窗（弹窗仍渲染在 iframe 内，1:1 车端 UI）。
    - `renderDevMode` 删除已失效的 `#devModeToggleSettings` 同步（该元素随「系统」行一并删除）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.29 → v1.8.30。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本断言升至 v1.8.30，并新增 v1.8.30 在 v1.8.29 之前的顺序断言。

## 2026-08-21 v1.8.29

- fix(WebConsole): 终端全屏后四周白边——`#terminalWrap` 全屏态去掉 `padding`/`border-radius` 并统一深色背景，删除亮色主题的全屏浅色背景覆盖，使终端 iframe 全屏铺满、无白边
  - 背景：终端全屏时 `#terminalWrap` 仍带 `padding:8px`，且亮色主题下 `html[data-theme="light"] #terminalWrap:fullscreen` 背景为 `#eef1f5`（近白），把深色终端 iframe 四周衬成白边。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `#terminalWrap:fullscreen` 由 `{background:#101318;height:auto;min-height:0;max-height:none}` 改为 `{background:#101318;height:auto;min-height:0;max-height:none;padding:0;border-radius:0}`，全屏铺满、无内边距与圆角。
    - 删除 `html[data-theme="light"] #terminalWrap:fullscreen{background:#eef1f5}` 覆盖规则，全屏统一走深色背景（终端本身为深色主题）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.28 → v1.8.29。
  - 测试同步：`tests/test_firmware_feature_flags.py`——终端全屏断言改为「`#terminalWrap:fullscreen` 含 `padding:0;border-radius:0`；不再存在 `html[data-theme="light"] #terminalWrap:fullscreen`」；版本与 CHANGELOG 顺序断言升至 v1.8.29。

## 2026-08-21 v1.8.28

- feat(WebConsole): DC 新增「仅设置」视图（`?settings=1`），供 DonkeyDrifter 的 Car Connector 页面只嵌入车辆设置板块，而非整个 DC 主页
  - 背景：Car Connector 之前用 iframe 嵌入了整个 DC 主页（Mode/Park/Drift/电池等显示卡全带进来），用户指出这些是「显示」而非「设置」；正确需求是只把设置类板块（WiFi 配网、OTA、开发模式、漂移设置、Judge、手柄校准）放进 Car Connector。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `<style>` 新增 `.settingsView` 与 `body.settings` 相关样式：默认隐藏 `#settingsView`，`body.settings` 时显示 `#settingsView` 并隐藏 `.grid` / `.headerRow` / `.fabToggle` / `.fabActions`。
    - `.grid` 闭合后新增 `<div id="settingsView" class="settingsView">`：标题「车辆设置」+ 三组 setRow——WiFi（STA Wi-Fi 配置 / AP 名称配置）、系统（OTA / 开发模式开关 `#devModeToggleSettings`）、调校（漂移设置 / Judge 设置 / 手柄校准）。
    - URL 检测：`location.search` 含 `settings=1` 时 `document.body.classList.add('settings')`。
    - `renderDevMode` 同步更新 `#devModeToggleSettings` 的开/关态与 aria-checked。
    - i18n 新增 `settings.title` / `settings.wifi` / `settings.system` / `settings.tuning` / `settings.judge` / `settings.dev`（zh/en）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.27 → v1.8.28。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本断言升至 v1.8.28，并新增 v1.8.28 在 v1.8.27 之前的顺序断言。

## 2026-08-20 v1.8.27

- fix(WebConsole): DC 深浅色手动切换改为仅内存态、不写 localStorage——修复「手动切换后刷新仍保持所选主题，无法重新跟随系统」的问题，使 DC 与 Donkey / DonkeyDrifter 三页一致：默认跟随系统、每次进入/刷新都重新按浏览器 prefers-color-scheme 解析
  - 背景：DC 原先把主题选择持久化到 `localStorage['mus4.ui.theme']`，手动点过太阳/月亮后刷新仍保持所选主题、不再跟随系统；Donkey 与 DonkeyDrifter 已改为不持久化，DC 需对齐。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 删除 `THEME_STORAGE_KEY`（`mus4.ui.theme`）常量与 `readStoredTheme()` / `writeStoredTheme()` 两个函数。
    - `setTheme(theme)` 由 `uiTheme=theme;writeStoredTheme(uiTheme);applyTheme()` 改为 `uiTheme=theme;applyTheme()`（只改内存态）。
    - `initTheme()` 由 `uiTheme=readStoredTheme();applyTheme();…` 改为 `uiTheme='auto';applyTheme();…`（默认跟随系统，不读存储）。
    - 头部防闪烁内联脚本由「按 localStorage 预置 data-theme」改为「直接按 matchMedia 预置 data-theme」，不读任何存储。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.25 → v1.8.27（避让并行会话已占用的 v1.8.26）。
  - 测试同步：`tests/test_firmware_feature_flags.py`——主题断言改为「无 THEME_STORAGE_KEY / readStoredTheme / writeStoredTheme / localStorage 读取，setTheme/initTheme 精确为内存态写法，防闪烁脚本为新 matchMedia 版本」；版本与 CHANGELOG 顺序断言升至 v1.8.27（跳号 v1.8.26），并补上 v1.8.25 条目（修正 v1.8.25 提交漏改版本断言的既有问题）。

## 2026-08-20 v1.8.26

- feat(WebConsole): DC 内嵌于 DonkeyDrifter 时经 postMessage 即时同步 DD 顶栏静音键——配合 DD 侧 `ConsoleMuteButton` 切换成功后广播 `dd-console-mute-changed` 事件，实现「在 DD 上改静音、内嵌 DC 立马变」，无需等 5s 轮询或手动刷新
  - 背景：此前静音虽已双向同步（Issue #117），但靠两边各自每 5s 轮询 `/api/mute`，DD 切换后内嵌 DC 最迟 5s 才更新；本次改为 DD 切换成功即广播 DOM 事件 → `DrifterConsolePage` 对 iframe `contentWindow.postMessage({type:'dd-console-mute-changed',muted:<bool>})` → DC 直接更新图标，不重载 iframe、不丢曲线/终端状态（静音是高频轻量操作，重载体验差）。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`toggleMute` 后新增 `window.addEventListener('message',function(e){…d.type==='dd-console-mute-changed'…uiMuted=!!d.muted;renderMuteButton()})`，识别 DD 转发来的静音消息即时更新；保留 `setInterval(initMute,5000)` 作为兜底纠偏。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.25 → v1.8.26。
  - 测试同步：`tests/test_firmware_feature_flags.py` 静音按钮 UI 测试新增 `dd-console-mute-changed` 与 `message` 监听断言；版本一致性测试由 v1.8.24 修正到 v1.8.26 并补上 v1.8.25 条目断言（顺带修复 v1.8.25 lang-sync 合入时遗漏的版本测试更新）。

## 2026-08-20 v1.8.25

- feat(WebConsole): DC 内嵌在 DonkeyDrifter 时经 iframe src 的 `?lang=` 跟随 DD 语言——修复「DD 已切英文、内嵌 Drifter Console 仍是中文」的跨源语言不同步问题
  - 背景：DD（:8000）顶栏切换语言只写 DD 自己 origin 的 `localStorage`，内嵌 DC（车端 :80）的 `initLanguage` 读的是车端 `/api/language` + 自己 origin 的 `localStorage`，两边各自独立，DD 切语言不会传导到内嵌 DC。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：新增 `readUrlLanguage()`（解析 `?lang=zh|en`），`initLanguage` 改为 `let lang=readUrlLanguage();if(!lang){…fetch /api/language…}`，即 `?lang=` 优先级最高、无参数时再走车端 `/api/language`/localStorage；console / drift / judge / ota 四处内嵌页的 `initLanguage` 同步修改。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.24 → v1.8.25。
  - 测试同步：`tests/test_firmware_feature_flags.py` 新增 `test_web_console_reads_dd_lang_url_param`（断言 `readUrlLanguage`/`window.location.search`/`lang=(zh|en)`/`let lang=readUrlLanguage()`）；全量 326 passed。

## 2026-08-20 v1.8.24

- style(WebConsole): DC 头部 OTA 按钮与 DEV 开关复刻 DonkeyDrifter 顶栏 `ConsoleOtaButton` / `ConsoleDevToggle`——把上一版恢复的「OTA 文字链接 + DEV 滑珠开关」统一改成 DD 同款文字胶囊（32px 高 / 12px 内边距 / 圆角全胶囊 / 深色 `#111820` 底 + `#344154` 边框 + inset 内圈 + `#b9c5d3` 字色），DEV 开启态用 `rgba(92,200,255,.25)` 青底 + `#5cc8ff` 边框/内圈/字色三层高亮，与 DD 完全一致
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - headerRow 中 `<a href="/update" class="otaLink"><button class="otaButton">OTA</button></a>` 改为 `<a href="/update" class="otaLink" data-i18n="button.ota">OTA</a>`（去掉非法嵌套的 button，链接直接作为文字胶囊）；`<label class="toggleSwitch devHint" id="devModeToggle"><input id="devModeCheck" ...><span class="slider"></span></label>` 改为 `<button type="button" id="devModeToggle" class="devHint" onclick="toggleDevModeFromSwitch()" role="switch" aria-checked="false">DEV</button>`。
    - 深色 CSS：`.otaLink` 由透明文字链接改为胶囊（`display:inline-flex;height:32px;padding:0 12px;border-radius:9999px;background:#111820;border:1px solid #344154;box-shadow:inset 0 0 0 1px #2b3441;color:#b9c5d3;font-size:12px;font-weight:600`，hover 转 `#5cc8ff`）；新增 `#devModeToggle` 同款胶囊 + `#devModeToggle.devOn` 青底三层高亮；删除 `.otaButton` 与 `#devModeToggle .slider`/`input:checked+.slider` 系列滑珠规则。
    - 浅色 CSS：`.otaLink` / `#devModeToggle` 用 `#f4f6f9` 底 + `#ccd5df` 边框 + inset `#d5dce4` + `#3f4f63` 字色（hover `#0c9bd6`），`devOn` 态与深色一致用 `#5cc8ff`。
    - JS：新增 `let uiDevMode=false`；`renderDevMode` 改为 `classList.toggle('devOn')` + 同步 `aria-checked`；`toggleDevModeFromSwitch` 改按 `uiDevMode` 判断（关→直接 `setDevMode(false)`、开→弹确认）；移除死代码 `devModeCheck` 常量与 `requestDevModeToggle`；init 增加 `setInterval(refreshDevMode,5000)` 与 DD 每 5s 轮询同步 DEV 状态。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.23 → v1.8.24。
  - 测试同步：`tests/test_firmware_feature_flags.py`——DEV 断言改为文字胶囊结构（`role="switch"`/`devOn`/`uiDevMode`），`.otaButton`/`devModeCheck`/`requestDevModeToggle` 改为 `not in`，OTA/DEV 头部/浅色断言改为 `.otaLink`/`#devModeToggle` 胶囊规则，版本与 CHANGELOG 顺序断言升至 v1.8.24。

## 2026-08-20 v1.8.23

- fix(WebConsole): 恢复 DC 头部 OTA 按钮与 DEV 开关——上一版（v1.8.17）将 Donkey/OTA/DEV 移至 DonkeyDrifter 顶栏时误移走了用户仍需在 DC 头部直接操作的 OTA 与 DEV，现把两者加回（Issue #108 续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - headerRow 语言按钮后恢复 `<span class="rowBreak br2"></span><a href="/update" class="otaLink"><button class="otaButton" data-i18n="button.ota">OTA</button></a><label class="toggleSwitch devHint" id="devModeToggle"><input type="checkbox" id="devModeCheck" onchange="toggleDevModeFromSwitch()"><span class="slider"></span></label><span class="rowBreak br3"></span>`。
    - 恢复 DEV 确认弹窗 `#devModeModal`、DEV 相关 JS（`devModeCheck`/`devModeModal` 引用与 `renderDevMode`/`toggleDevModeFromSwitch`/`refreshDevMode`/`requestDevModeToggle`/`closeDevModeModal`/`setDevMode` 函数）、init 里的 `refreshDevMode()`。
    - 恢复 OTA/DEV 深浅两主题 CSS：`.otaLink`/`.otaButton`/`#devModeToggle` 及移动端 order（`.br2{order:10}`、`.headerRow .otaLink{order:12}`、`#devModeToggle{order:14;margin-left:auto}`、`.br3{order:15}`）。
    - `_applyLauncherStatus` 恢复 `enterDonkeyBtn` 动态 href 改写（与 Donkey 入口配套）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.22 → v1.8.23。
  - 测试同步：`tests/test_firmware_feature_flags.py`——OTA/DEV 头部断言由 `not in` 改回 `in`，移动端布局恢复 4 行（br2/br3/OTA/DEV order 断言），浅色 otaButton 断言恢复，版本与 CHANGELOG 顺序断言升至 v1.8.23。

## 2026-08-20 v1.8.22

- style(WebConsole): DC 顶栏静音键静音激活态复刻 DonkeyDrifter 顶栏 `ConsoleMuteButton`——激活时改用 `rgba(92,200,255,.1)` 半透明青底 + `#5cc8ff` 边框/inset 内圈/图标字色（深浅两主题一致，浅色不再用 `#0c9bd6`），与 DD 静音键视觉完全一致
  - `libraries/mus4_web/src/WebConsoleAssets.h`：深色 `.muteButton.muted` 由仅改字色 `#5cc8ff` 扩为 `background:rgba(92,200,255,.1);border-color:#5cc8ff;box-shadow:inset 0 0 0 1px #5cc8ff;color:#5cc8ff`；浅色 `html[data-theme="light"] .muteButton.muted` 同步改为同款 `#5cc8ff` 三层高亮（原 `#0c9bd6`）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.20 → v1.8.22。
  - 测试同步：`tests/test_firmware_feature_flags.py`——深浅两套 `.muteButton.muted` 断言更新为新三层高亮写法，版本与 CHANGELOG 顺序断言升至 v1.8.22。

## 2026-08-19 v1.8.20

- fix(WebConsole): 恢复 Drifter Console 主页面 header 行显示——上一版 v1.8.19 误把车端 DC 标题栏整行隐藏，现改为仅在 DD 嵌入（URL 带 `?embedded=1`）时经 `body.embedded` 隐藏，直接访问车端 DC 时标题栏照常显示（Issue #234）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主 DC 页 `.headerRow` 由 `display:none` 恢复为 `display:flex`。
    - 新增 CSS 规则 `body.embedded .headerRow{display:none}`。
    - 主 DC 页脚本初始化前加 `if(location.search.indexOf('embedded=1')>=0)document.body.classList.add('embedded')`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.19 → v1.8.20。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本断言升至 v1.8.20、顺序断言补 v1.8.20；header 断言恢复 `display:flex` 并新增 `body.embedded .headerRow{display:none}` 断言。
  - 注：DD 侧 iframe 改用 `http://<ip>/?embedded=1` 加载，配套改动见 DonkeyDrift 仓库当日条目。

## 2026-08-19 v1.8.19

- style(WebConsole): 隐藏 Drifter Console 主页面 header 行（头像/标题/GitHub 图标/深浅色开关/OTA/DEV 开关整行不再显示），版本号改由 DonkeyDrifter 连接条「连接」按钮右侧显示（Issue #234）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：主 DC 页（`/`）`.headerRow` 由 `display:flex;align-items:center` 改为 `display:none`，视觉删除但保留 DOM 供 JS 引用（`versionLabel`/`enterDonkeyBtn`/静音/OTA/DEV 等元素仍被 getElementById 引用）；`/drift` 页 `.headerRow` 保持原样。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.18 → v1.8.19。
  - 测试同步：`tests/test_firmware_feature_flags.py`——版本断言升至 v1.8.19，CHANGELOG 顺序断言补 `v1.8.19 < v1.8.18`。
  - 注：DD 侧配套改动见 DonkeyDrift 仓库当日条目（Drifter Console 连接条「连接」按钮右侧显示车端固件版本号）。

## 2026-08-19 v1.8.18

- fix(WebConsole): 恢复 DC 顶栏 Donkey 入口——上一版（v1.8.17）将 Donkey/OTA/DEV 移至 DonkeyDrifter 顶栏时误移走了用户仍需的 Donkey 快捷入口，现把 Donkey 加回 DonkeyDrifter 左侧（Issue #108 续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - headerRow 在 `<h1>` 后、DonkeyDrifter 前恢复 `<a class="navTab" data-i18n="button.enterDonkey" id="enterDonkeyBtn" href="http://192.168.3.41:8090/" target="_blank" rel="noopener">Donkey</a>`。
    - 移动端 `@media (max-width:820px)` 恢复 `#enterDonkeyBtn{order:6}`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.17 → v1.8.18。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`enterDonkeyBtn` 与 `.navTab` 数量断言恢复为存在/2，移动端 order 断言恢复，版本断言升至 v1.8.18 且顺序断言补 v1.8.18。

## 2026-08-19 v1.8.17

- feat(WebConsole): DC 头部 Donkey / OTA / DEV 三控件移至 DonkeyDrifter 顶栏，DC 侧只保留导航入口与状态控件，静音键补 5s 轮询实现与 DD 双向同步
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - headerRow 移除「Donkey」入口 `enterDonkeyBtn`（DonkeyDrifter / Kimi Code Web / DeepSeek Harness 保留）、OTA 链接 `.otaLink`（`/update`）、DEV 开关 `#devModeToggle` 及对应 `br2`/`br3` 换行。
    - 清理死代码：`enterDonkeyBtn` 的 `_applyLauncherStatus` 动态 href 改写、DEV 相关 JS（`devModeCheck`/`devModeModal`/`renderDevMode`/`toggleDevModeFromSwitch`/`refreshDevMode`/`requestDevModeToggle`/`closeDevModeModal`/`setDevMode`）、`devModeModal` 确认弹窗 HTML、`#devModeToggle`/`.otaLink`/`.otaButton` 深浅两主题 CSS。
    - init 增加 `setInterval(initMute,5000);`，静音状态 5s 轮询 `/api/mute`，与 DD 顶栏静音键双向同步（DC 侧改动会被 DD 轮询到，反之亦然）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.16 → v1.8.17。
  - 测试同步：`tests/test_firmware_feature_flags.py`——DEV 开关/OTA 按钮头部断言改为 `not in`，`test_web_console_mobile_header_layout` 改为 3 行布局（无 br2/br3/Donkey/OTA/DEV），入口按钮位置与 `.navTab` 计数（2→1）更新，版本与 CHANGELOG 顺序断言升至 v1.8.17。

## 2026-08-19 v1.8.16

- style(WebConsole): DC 顶栏 Donkey / DonkeyDrifter 字体渲染对齐 DD 导航——补 `font-synthesis:none` 阻止 500 字重被浏览器合成加粗，`text-rendering:optimizeLegibility` + `-webkit-font-smoothing:antialiased` + `-moz-osx-font-smoothing:grayscale` 让字形更细更清晰（Issue #108 续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`.headerRow` 追加上述四条渲染属性，使 4 个入口标签继承 DD 导航同款抗锯齿/字形合成策略。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`.headerRow` 断言补 `font-synthesis:none;text-rendering:optimizeLegibility;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale`。

- style(WebConsole): DC 顶栏入口标签彻底复刻 DD 两类标签结构——Donkey / DonkeyDrifter 为 14px 功能标签，Kimi Code Web / DeepSeek Harness 改为 12px 弱化标签并内嵌 lucide 图标（Sparkles / FlaskConical），与 DD 高级入口完全一致（Issue #108 续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 新增 `.navTabWeak` 深色规则：`color:#6b7d90;font-size:0.75rem;font-weight:500;line-height:1rem;display:inline-flex;align-items:center;gap:4px`，hover `#b9c5d3`；新增浅色 `html[data-theme="light"] .navTabWeak`：`color:#7c8da0`，hover `#3f4f63`。
    - KCW/DSH 两个 `<button>` 由 `class="navTab"` 改为 `class="navTabWeak"`，图标 `<svg>` 与文字拆为 `<span data-i18n=...>`，按钮自身移除 `data-i18n`，避免 `applyLanguage` 用 `textContent` 覆盖清掉内嵌图标。
    - `openKimiCodeWeb()`/`openDsh()` 的 loading/复位文案由 `btn.textContent=...` 改为 `btn.querySelector('span[data-i18n]').textContent=...`，点击后图标不再消失。
  - 测试同步：`tests/test_firmware_feature_flags.py`——新增 `.navTabWeak` 深浅色断言、`class="navTabWeak"` 数量 2、KCW/DSH 内嵌图标 SVG 路径断言；`class="navTab"` 数量由 4 改为 2。

- style(WebConsole): 静音按键 `.muteButton` 与相邻深浅色切换 `.themeButton` / 语言切换按钮样式统一——改为 32×32 圆形、同款底色/边框/内阴影与 hover 反馈，深浅两主题对齐（Issue #117）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 深色 `.muteButton`：`height:24px;min-width:28px;padding:0 6px;border:none;background:transparent;color:#8fa1b5` → `width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:#111820;border:1px solid #344154;box-shadow:inset 0 0 0 1px #2b3441;color:#b9c5d3`；保留 `margin-left:auto` 右推布局。
    - 深色 `.muteButton:hover`：`color:#5cc8ff` → `color:#e8edf2`，与主题按钮 hover 一致；`.muteButton.muted` 保持 `color:#5cc8ff` 作为静音激活态。
    - 浅色 `html[data-theme="light"] .muteButton`：`background:transparent` → `background:#f4f6f9;border-color:#ccd5df;box-shadow:inset 0 0 0 1px #d5dce4;color:#3f4f63`，并新增 hover `color:#1a2330`、muted `color:#0c9bd6`；从 `html[data-theme="light"] .ghLink:hover,...` 合并规则中移出静音键的 hover/muted 着色。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.15 → v1.8.16。
  - 测试同步：`tests/test_firmware_feature_flags.py`——静音键深色 32×32 圆形样式与 hover 断言、浅色 `.muteButton` 底色/边框/内阴影/hover/muted 断言，版本断言升至 v1.8.16 且顺序断言补 v1.8.16。

- style(WebConsole): DC 顶栏标题与入口标签改回 DD theme-mus4 实际渲染值（上一轮误按 Tailwind 默认色板，字号/字色/字体/行高均与 DD 皮肤不一致），并修复 Kimi Code Web / DeepSeek Harness 两个 `<button>` 标签字体被浏览器 UA 样式覆盖为 Arial 的问题（Issue #108 续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 标题 `h1`：`font-size:20px` → `1.25rem`，补 `line-height:1.75rem`（DD text-xl），深色字色 `#f4f4f5` → `#e8edf2`（DD theme-mus4 `text-zinc-100`）。
    - 字体栈：`.headerRow` 由 `ui-sans-serif,system-ui,sans-serif` 改为 DD theme-mus4 `.font-sans` 实值 `system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif`。
    - 深色 `.navTab`：字色 `#a1a1aa` → `#8fa1b5`（DD theme-mus4 `text-zinc-400`），hover `#22d3ee` → `#8bdcff`（`hover:text-cyan-400`）；`font-size:14px` → `0.875rem`、`line-height:1` → `1.25rem`（DD text-sm）；补 `font-family:inherit` 让 `<button>` 标签继承 `.headerRow` 的完整字体栈，与 `<a>` 标签一致。
  - 测试同步：`tests/test_firmware_feature_flags.py`——标题/`.navTab` 深色断言更新为 theme-mus4 实值并补 `font-family:inherit`。

## 2026-08-19 v1.8.15

- style(WebConsole): DC 顶栏标题与入口标签的字号/字色/间距进一步对齐 DD 主导航——标题 text-xl 20px + font-bold 700 + zinc-100 前景色，入口标签 zinc-400 前景色 + cyan-400 hover，标题↔功能 32px、功能↔功能 24px（Issue #108 续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `h1` 字号 22px → 20px 并显式 `font-weight:700`；`.headerRow` 追加 `font-family:ui-sans-serif,system-ui,sans-serif`。
    - 新增 `.headerRow h1{color:#f4f4f5;margin:0 20px 0 0}`：标题↔功能间距 = gap 12px + margin-right 20px = 32px（对齐 DD `mr-8`）。
    - 深色 `.navTab` 前景色 `#8fa1b5` → `#a1a1aa`（DD zinc-400），hover `#8bdcff` → `#22d3ee`（DD cyan-400），并追加 `margin-right:12px`：功能↔功能间距 = gap 12px + 12px = 24px（对齐 DD `space-x-6`）。
    - 新增浅色 `html[data-theme="light"] .headerRow h1{color:#1a2330}` 保持浅色标题可读。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.14 → v1.8.15。
  - 测试同步：`tests/test_firmware_feature_flags.py`——标题字号/字色/间距断言，`.navTab` 深色前景色/hover/间距断言更新，版本断言升至 v1.8.15 且顺序断言补 v1.8.14。

## 2026-08-19 v1.8.14

- style(WebConsole): DC 顶栏 Donkey / DonkeyDrifter / Kimi Code Web / DeepSeek Harness 四个入口标签复刻 DD 主导航标签样式（14px / 500 字重 / 弱化前景色，hover 用主题强调色，无框无底无内边距），仅保留这 4 个入口（Issue #108 续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 新增 `.navTab` 深色规则：`color:#8fa1b5;font-size:14px;font-weight:500;background:transparent;border:none;padding:0;line-height:1;white-space:nowrap;display:inline-flex;align-items:center;cursor:pointer`，hover `color:#8bdcff`。
    - 新增浅色 `html[data-theme="light"] .navTab`：`color:#5b6b7d`，hover `color:#0a7eb2`。
    - 4 个入口按钮 `class="otaButton"` → `class="navTab"`；`.headerRow` 的 34px 高规则只保留 `.headerRow .otaLink .otaButton`（OTA 按钮），不再覆盖入口标签。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.13 → v1.8.14。
  - 测试同步：`tests/test_firmware_feature_flags.py`——新增 `.navTab` 深浅色断言，入口按钮 class 断言改为 navTab，34px 规则断言改为仅 OTA，版本断言升至 v1.8.14。

## 2026-08-19 v1.8.13

- fix(WebConsole): DC 顶栏 Donkey / DonkeyDrifter / Kimi Code Web / DeepSeek Harness / OTA 等入口按钮去掉蓝色胶囊框，统一为无框透明样式，深浅两主题下可读且 hover 反馈正常（Issue #108）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 深色 `.otaButton` 基础规则：`background:#5cc8ff;color:#061019;border-color:#5cc8ff` → `background:transparent;color:#e8edf2;border-color:transparent`，去掉蓝色背景与蓝色边框，文字改主题前景色。
    - 深色 `.otaButton:hover`：`background:#8bdcff` → `color:#5cc8ff`，hover 仅改文字色，不再填充蓝色。
    - 浅色 `html[data-theme="light"] .otaButton`：`background:#5cc8ff;color:#061019;border-color:#5cc8ff` → `background:transparent;color:#1a2330;border-color:transparent`。
    - 浅色 hover 规则拆分：原 `html[data-theme="light"] .otaButton:hover,html[data-theme="light"] .rcSetBtn:hover{background:#3aa8dd}` 改为 `.otaButton:hover` 仅 `color:#0c9bd6`，`.rcSetBtn:hover` 保持 `background:#3aa8dd` 不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.12 → v1.8.13。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`.otaButton` 基础/浅色断言改为透明无框样式，版本断言升至 v1.8.13。

## 2026-08-18 v1.8.12

- feat(FW): 支持通过命令设置车控模式（手动/半自动/全自动），与遥控器切换双向兼容（Issue #111）
  - `libraries/mus4_control/src/ControlMixer.h` / `ControlMixer.cpp`：新增 `setCarModeCommand(mode)`；`mode_change()` 改为后到者生效仲裁——新增 `hostMode`（上位机覆盖，-1=无）与 `lastRcMode`（上次生效的 RC 派生模式），遥控器开关位置变化时清覆盖并让遥控器生效，否则维持上位机命令；提取 `applyMode()` 统一写 `car_output.mode` 并触发蜂鸣器。
  - `libraries/mus4_command/src/CommandDispatcher.cpp`：新增 `MODE <m>` / `MODE:<m>` 命令（m∈{0,1,2}），成功回 `ACK:MODE <m>`、非法值回 `NACK:MODE_INVALID`；因 `dispatchCommandLine()` 为 Serial/无线/Web `/api/cmd` 共用入口，命令对所有通道生效。
  - `libraries/mus4_command/src/WirelessConsole.cpp`：新增 `isWirelessModeCommand()`；`isWirelessCommandAllowed()` 对模式命令放行（需认证，Park Locked 下也允许切模式，油门仍由既有 Park/emergencyStop 钳 0）。
  - `MUS4_FW.ino`：`M<m>:P<p>` 帧由「仅 MANUAL」提升为「所有模式，状态变化 + 1Hz 心跳」发送（从 MANUAL-only 的 T<S> 块中提出），使上位机在非 MANUAL 模式下也能收到模式/驻车变化。
  - `README.md` / `README.zh-CN.md`：串口协议输入帧新增 `MODE <m>`；`M:P` 行改为所有模式发送。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.11 → v1.8.12。
  - 测试同步：`tests/test_firmware_feature_flags.py`——新增模式命令/仲裁/M:P 全模式断言，版本断言升至 v1.8.12。

## 2026-08-18 v1.8.11

- fix(WebConsole): 删除 DC 顶栏 LED 闪烁颜色 RGB 切换按键，空闲闪烁固定为 RGB 三色全选（mask=7）且不可修改（Issue #107）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 删除顶栏 `#ledBlinkTabs` 红/绿/蓝多选控件（HTML 及 `#ledBlinkTabs` 容器 34px 覆写、三条选中配色/悬停规则、`order:11` 定位）；共享胶囊样式 `.langTabs` 保留不动。
    - 删除 `uiLedBlinkMask` / `LED_BLINK_TAB_COLORS` / `renderLedBlinkTabs()` / `initLedBlink()` / `toggleLedBlinkColor(bit)` 及启动链中的 `initLedBlink();` 调用。
    - i18n 删除 `led.title` / `led.red` / `led.green` / `led.blue` 中英词条。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：删除 `GET/POST /api/led-blink` 两条路由与 `handleWifiWebLedBlinkGet/Set` 两个处理器及 `#include "LedBlinkPreference.h"`。
  - `libraries/mus4_core/src/LedBlinkPreference.h` / `LedBlinkPreference.cpp`：移除 NVS 持久化（`loadLedBlinkPreference` / `saveLedBlinkPreference` / Preferences），`getLedBlinkMask()` 恒返回 7；空闲（手动 + Park）三色交替闪烁仍由 ControlMixer 读取该掩码、LedStatus 应用，逻辑不变。
  - `MUS4_FW.ino`：删除 `setup()` 中的 `loadLedBlinkPreference();` 调用及 `#include "LedBlinkPreference.h"`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.10 → v1.8.11。
  - 测试同步：`tests/test_firmware_feature_flags.py`——响应式 order 测试删除 `#ledBlinkTabs{order:11}` 断言（注释同步改为「OTA + 静音」）；`test_web_console_theme_toggle` 删除 themeToggle 与 ledBlinkTabs 的相对顺序断言；`test_web_console_led_blink_color_selector` 重写为「前端控件/逻辑/词条与后端接口/持久化全部移除、`getLedBlinkMask()` 恒返回 7、ControlMixer/LedStatus 仍驱动空闲闪烁」断言；版本断言升至 v1.8.11。全量 163 passed。

## 2026-08-18 v1.8.10

- fix(WebConsole): DC 终端板块不再长时间停留在「正在连接上位机终端」——首探前先等待真实上报 IP，消除用默认回退 IP 首探必败的窗口
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `addTerminalTab()`：改为先完成标签 UI 创建与选中，再 `_fetchLauncherIp().then(()=>probeTerminal(term))` 首探——原实现首探在页面加载时往往先于 `/api/status` 返回执行，用脚本默认回退值 `192.168.3.41`（DHCP 漂移后已失联）探测必然失败，用户看到长时间 loading 后才靠 4s 重试自愈；真机网络环境下叠加首探 10s 超时即表现为"一直连不上"。
    - `probeTerminal()`：探测超时由 10000ms 缩短为 5000ms，配合 4s 自动重试更快收敛。
    - `termFailHint()`：从未收到 host_ip 上报（`_launcherIpAge===-1`，仍为默认回退值）时直接返回 `terminal.unknownIp` 提示，不再拼出误导性的回退地址链接。
    - i18n 新增 `terminal.unknownIp` 中英词条。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.9 → v1.8.10。
  - 测试同步：`tests/test_firmware_feature_flags.py`——首探断言更新为 `_fetchLauncherIp().then(()=>probeTerminal(term))`；超时断言 10000 → 5000；新增 `terminal.unknownIp` 词条与 `termFailHint` IP 未知分支断言；版本断言升至 v1.8.10、CHANGELOG 顺序链延伸至 v1.8.10。

## 2026-08-18 v1.8.9

- feat(WebConsole): DC 顶栏「Drifter Console」标题文字可点击跳转官网，效果与点击 logo 图标一致（Issue #179，跨仓库功能：DD/DC/D 三页面标题可点）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主标题 `<h1 data-i18n="app.title">Drifter Console</h1>` 改为 `<h1><a class="titleLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener" data-i18n="app.title">Drifter Console</a></h1>`——`data-i18n` 随文字移到 `<a>` 上（i18n 用 textContent 赋值，包裹层不劫持），语言切换正常；与 logo 链接同 URL、新标签页打开。
    - CSS 新增 `.titleLink{color:inherit;text-decoration:none}`：颜色继承 h1、无下划线，深浅主题下文字样式均与原纯文本一致。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.8 → v1.8.9。
  - 测试同步：`tests/test_firmware_feature_flags.py`——logo 测试、GitHub 链接顺序测试、入口按钮位置测试中三处 `<h1 data-i18n=...>` 断言更新为新的 `<a class="titleLink">` 结构并新增 titleLink HTML/CSS 断言；版本断言升至 v1.8.9、CHANGELOG 顺序链延伸至 v1.8.9。全量 163 passed。
  - DD（DonkeyDrifter Web UI）与 D（Donkey 启动页）两侧同类改动在 DonkeyDrift 仓库同步提交（`web_ui/frontend/src/components/Layout.tsx`、`donkeycar/launcher/server.py`）。

## 2026-08-18 v1.8.8

- fix(WebConsole): DC「进入 DD」（DonkeyDrifter）按钮改为直达启动中转页，中间不再显示 Donkey 启动菜单页（Issue #103）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`enterDonkeyDrifterBtn` 两处指向从 `http://<ip>:8090/#drive` 改为 `http://<ip>:8090/launch/drive`——headerRow 静态初值（`192.168.3.41`）与 `_applyLauncherStatus()` 动态改写。`/launch/drive` 为 launcher 现成 GET 端点，返回极简跳转页（spinner + 同源 POST `/api/launch/drive` + 轮询 vite 就绪后重定向），全程不渲染 Donkey 菜单页；原 `#drive` 路径需先渲染菜单页再由其 JS 检测 hash 自动触发 6 号启动，中间页即用户所见问题。
  - 「Donkey」按钮（`:8090/`）保持不动。
  - 历史：v1.7.62 曾因 Safari 无法加载 `/launch/drive` 改用 `#drive`；此后 `LAUNCH_DRIVE_HTML` 已改轮询就绪后重定向（DonkeyDrift 侧），且 `/terminal` 等非标准路径 Safari 下正常，判定根因已消除，本次改回。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.7 → v1.8.8。
  - 测试同步：`tests/test_firmware_feature_flags.py`——`#drive` 断言改为 `:8090/launch/drive` 出现 2 次（静态 + 动态）且 `:8090/#drive` 不再出现；版本断言升至 v1.8.8、CHANGELOG 顺序链延伸至 v1.8.8。

## 2026-08-18 v1.8.7

- feat(WebConsole): DC 顶栏新增「DeepSeek Harness」入口按钮，与 DonkeyDrifter Web UI（DD）侧同款（Issue #164 后续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 顶栏「Kimi Code Web」右侧新增 `#openDshBtn`（`button.openDsh`，文案「DeepSeek Harness」），交互与 Kimi Code Web 按钮完全同款：沿用 `_launcherIp`（`/api/status` 的 `host_ip`），点击 `POST http://<host>:8090/api/launch/dsh`（launcher 侧转发的 DSH 启动端点），同步上下文先开 `about:blank` 句柄，成功后导航 `j.url`、失败关闭并走 toast 提示；`AbortController` 120s 超时；等待态禁用按钮并切换启动中文案（`button.openDshLaunching`）。
    - i18n 中英各新增 4 条：`button.openDsh` / `button.openDshLaunching` / `toast.dshFailed` / `toast.dshTimeout`。
    - CSS：34px 高度规则追加 `#openDshBtn`；窄屏（max-width:820px）第 2 行插入 DSH 按钮 `order:9`，后续元素（br2/ledBlink/OTA/静音/DEV/br3/主题/语言）顺移 +1 至 10–17。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.6 → v1.8.7。
  - 测试同步：`tests/test_firmware_feature_flags.py`——34px 规则与窄屏 order 断言更新（含 `#openDshBtn{order:9}` 顺移链）；`test_web_console_header_entry_buttons` 新增 DSH 按钮 HTML 位置（kimi 之后、GitHub 链接之前）、`openDsh()`/`:8090/api/launch/dsh`/`dshLaunching` 与中英 i18n 词条断言；版本断言升至 v1.8.7、CHANGELOG 顺序链延伸至 v1.8.7。
  - DSH 启动端点与 launcher 侧修复（设置页 403 权限补丁、缺省 cwd 进 Projects）在 DonkeyDrift 仓库同步提交。

## 2026-08-17 v1.8.6

- fix(WebConsole): DC 终端 loading 态加 10s 超时兜底，不再无限期停留在「正在连接上位机终端…」（Issue #101）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`probeTerminal()` 改为带超时的探测——`AbortController` + `setTimeout(...,10000)`，`no-cors` fetch 因上位机 IP 不可达长时间挂起（TCP 无响应、reject 也不来）时 10s 未落定一律按 fail 处理，复用既有的失败提示（含 staleIp 年龄标注）与 `scheduleTermRetry()` 每 4s 自动重试；引入探测序号 `term._probe` 防止旧探测的迟到结果覆盖新探测的状态（超时转 fail 后，迟到的成功也不会误把标签置回 ok）。
  - 上位机终端页内层 WS 连接超时在 DonkeyDrift 仓库同步修复（DonkeyDrift `donkeycar/launcher/terminal_static/terminal.html`，Issue #101 补充线索第 2 层）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.5 → v1.8.6。
  - 测试同步：`tests/test_firmware_feature_flags.py` 终端测试新增 #101 断言（探测序号/AbortController 超时/signal/序号守卫/done 分发）；版本断言升至 v1.8.6、CHANGELOG 顺序链延伸至 v1.8.6。全量 163 passed。

## 2026-08-17 v1.8.5

- style(WebConsole): 三页面（DC/D/DD）语言按钮配色统一为 DC/D 主题按钮（深浅切换）样式（Issue #92 后续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`.langButton` 从 DD 原生 zinc 配色（#27272a 底、#3f3f46 边框、#d4d4d8 字色、hover #f4f4f5；浅色 #f4f4f5/#d4d4d8/#52525b/#18181b）改为 DC/D 主题按钮（`.themeButton`）配色——深色 `background:#111820` + `border:1px solid #344154` + `box-shadow:inset 0 0 0 1px #2b3441` 内圈、字色 `#b9c5d3`、hover `#e8edf2`；浅色 `background:#f4f6f9` + `border-color:#ccd5df` + 内圈 `#d5dce4`、字色 `#3f4f63`、hover `#1a2330`；32×32 圆形、字体栈、字号/字重与切换逻辑均保持不变，仅换配色；hover 规则的 background 锁定同步换为 #111820/#f4f6f9。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.4 → v1.8.5。
  - 测试同步：`tests/test_firmware_feature_flags.py` `.langButton` CSS 精确串断言（深/浅四条）更新为主题按钮配色串；`test_firmware_version_is_current_and_changelog_is_ordered` 版本断言升至 v1.8.5、CHANGELOG 顺序链延伸至 v1.8.5。全量 163 passed。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.8.5 build="Aug 17 2026 09:52:02"`；无头浏览器实测深浅两主题下 `#langToggle` 与 `#themeToggle` 计算样式逐值一致（bg/border/inset 内圈/字色/32×32）。

## 2026-08-17 v1.8.4

- fix(WebConsole): DC 深浅主题切换按钮与 DD 主题按钮逐值统一——三处（DC / D 启动页 / DD）按钮一模一样（DD Issue #140 后续）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `#themeToggle`（`.themeButton`）视觉从透明幽灵胶囊（24px 高、无背景无边框、图标色 `#8fa1b5`、hover 青 `#5cc8ff`）改为 DD 主题按钮的实际渲染规格（DD 的 Tailwind 类经 `theme-mus4.css`/`theme-light.css` 重映射后的值）：32×32 圆形（`border-radius:9999px`），深色 `background:#111820` + `border:1px solid #344154` + `box-shadow:inset 0 0 0 1px #2b3441` 内描边，图标色 `#b9c5d3`、hover `#e8edf2`；浅色 `background:#f4f6f9` + `border-color:#ccd5df` + 内描边 `#d5dce4`，图标色 `#3f4f63`、hover `#1a2330`。
    - 图标从 feather 路径换为 lucide Moon/Sun（与 DD `ThemeSwitcher.tsx`、D 启动页 `server.py` 完全相同的路径数据）：月亮 `M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z`，太阳 circle r=4 + 8 条独立射线 path；16px、stroke-width 2、`stroke="currentColor"` 不变。
    - 逻辑不动：单击深/浅互切、`mus4.ui.theme` 持久化、默认跟随浏览器 `prefers-color-scheme`、深色显月亮/浅色显太阳均保持 v1.8.3 行为。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.3 → v1.8.4。
  - 测试同步：`tests/test_firmware_feature_flags.py` `test_web_console_theme_toggle` 的 CSS 断言更新为 DD 规格逐值串（含浅色 `color:#3f4f63`、hover `#1a2330`），新增 lucide 图标路径断言；`test_firmware_version_is_current_and_changelog_is_ordered` 版本断言升至 v1.8.4，CHANGELOG 顺序链延伸至 v1.8.4。

## 2026-08-17 v1.8.3

- style(WebConsole): 语言按钮字体逐值复刻 DD（#92 返工：字体栈补齐 + 车上旧样式根因修复）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`.langButton` 补 DD `index.css` :root 完整字体栈（`-apple-system,BlinkMacSystemFont,"Segoe UI","Noto Sans",Helvetica,Arial,sans-serif,"Apple Color Emoji","Segoe UI Emoji"`）及 `font-synthesis:none;text-rendering:optimizeLegibility;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale`，抵消 DC 页面级 `system-ui,sans-serif` 与基础 `button` 规则的字体继承，按钮渲染字体与 DD 完全一致。
  - 车上"无边框/24px/透明底"假象根因：另一并行工作区分支于 08:46 用未合入新样式的中间固件 OTA 覆盖了车辆（此前 08:36 已刷入正确样式），源码与 specificity 均无问题；本轮重编译 v1.8.3（build 09:06:41）OTA 刷回，无头浏览器实测计算样式确认 32×32、1px #3f3f46 边框、#27272a 底、#d4d4d8 字色、600 字重、DD 字体栈（深浅两主题分别验证）。
  - 测试同步：`tests/test_firmware_feature_flags.py` `.langButton` CSS 精确串断言更新为含字体栈的新串，全量 163 passed。
- style(WebConsole): 三页面（DC/D/DD）语言切换按钮样式统一为 DD 原生样式（GitHub Issue #92 后续统一）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`.langButton` 从 DC 自有样式（24px 高、透明底、无边框、#8fa1b5 字色、12px/800）改为逐值复刻 DD `LanguageSwitcher` 原生渲染——32×32 圆形（border-radius:9999px）、#27272a 底、1px #3f3f46 边框、12px/600、#d4d4d8 字色、hover #f4f4f5，transition 覆盖 color/background-color/border-color；hover 规则补 background 锁定（#27272a），抵消 DC 通用 `button:hover` 的背景覆盖，保证与 DD"hover 只变色"一致；浅色主题用同族 zinc 值（底 #f4f4f5、边框 #d4d4d8、字色 #52525b、hover #18181b）。
  - 测试同步：`tests/test_firmware_feature_flags.py` `test_web_console_language_tabs_wired_to_set_language` 的 `.langButton` CSS 精确串断言更新为 DD 样式串（深/浅两套），全量 163 passed。
- feat(WebConsole): DC 语言切换改为静音式单按钮——单击中/英互切，未手动选过语言时默认跟随浏览器语言（GitHub Issue #92）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 主页面切换入口从折叠 FAB + 弹出菜单（🌐 `langFab` → `langMenu` 中/英两项）改为顶栏单按钮 `#langToggle`（`.langButton`）：透明胶囊文字按钮，中文态显「中」、英文态显「EN」，位于静音按钮右边、主题切换按钮左边（窄屏 order:16）。
    - 单击 `toggleLanguage()` 在中/英间互切（`setLanguage(uiLang==='zh'?'en':'zh')`）并持久化到 `mus4.ui.lang`；`renderLangButton()` 随语言切换刷新按钮文字，aria 标题复用 `language.title` 词条。
    - 默认跟随浏览器语言：四个页面（主页面/Donkey 页/Joystick 校准页/关于页）的 `readStoredLanguage()` 在 localStorage 无有效值时回退 `detectBrowserLanguage()`（`navigator.language` 以 `zh` 开头即中文）；服务端 `/api/language` 返回 `auto` 时同样按浏览器语言解析后写入本地存储。
    - 移除弹出菜单全部代码：`langFab`/`langMenu` 的 HTML、CSS 与 FAB 径向动作排序链、`toggleLanguageMenu()`/`closeLanguageMenu()`/`renderLangMenu()`、🌐 图标；三页面既有 `langTabs`（诊断/关于/Joystick 简化页的双页签）保持不动。
  - 测试同步：`tests/test_firmware_feature_flags.py` `test_web_console_language_tabs_wired_to_set_language` 重写为单按钮断言（按钮 HTML/CSS/`toggleLanguage`/`renderLangButton`/`readStoredLanguage` 浏览器回退/`detectBrowserLanguage` 精确串，`langSwitch`/`data-lang`/English/`langFab`/`langMenu` 无残留断言）；FAB 径向动作断言去掉语言入口并加无残留检查；移动端顶栏断言改 `#langToggle` 与窄屏 order:16；`data-i18n-title="language.title"` 断言改 `data-i18n-aria`。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.8.3 build="Aug 17 2026 08:16:58"`。
- feat(WebConsole): DC 深浅主题切换改为静音式单按钮——单击深/浅互切，默认跟随浏览器深浅（GitHub Issue #93）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 切换入口从 `#themeTabs` 三态按钮组（浅色/跟随系统/深色）改为单图标按钮 `#themeToggle`（`.themeButton`），形态与位置参照同顶栏静音按钮 `#muteToggle`：透明胶囊图标按钮，位于红绿蓝切换键右边、语言切换键左边（窄屏 order:15 不变）。
    - 单击 `toggleTheme()` 在当前生效主题的深/浅反向间来回切换（`setTheme(resolvedTheme()==='light'?'dark':'light')`）并持久化到 `mus4.ui.theme`；图标反映当前主题——深色显月亮（`icoMoon`）、浅色显太阳（`icoSun`），由 `html[data-theme]` CSS 驱动，无需 JS 改图标。
    - 默认跟随浏览器 `prefers-color-scheme`：`uiTheme='auto'`（localStorage 无值）时 `resolvedTheme()` 经 matchMedia 解析并监听系统 change 实时跟随；用户手动单击后选择持久化，此后不再跟随浏览器，刷新后保持。auto 态由"未手动切换 = 跟随浏览器"等效替代。
    - 移除三态按钮组全部代码：`#themeTabs` 容器与按钮 HTML、`renderThemeTabs()`、`setTheme()/initTheme()` 中的渲染调用、深浅两套 `#themeTabs` 分段控件 CSS；i18n 删去 `theme.auto/light/dark` 词条（仅保留 `theme.title` 作 aria 标题，中英各一条）。
    - 首屏防闪烁内联脚本、`systemTheme()/resolvedTheme()/applyTheme()` 主题解析应用链路保持不动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.2 → v1.8.3。
  - 测试同步：`tests/test_firmware_feature_flags.py` `test_web_console_theme_toggle` 重写为单按钮断言（按钮 HTML/双图标/CSS/`toggleTheme` 一键互切、`themeTabs`/`themeSwitch`/`renderThemeTabs`/`theme.auto|light|dark` 无残留断言）；窄屏 order 断言改 `#themeToggle{order:15}`；`test_web_console_light_theme_overrides` 的 `setTheme/initTheme` 断言去渲染调用；版本断言升至 v1.8.3，CHANGELOG 顺序链延伸至 v1.8.3。

## 2026-08-16 v1.8.2

- fix(WebConsole): 修复终端标签智能缩写在真实设备上始终不触发（GitHub Issue #90 复盘：v1.8.1 的振荡修复正确但溢出检测根本没机会触发）
  - 根因（无头浏览器在车上 v1.8.1 实测复现）：`.grid` 的 `grid-template-columns:1fr` / `@media(min-width:900px)` 下 `2fr 1fr` 均未加 `minmax(0,…)`，grid 列最小宽度默认取内容宽度——多标签时 `#termTabs` 的内容宽度沿 `.row`→`.panel`→grid 列一路顶住最小宽度，`clientWidth` 恒等于 `scrollWidth`，页面转而出现横向滚动；`fitTermTabLabels()` 的 `scrollWidth>clientWidth` 永远为 false，缩写永不触发（此前推测的"车上固件落后/振荡缺陷"都不是主因，振荡缺陷已在 v1.8.1 修复）。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：两处 grid 列定义加 `minmax(0,…)`（`minmax(0,1fr)`；`minmax(0,2fr) minmax(0,1fr)`），列可收缩到内容以下，标签条真正溢出，智能缩写/恢复长名按预期工作，页面不再横向滚动。
  - 验证：Playwright Chromium 对车上真实页面回归——单列 850px 下 4 标签显示「终端 1…4」、12 标签缩写为「1…12」、临界宽度 500↔1280 来回 10 轮无振荡残留、`documentElement.scrollWidth` 不再超出视口。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.1 → v1.8.2。
  - 测试同步：`tests/test_firmware_feature_flags.py` grid 断言更新为 `minmax(0,…)` 并新增"无裸 `2fr 1fr` 残留"断言；版本断言升至 v1.8.2，CHANGELOG 顺序链延伸至 v1.8.2。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.8.2 build="Aug 16 2026 21:18:03"`，页面含 `minmax(0,2fr) minmax(0,1fr)`。

## 2026-08-16 v1.8.1

- fix(WebConsole): 修复 DC 终端偶发"无法连接上位机终端服务"后不恢复（GitHub Issue #89）与终端标签智能缩写临界宽度振荡"未生效"（GitHub Issue #90）
  - `libraries/mus4_web/src/WebConsoleAssets.h`（Issue #89）：
    - 终端探活从 `addTerminalTab()` 内联的一次性 fetch 抽为可重入的 `probeTerminal(term)`；探测失败后 `scheduleTermRetry()` 启动 4s 周期重试定时器（全局唯一 `_termRetryTimer`），每轮先 `_fetchLauncherIp()` 刷新 host_ip（DHCP 变更后拿新 IP），再对所有 `state==='fail'` 的标签重探；任一标签成功或失败标签全部消失（关闭）即停止定时器；成功后设置 iframe `src`（已设置则不重设，避免整页刷新丢会话）并清掉提示——上位机恢复在线后终端自动加载，不再停留在失败提示。
    - `_fetchLauncherIp()` 的解析逻辑抽为 `_applyLauncherStatus(txt)`，除 `host_ip=` 外新增解析 `host_ip_age_s=` 存入 `_launcherIpAge`（无数据为 -1）。
    - 失败提示抽为 `termFailHint()`：年龄 >90s（上位机正常 30s 上报一次，3 个周期未更新视为过期）时在提示尾部追加「上位机 IP 已 N 秒未上报，可能已过期」，辅助区分"上位机离线"与"ESP32 里的 host_ip 已过期"。
    - i18n 新增 `terminal.staleIp` 中英词条（带 `{n}` 占位）。
  - `libraries/mus4_web/src/WebConsoleAssets.h`（Issue #90）：
    - `fitTermTabLabels()` 改为每次先统一恢复长名「终端 N」测量，`scrollWidth>clientWidth` 才缩写为 N；原实现按改名前布局判定 `packed`，改名后不复查，临界宽度下长名↔短名来回振荡，稳态停在"长名+溢出"，用户看到的就是"功能没生效"；改后判定结果确定、可收敛，关标签/缩窗口恢复长名。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.8.0 → v1.8.1。
  - 测试同步：`tests/test_firmware_feature_flags.py` 终端断言更新——`fitTermTabLabels` 先长名测量断言 + `packed?` 不再存在断言；新增 `probeTerminal`/`scheduleTermRetry`/`_applyLauncherStatus`/`host_ip_age_s` 解析/`termFailHint`/`terminal.staleIp` 词条断言；版本断言升至 v1.8.1，CHANGELOG 顺序链延伸至 v1.8.1。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.8.1`。

## 2026-08-16 v1.8.0

- fix(wifi): 修复 STA 连接历史重试"一轮耗尽后不再扫描"与"STA 未配置时不进重试窗口"两个缺口（GitHub Issue #88：之前连接过的 Wi-Fi 出现后小车不自动连接）
  - `libraries/mus4_wifi/src/WifiManager.cpp`：
    - 新增常量 `WIFI_STA_HISTORY_RESCAN_INTERVAL_MS = 15000`（紧邻既有 `WIFI_STA_HISTORY_RETRY_INTERVAL_MS`）：一轮历史候选试完后等待 15s 冷却再重开新一轮扫描，覆盖"小车先开机、Wi-Fi 后出现"场景；时长为扫描频率与空转功耗折中。
    - `updateWifiStaHistoryRetry()` "candidates exhausted" 分支不再终局：记录 `staHistRescanDeadlineMs = millis() + 15000`，日志改为 `candidates exhausted, rescan in 15s`；原实现把 `staHistTriedMask` 全槽位置位后永久停止扫描，历史 Wi-Fi 之后出现时小车永不重连。
    - `anyUntried==false` 分支改为冷却判定：未到 `staHistRescanDeadlineMs` 保持 `staHistRetryActive=false` 直接返回；冷却期满清 `staHistTriedMask`、打日志 `starting new round`，落入既有扫描启动逻辑重开新一轮（时间回绕比较沿用 `(long)(millis() - deadline) < 0` 风格）。
    - 重试窗口条件 `inRetryWindow` 追加 `|| wifiStaHistoryCount() > 0`：STA 从未配置（NVS `sta_en=false`）但历史记录非空时运行期也进入重试，不再只能靠开机那一刻的扫描；函数既有 `wifiStaHistoryCount() == 0` 兜底保证历史为空时不空转。
    - connected 上升沿同步清 `staHistRescanDeadlineMs = 0`。
  - `libraries/mus4_core/src/RuntimeState.h`：`WifiRuntimeState` 新增 `unsigned long staHistRescanDeadlineMs = 0;`（候选耗尽后重开新一轮扫描的最早时刻），注释块同步补充。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.8.0（行为修复含状态机语义扩展，进 minor）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 新增 `test_wifi_sta_history_retry_rescans_after_exhaustion`（冷却常量、重扫字段与 connected 边沿清零、exhausted/rescan 日志、回绕比较写法断言）与 `test_wifi_sta_history_retry_window_accepts_unconfigured_sta`（窗口条件含 `wifiStaHistoryCount() > 0`、空历史兜底仍在断言）；版本断言升至 v1.8.0，CHANGELOG 逐版本断言与顺序链延伸至 v1.8.0。

## 2026-08-16 v1.7.99

- feat(WebConsole): Serial 终端窗口右下角新增全屏按钮，UI 与行为完全对齐遥测曲线面板 `#chartPanel` 的全屏按钮
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - HTML：`#terminalWrap` 内末尾（`#terminalHint` 之后）新增 `#termFullscreenBtn`（`class="iconButton"`，`onclick="toggleTerminalFullscreen()"`，SVG 与 chartFullscreenBtn 完全相同）；按钮居 DOM 末尾，`addTerminalTab()` 用 `terminalWrap.insertBefore(f,terminalHint)` 插入的 iframe 始终位于按钮之下，按钮可压在 iframe 上。
    - CSS：`#terminalWrap` 规则内追加 `position:relative`（`.termFrame` 规则不动）；紧跟 `#chartFullscreenBtn` 规则新增 `#termFullscreenBtn{position:absolute;right:8px;bottom:8px;z-index:2}`；新增 `#terminalWrap:fullscreen{background:#101318;height:auto;min-height:0;max-height:none}`——抵消原规则的 height/min-height/max-height，否则全屏被 calc 钳住；浅色主题在 `html[data-theme="light"] #chartPanel:fullscreen` 旁新增 `html[data-theme="light"] #terminalWrap:fullscreen{background:#eef1f5}`。
    - JS：`toggleChartFullscreen()` 旁新增同构的 `toggleTerminalFullscreen()`；`refreshDynamicLabels()` const 链仿照 `f` 新增 `tf=document.getElementById('termFullscreenBtn')`，函数体内加 `if(tf)` 守卫的图标/标题切换（复用 `button.fullscreen`/`button.split` i18n 键，未新增键）；复用已有 fullscreenchange 监听（会调 `refreshDynamicLabels()`），未新增监听；`applyCmdTarget` 未改（按钮随 `#terminalWrap` 显示/隐藏）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.99。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本断言升至 v1.7.99，CHANGELOG 逐版本断言新增 v1.7.99，日期顺序链延伸至 v1.7.99；图表全屏断言附近新增 `#termFullscreenBtn` 按钮 HTML、CSS 规则与 `toggleTerminalFullscreen()`/`tf` 图标切换断言。
  - 已 OTA 刷至车辆（192.168.3.52，DHCP 由 .46 变为 .52）验证：`/api/status` 返回 `version=v1.7.99`。

## 2026-08-16 v1.7.98

- feat(WebConsole): 点击 DC 主页左上角 logo 在新标签页打开 https://www.donkeydrift.com
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 DC 主页）：页头 `<img class="headerLogo" src="/favicon.png" alt="Drifter Console">` 包进 `<a class="logoLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener">`；CSS 在 `.headerLogo` 规则旁新增 `.logoLink{display:inline-flex}`——锚点以 inline-flex 包裹，`.headerRow` 原有 flex 布局、`gap:12px` 与 logo 尺寸完全不变；light 主题的 `.headerLogo` 描边覆盖不受影响。Drift Assist Tuning 子页面页头无 logo，未改动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.98（避让：并行会话 #85 终端保底一个已占用 v1.7.97）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 版本断言升至 v1.7.98，`test_web_console_header_logo_left_of_title` 新增 logoLink 锚点包裹与 `.logoLink{display:inline-flex}` 断言；版本链延伸至 v1.7.98。

## 2026-08-16 v1.7.97

- feat(WebConsole): 终端标签页保底一个——仅剩一个终端窗口时不允许关闭，防止终端被全部关空
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 新增 `updateTermTabClose()`：`termList.length<=1` 时隐藏所有标签的 × 关闭钮（`display:none`），多于一个时恢复显示；`addTerminalTab()` 末尾与 `killTerminalTab()` 杀标签后均调用刷新。
    - `killTerminalTab()` 入口新增守卫 `if(termList.length<=1)return;`——逻辑层双保险，最后一个终端永远关不掉；term 对象新增 `c` 字段引用 × 元素以便控制显隐。
    - `termList.length===0` 的空态分支保留作兜底（新逻辑下不可达）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.97。
  - 测试同步：`tests/test_firmware_feature_flags.py` 新增 `updateTermTabClose`、term 对象 `c:c` 引用、`killTerminalTab` 入口守卫断言；版本链延伸至 v1.7.97。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.7.97`。

## 2026-08-16 v1.7.96

- fix(WebConsole): 终端标签页名字首次命名后锁定——终端内输入首条命令命名（如 `kimi`）后，后续命令（如 `/web`）上报的名字被忽略，标签始终保持 `kimi` 不变
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`donkeydrifter.term.name` message 监听由 `if(!cur)return;` 改为 `if(!cur||cur.name)return;`——`cur.name` 非空（已自定义命名）的标签不再接受后续上报改名；未命名标签（`name` 为 null）仍按输入首词命名，`fitTermTabLabels()` 跳过自定义名的逻辑不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.96。
  - 测试同步：`tests/test_firmware_feature_flags.py` 命名断言注释更新为"首次命名后锁定"语义，新增 `if(!cur||cur.name)return;` 断言；版本链延伸至 v1.7.96。

## 2026-08-15 v1.7.95

- 固件版本号从 `v1.7.92` 更新到 `v1.7.95`（避让：并行会话 #80 入口按键去前缀占用 v1.7.92、#82 主题/语言切换键重设计与 RGB 粗框占用 v1.7.93/v1.7.94）。
- fix(WebConsole): 终端标签过多时标签条内部滚动、不再改变窗口比例；➕ 钉在行尾右端；默认标签名智能缩写（标签条放不下时缩写为纯数字）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 标签条溢出不改布局：`#termTabs` 增加 `scrollbar-width:none` + `#termTabs::-webkit-scrollbar{display:none}`——溢出时横向滚动但不再出现滚动条（此前 6+ 标签时滚动条挤出额外行高、工具行比例被改变）；`flex:1 1 auto;min-width:0` 不变，条内滚动不撑宽页面。
    - ➕ 始终靠右：`#newTermBtn` 从 `#termTabs` 滚动区内移出、作为其右邻兄弟元素钉在行尾（`#newTermBtn{width:22px;height:22px;flex:0 0 auto}`，替代原 `#termTabs .iconButton` 规则），无论多少个标签都固定可见；`addTerminalTab()` 改回 `termTabs.appendChild(b)`。
    - 默认标签名智能缩写：新增 `fitTermTabLabels()`——标签条溢出（`termTabs.scrollWidth>termTabs.clientWidth`）时未命名标签缩写为纯数字 N，放得下时恢复 `t('terminal.tab')+' '+N`（“终端 N”/“Term N”）；新建、杀标签、窗口 resize（`window.addEventListener('resize',fitTermTabLabels)`）、切回 Serial（`applyCmdTarget`）时均重算；v1.7.90 的自定义改名标签（`x.name` 非空）跳过不受影响；i18n `terminal.tab` 中英词条保留。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.95。
  - 测试同步：`tests/test_firmware_feature_flags.py` 更新工具行 DOM、`appendChild`、`fitTermTabLabels` 智能缩写（`scrollWidth>clientWidth`、`packed` 三元、`resize` 监听）、`applyCmdTarget` 切回重算断言，新增隐藏滚动条与 `#newTermBtn` 钉右样式断言；`terminal.tab` 词条为存在断言；版本链延伸至 v1.7.95。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.7.95`。

## 2026-08-15 v1.7.94

- feat(WebConsole): RGB 闪烁颜色开关（.langTabs）外框升级为 DC 粗框语言——外圈 1px border + 内圈 1px inset 描边，与主题/中英文切换键同款（避让：并行会话 #78/#79/#80 已占用 v1.7.88-v1.7.92，本分支两条目改号 v1.7.93/v1.7.94）
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 DC 主页）：
    - 深色基础：容器 `border:none` → `border:1px solid #344154`，保留 `box-shadow:inset 0 0 0 1px #2b3441` 内描边——两条 1px 相加，视觉 2px 粗框（此前只有单圈内描边）；容器沿用 v1.7.89 的 34px 总高 + 4px 纵向 padding（border-box），内容区高 24px，分段按钮保持 24px 不变。
    - 浅色覆写：`html[data-theme="light"] .langTabs` 增加 `border-color:#ccd5df`，内描边 `#aeb9c7`→`#d5dce4`——与 `#themeTabs`/`.langSwitch` 浅色框色逐值一致（外 #ccd5df + 内 #d5dce4）。
    - 选中段连体拼接 JS（`renderLedBlinkTabs`，只动按钮圆角/阴影）不受影响。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.94。
  - 测试同步：`tests/test_firmware_feature_flags.py` 更新 `.langTabs` 深色容器/浅色覆写断言；版本链延伸至 v1.7.94。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.7.94`，Playwright 深/浅截图复核 RGB 开关粗框生效。

## 2026-08-15 v1.7.93

- feat(WebConsole): 主题/语言切换键重设计——深色配色改用 DD 皮肤实际渲染值，与 DonkeyDrifter ThemeSwitcher/LanguageSwitcher 双主题完全同款
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 DC 主页）：
    - 背景：v1.7.78/v1.7.81 初版照搬的是 Tailwind 默认色板原值（zinc-800 `#27272a` / cyan-600 `#0891b2`），而 DD 实际运行时被皮肤 CSS（theme-mus4.css/theme-light.css）整体覆盖，真实渲染是另一组色；本次改为按 DD 皮肤渲染值 1:1 复刻。
    - `#themeTabs`（深色基础）：容器 `#27272a`→`#111820`、边框 `#3f3f46`→`#344154` 并新增内描边 `box-shadow:inset 0 0 0 1px #2b3441`；按钮未选中 `#a1a1aa`→`#8fa1b5`、hover `#e4e4e7`→`#e8edf2`；选中段 `#0891b2` 白字 → `#5cc8ff` 蓝底 + `#061019` 近黑字 + 800 粗（hover 不变色，同 DD）。
    - `#themeTabs` 浅色覆写：容器 `#dde3ec`/`#aeb9c7` → `#f4f6f9`/`#ccd5df` + 内描边 `#d5dce4`，hover 文字 `#0b2536`→`#1a2330`，选中段同步改为 `#5cc8ff`+`#061019`（800 粗继承深色基础规则）——浅色下与旁边 `.langSwitch` 现有浅色样式逐值一致。
    - `.langSwitch`（深色基础）同步换成同一组皮肤渲染值（容器/边框/内描边/文字/hover/选中段全部与 `#themeTabs` 相同），修正 v1.7.78 深色沿用 Tailwind 原值导致的「双切换键深色选中色不一致」；浅色覆写不动（已是 DD 皮肤值）。
    - 结果：深/浅两种模式下，主题切换键 = 中英文切换键 = DD 的 ThemeSwitcher = DD 的 LanguageSwitcher，四个控件同一视觉语言；几何不变（容器 34px、按钮 24px、12px 字、胶囊圆角）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.93。
  - 测试同步：`tests/test_firmware_feature_flags.py` 更新 `#themeTabs`/`.langSwitch` 深浅色断言为新皮肤值。
  - 已 OTA 刷至车辆（192.168.3.46）验证，Playwright 深/浅截图复核两组切换键视觉一致。
## 2026-08-15 v1.7.92

- feat(WebConsole): DC 头部三个入口按键显示文案去掉"打开 "/"Open "前缀（zh/en 同步），与 DD 侧同步精简
  - `libraries/mus4_web/src/WebConsoleAssets.h`：headerRow 静态 HTML `>打开 Donkey</a>`→`>Donkey</a>`、`>打开 DonkeyDrifter</a>`→`>DonkeyDrifter</a>`、`>打开 Kimi Code Web</button>`→`>Kimi Code Web</button>`；I18N zh 词典 `'button.enterDonkey':'打开 Donkey'`→`'Donkey'`、`'button.enterDonkeyDrifter':'打开 DonkeyDrifter'`→`'DonkeyDrifter'`、`'button.openKimiCodeWeb':'打开 Kimi Code Web'`→`'Kimi Code Web'`，en 词典 `'Open Donkey'`/`'Open DonkeyDrifter'`/`'Open Kimi Code Web'` 三值同步去前缀。按钮 id、data-i18n 键名、href/onclick 跳转与其它词条（toast、"打开新地址"等）一律不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.92（避让：并行会话 #78/#79 已占用 v1.7.90/v1.7.91）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 入口按键 i18n 断言改为去前缀文案（zh/en 词条值相同后断言各出现 2 次），版本链延伸至 v1.7.92。
  - 验证：编译通过；全量 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 与首页 HTML 确认。

## 2026-08-15 v1.7.91

- fix(WebConsole): RGB 闪烁颜色切换键悬停不再变形——去掉悬停强制独立椭圆，按钮保持连体段形状，仅背景提亮
  - `libraries/mus4_web/src/WebConsoleAssets.h`：删除 `#ledBlinkTabs button:hover{border-radius:999px!important;z-index:1}`（悬停时 `!important` 圆角覆盖 JS 连体圆角，导致悬停按钮鼓成独立椭圆）；随之删除专为垫椭圆圆角缝隙而生的两条桥接伪元素规则（`:has(+button.active:hover)::after` / `.active:hover+button.active::before`）及三条对应配色规则；悬停背景提亮（#ff9797/#74e4ad/#8bdcff）与连体 box-shadow 机制不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.91（避让：并行会话 #78 终端标签命名已占用 v1.7.90）。
  - 测试同步：`tests/test_firmware_feature_flags.py` 相关断言改为不存在断言，版本链延伸至 v1.7.91。
  - 验证：编译通过；全量 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 确认。

## 2026-08-15 v1.7.90

- 固件版本号从 `v1.7.89` 更新到 `v1.7.90`（避让：并行会话 #77 RGB 切换键总高修正已占用 v1.7.89）。
- feat(WebConsole): Serial 终端标签页名字跟随终端内输入的命令（首个词），由上位机终端页 postMessage 上报
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 新增 `window.addEventListener('message',...)`：校验 `type==='donkeydrifter.term.name'`，按 `e.source` 匹配 `termList` 里的 iframe（`x.f.contentWindow`），把该标签的 `.termTabLabel` 改为上报名（如输入 `kimi` 回车 → 标签显示 `kimi`；输入 `abc defg hijk` → 显示 `abc`），并同步 `b.title` 悬浮提示。
    - 连续重编号避让自定义名：杀标签后的重编号改为 `x.name||'终端 N'`——已命名的标签保持自定义名，未命名的维持位置编号；term 对象新增 `name` 字段（默认 null）。
  - 配套：DonkeyDrift 仓库 `terminal_static/terminal.html` 新增行捕获（回车提交首词、退格/转义处理、备用屏幕缓冲区 TUI 内不跟踪）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.90。
  - 测试同步：`tests/test_firmware_feature_flags.py` 新增 message 监听/e.source 匹配/自定义名优先重编号断言；版本链延伸至 v1.7.90。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.7.90`。

## 2026-08-15 v1.7.89

- fix(WebConsole): DC 头部 RGB 闪烁颜色切换键（#ledBlinkTabs）总高修正为 34px，与 DD 语言/主题两个切换键完全一致
  - `libraries/mus4_web/src/WebConsoleAssets.h`：容器规则由 `#ledBlinkTabs{height:auto;padding:4px 2px}` 改为 `#ledBlinkTabs{height:34px;padding:4px 2px}`——DD 切换键总高 34px（按钮 24px + 容器上下各 4px 内边距 + 上下各 1px 边框），本容器继承 `.langTabs` 的 `box-sizing:border-box` 后以固定 34px 对齐（inset 投影描边不另占高度）；按钮 24px、连体椭圆机制与红绿蓝配色（#ff6b6b/#39d98a/#5cc8ff）均不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.89。
  - 测试同步：`tests/test_firmware_feature_flags.py` 更新容器高度断言与注释，版本链延伸至 v1.7.89。
  - 验证：编译通过；全量 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 确认。

## 2026-08-15 v1.7.88

- feat(WebConsole): 工具行垃圾桶按钮移除；➕ 新建终端按钮移入标签条右端（始终位于最新标签右边）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 删除工具行 `#clearBtn` 垃圾桶按钮及 `onClearBtn()`/`killActiveTerminalTab()` 包装（Serial 模式关终端统一用标签上的 ×，v1.7.87 已支持按 id 关任意标签）；Web 模式的"清空日志"按钮随之一并移除（`clearLog()` 函数保留未删）。
    - `#newTermBtn`（➕）从工具行移入 `#termTabs` 作为其末位子元素；`addTerminalTab()` 改用 `termTabs.insertBefore(b,newTermBtn)`，新标签插到 ➕ 左边，➕ 始终位于最新标签右边；新增 `#termTabs .iconButton{width:22px;height:22px}` 与 22px 标签等高对齐。
    - `applyCmdTarget()` 删除 `clearBtn.title` 随模式切换逻辑；i18n 移除 `terminal.kill` 中英词条（`button.clear` 词条保留，图表工具条清空按钮仍在用）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.88。
  - 测试同步：`tests/test_firmware_feature_flags.py` 更新工具行 DOM 断言（➕ 在 `#termTabs` 内、无 `#clearBtn`），新增 `termTabs.insertBefore(b,newTermBtn)` 及 `onClearBtn`/`killActiveTerminalTab`/`terminal.kill` 不存在断言；版本链延伸至 v1.7.88。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.7.88`。

## 2026-08-15 v1.7.87

- feat(WebConsole): Serial 终端标签页加独立关闭叉 ×、浅色皮肤；工具行按"Serial/Web 开关 → ➕ → 🗑 → 标签条"重排
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 工具行 DOM 重排：`#cmdTarget`（Serial/Web 切换）移到最左，右侧依次 ➕ `#newTermBtn`、🗑 `#clearBtn`、`#termTabs` 标签条；`#pauseBtn`/`#sendBtn`/`#cmd`（仅 Web 模式可见）排最后，Web 模式下开关同样最左。
    - 每个终端标签左侧新增 × 关闭钮（`.termTabClose` 子元素 + `terminal.closeTab` 中英词条）：点击 × 调 `killTerminalTab(id)` 按 id 关闭对应终端——不再只能杀当前选中标签；× 点击 `stopPropagation`，不触发标签切换。`killActiveTerminalTab()` 改为 `killTerminalTab(termActive)` 包装，垃圾桶行为不变。
    - 标签文字拆为 `.termTabLabel` 子 span（原 `b.textContent=` 赋值会抹掉 × 子元素），v1.7.80 的连续重编号改写 label span；× 关闭任意标签后同样触发重编号（杀掉终端1后终端2 自动改名终端1，以此类推）。
    - 标签浅色皮肤：`html[data-theme="light"]` 下新增 `.termTab`（白底深字）、`.termTab.active`（蓝字浅蓝底 `#eaf3fb`/`#0b6bcb`）、`.termTabClose:hover`、`.terminalHint` 四条覆写；终端画布区域（`#terminalWrap`/`.termFrame`/iframe 内 xterm）保持深色不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.87。
  - 测试同步：`tests/test_firmware_feature_flags.py` 更新工具行 DOM 顺序断言、`.termTabLabel`/重编号断言，新增 × 关闭钮、`killTerminalTab(id)`、浅色覆写、`terminal.closeTab` 词条断言；版本链延伸至 v1.7.87。
  - 已 OTA 刷至车辆（192.168.3.46）验证：`/api/status` 返回 `version=v1.7.87`。

## 2026-08-15 v1.7.86

- 固件版本号从 `v1.7.85` 更新到 `v1.7.86`（避让：并行会话已合入 #68/#69/#72/#73/#74 占用开发期间使用的号段，合入前统一改号）。
- fix(WebConsole): DC 头部 OTA 按钮与 DEV 开关按原比例加宽（保持 34px 高），"DEV" 文字移到开关滑珠上
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 DC 主页）：
    - OTA 按钮：34px 高不变，新增 `.headerRow .otaLink .otaButton{font-size:16px;padding:0 14px}`——字号 11→16px、水平内边距 10→14px，按 24→34px 同比例（≈1.42）放大，不再窄高。
    - DEV 开关：轨道 44×34px 改回原始宽高比 → `width:62px;height:34px`（24px 时的 44:24≈1.83）；滑珠 26px、边距 4px 不变，选中位移改 `translateX(28px)`（=62-26-4-4）。
    - "DEV" 写在开关上：滑珠伪元素加 `content:"DEV"` + flex 居中（9px 粗体深色字，白珠上读感清晰），随滑珠左右移动。
    - 删除开关旁文字标签 `<span class="toggleLabel devHint">DEV <b id="devModeSwitchText">OFF</b></span>`；`devHint` 提示气泡移到 `<label>` 上并调整弹出位置（`top:36px`）；JS 同步删除 `devModeSwitchText` 的 const 与两处 `textContent` 更新（否则空指针报错）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.86。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.86；DEV 标签断言改为新结构 + `devModeSwitchText` 不存在；开关规则断言更新为 62px 宽/28px 位移/滑珠文字版。
  - 验证：编译通过；全量 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 的 `version=v1.7.86` 确认。
- 涉及文件：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`、`MUS4_FW/libraries/mus4_core/src/BuildInfo.h`、`MUS4_FW/tests/test_firmware_feature_flags.py`

## 2026-08-15 v1.7.85

- 固件版本号从 `v1.7.84` 更新到 `v1.7.85`（同 v1.7.86 条目避让说明，合入前统一改号）。
- fix(WebConsole): DC 头部 OTA 按钮与 DEV 开关高度提至 34px，与三个"打开"按键（及 DD 侧按键）同高
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 DC 主页）：
    - 34px 专属规则扩展为 `#enterDonkeyBtn,#enterDonkeyDrifterBtn,#openKimiCodeWebBtn,.headerRow .otaLink .otaButton{height:34px}`，覆盖头部 OTA 按钮（无 id，经 `.otaLink` 容器定位，不动 `.otaButton` 基础规则）。
    - DEV 开关按 `#devModeToggle`  scoped 加高：轨道 `height:34px`（宽度 44px 不变），滑珠等比放大至 26px、边距 4px（`bottom:4px` 保持垂直居中），选中位移相应改为 `translateX(10px)`（=44-26-4-4）。页面其它 `.toggleSwitch` 不受影响。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.85。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.85；34px 规则断言同步扩展选择器；补 DEV 开关加高规则断言。
  - 验证：编译通过；全量 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 确认（开发期间用旧号段，合并版随 v1.7.86 复验）。
- 涉及文件：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`、`MUS4_FW/libraries/mus4_core/src/BuildInfo.h`、`MUS4_FW/tests/test_firmware_feature_flags.py`

## 2026-08-15 v1.7.84

- 固件版本号自 Tony 顶端 `v1.7.83` 更新到 `v1.7.84`（同 v1.7.86 条目避让说明，合入前统一改号）。
- fix(WebConsole): DC 标题行整行改为垂直居中——#65 把三个"打开"按键提至 34px 后，底部对齐（`align-items:flex-end`）让图标/标题/GitHub 图标/版本号/静音键沉底，现改为 `align-items:center`（手机版媒体查询本就是 center，桌面版对齐之）
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 DC 主页 `WIFI_WEB_CONSOLE_HTML`，DRIFT 页不动）：`.headerRow` 的 `align-items:flex-end` 改为 `center`；`.version` 与 `.ghLink` 去掉为底部对齐补偿用的 `transform:translateY(-1px)`；`.headerLogo` 本有 `align-self:center`、`.muteButton` 无偏移，随整行居中自然生效。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.84。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.84；头部布局断言更新为 `align-items:center` 及无 `translateY` 的 `.version`/`.ghLink` 规则。
  - 验证：编译通过；全量 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 确认（开发期间用旧号段，合并版随 v1.7.86 复验）。
- 涉及文件：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`、`MUS4_FW/libraries/mus4_core/src/BuildInfo.h`、`MUS4_FW/tests/test_firmware_feature_flags.py`

## 2026-08-15 v1.7.83

- 固件版本号从 `v1.7.82` 更新到 `v1.7.83`（避让：v1.7.82 已被 #73 `Tony-dc-rgb-tabs-dd` 并入 Tony；更早避让历史：v1.7.78 曾被 #68 `Tony-dc-lang-switch-dd` 占用、v1.7.79/v1.7.80 曾被 #69 `Tony-serial-term-tabs`/#72 `Tony-serial-tab-renumber` 占用，本分支原 v1.7.76/v1.7.77/v1.7.81 条目随合入最新 Tony 合并为本条并定版 v1.7.83）。
- feat(WebConsole): DC 主题切换键改为与 DonkeyDrifter 相同的分段控件样式，并补浅色模式浅色变体
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 头部主题切换 `<span id="themeTabs">` 的 class 从 `langTabs` 改为独立 `themeSwitch`，不再复用语言切换键的胶囊样式；按钮、`onclick`、`data-theme`、i18n 属性与 `renderThemeTabs()` 等 JS 逻辑全部保持不变。
    - 新增 5 条 `#themeTabs` 基础 CSS（id 选择器压过浅色主题 `button` 覆写）：容器 `display:inline-flex` + `gap:4px` + `background:#27272a`（zinc-800）+ `border:1px solid #3f3f46`（zinc-700）+ `border-radius:999px` + `padding:4px` + `height:34px`；按钮 `padding:4px 12px`、`height:24px`、`font-size:12px`、`color:#a1a1aa`（zinc-400）、hover 仅文字变 `#e4e4e7`（zinc-200）；选中段 `background:#0891b2`（cyan-600）+ 白字实心圆角胶囊。总高 = 24px 按钮 + 8px padding + 2px border = 34px，与 DD ThemeSwitcher 一致。
    - 追加 5 条 `html[data-theme="light"]` 浅色覆写（优先级天然高于基础规则；`dataset.theme` 是 `applyTheme()` 写入的解析后主题，跟随系统时也会正确落到 light/dark）：容器 `background:#dde3ec`、`border-color:#aeb9c7`，未选中按钮 `color:#5b6b7d`、hover 文字 `#0b2536`（均取自 DC 浅色 `.langTabs` 既有令牌）；选中段两种主题均保持 cyan-600 白字。浅色页面下不再是突兀的深色胶囊。
    - 既有移动端媒体查询 `#themeTabs{order:15}` 保留不动，新规则为叠加；主题默认逻辑未动（首屏防闪烁脚本、`let uiTheme='auto'`、`readStoredTheme()` 回退 `'auto'`，默认仍跟随系统）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.83。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_theme_toggle` 更新描述并补 `class="themeSwitch"`、5 条基础 CSS 与 5 条浅色覆写断言；版本号断言同步至 v1.7.83。
  - 验证：编译通过；全量 pytest 通过；HTTP OTA 上传后车上 `/api/status` 确认 `version=v1.7.83`；Playwright 无头实测浅色模式容器 rgb(221,227,236)/边框 rgb(174,185,199)/未选中 rgb(91,107,125)，深色模式 rgb(39,39,42)/rgb(63,63,70)/rgb(161,161,170) 不回归，两种模式选中段均 cyan-600 白字、高度均 34px，深浅色截图确认。

## 2026-08-15 v1.7.82

- 固件版本号定版 `v1.7.82`（避让：v1.7.80 已被 #72 `Tony-serial-tab-renumber` 并入 Tony，v1.7.81 已被 `Tony-dc-theme-switch-dd` 分支占用），自 Tony 顶端 `v1.7.79` 更新而来。
- fix(WebConsole): RGB 闪烁颜色切换键加高至与 DonkeyDrifter 深浅色/中英文切换键一致（总高 32px），连体椭圆与配色不变
  - `libraries/mus4_web/src/WebConsoleAssets.h`：新增 `#ledBlinkTabs{height:auto;padding:4px 2px}` 覆盖共享 `.langTabs` 容器（24px 按钮 + 上下各 4px 内边距 = 32px，对齐 DD `ThemeSwitcher`/`LanguageSwitcher` 的 p-1 + py-1 尺寸）。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_led_blink_color_selector` 新增容器规则断言。
## 2026-08-15 v1.7.80

- 固件版本号从 `v1.7.79` 更新到 `v1.7.80`。
- feat(WebConsole): Serial 终端标签按位置连续编号——杀掉某个标签后，其后的标签自动重编号（如杀"终端 1"后原"终端 2"变为"终端 1"，以此类推）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `addTerminalTab()`：新标签文字由内部自增 id 改为位次 `termList.length+1`（id 仍仅作选中标识内部使用）。
    - `killActiveTerminalTab()`：`splice` 后对 `termList` 按位次重排全部标签文字（`终端 N`/`Term N` 随 i18n 词条语言）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.80。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.80；serial 终端测试新增连续编号两条断言（新建按位次、杀后重编号）。
  - 验证：全量 pytest 通过；编译通过并 HTTP OTA 上传，车上 `/api/status` 确认 `version=v1.7.80`。

## 2026-08-15 v1.7.79

- 固件版本号从 `v1.7.78` 更新到 `v1.7.79`（避让：v1.7.78 已被 #68 `Tony-dc-lang-switch-dd` 的中英文切换键分段胶囊改动占用）。
- feat(WebConsole): Serial 终端改为浏览器式标签页——➕ 新建终端标签页（每标签独立 iframe/PTY 会话），🗑 在 Serial 模式关闭当前选中标签页
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 工具行 `#cmdTarget` 下拉后新增 `#termTabs` 标签条（`display:flex`+`overflow-x:auto`，仅 Serial 模式显示）；每个终端一个 `.termTab` 按钮（文字 `终端 N`/`Term N`，选中态 `.active` 高亮）。
    - `#newTermBtn` 的 onclick 改为 `addTerminalTab()`：在 `#terminalWrap` 内动态创建 `.termFrame` iframe（保留 `scrolling="no"` 与 `display:block` 等 #57 白边修复属性，CSS 选择器由 `#terminalFrame` 改为 `.termFrame`），复用 `terminalUrl()` 与 launcher `/api/status` 探活逻辑（失败在 `#terminalHint` 显示 `terminal.unreachable`），创建后自动选中；删除 `openNewTerminal()` 与 `window.open` 新开浏览器标签逻辑，移除静态 `<iframe id="terminalFrame">`。
    - 新增 `selectTerminalTab(id)`：只显示选中标签的 iframe（其余 `display:none` 但保持存活，PTY 不断），同步 `active` 样式与 hint；新增 `killActiveTerminalTab()`：移除当前选中 iframe（DOM 移除即关 WebSocket，launcher 自动杀 PTY 子进程），选中相邻标签，杀光后清空标签条并显示 `terminal.empty` 提示。
    - `#clearBtn` 的 onclick 改为 `onClearBtn()` 分发：Serial 模式调 `killActiveTerminalTab()`，Web 模式仍调 `clearLog()`；按钮 title 随模式在 `applyCmdTarget()` 中切换（Serial 用 `terminal.kill`，Web 用 `button.clear`）。
    - `applyCmdTarget()`：新增 `termTabs` display 切换；首次进入 Serial（`termInited` 标志）自动 `addTerminalTab()` 建第一个标签，用户杀光全部标签后切换模式回来不自动重建（保持空态+提示）。
    - i18n：`terminal.new` 改为"新建终端标签页"/"New terminal tab"；新增 `terminal.tab`（终端/Term）、`terminal.kill`（关闭当前终端标签页）、`terminal.empty`（终端已关闭，点 ➕ 新建）中英词条。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.79。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.79；serial 终端测试同步标签页新结构（`termTabs`/`addTerminalTab`/`selectTerminalTab`/`killActiveTerminalTab`/`onClearBtn`、`termInited` 首次自动建标签、动态 iframe `scrolling="no"`、`.termFrame` CSS 白边回归、i18n 四词条中英、`openNewTerminal`/`window.open(terminalUrl` 不复存在）；工具行 HTML 断言同步。
  - 验证：全量 pytest 通过，内嵌 script 块 node --check 通过；编译通过并 HTTP OTA 上传，车上 `/api/status` 确认 `version=v1.7.79`。

## 2026-08-15 v1.7.78

- 固件版本号从 `v1.7.77` 更新到 `v1.7.78`（开发期间曾用 v1.7.76/v1.7.77；两号已分别被 #65/#66 占用，合入前统一改号 v1.7.78）。
- feat(WebConsole): 中英文切换按键改为与 DonkeyDrifter web_ui LanguageSwitcher 完全一致的分段胶囊控件（含高度）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 头部语言 span 从共享类 `.langTabs` 拆分为独立类 `.langSwitch`（`#ledBlinkTabs`/`#themeTabs` 仍复用 `.langTabs`，样式与逻辑不受影响）；原指向"无 id langTabs"的窄屏定位规则（820px 媒体查询内 `.headerRow .langTabs:not([id]){order:16;margin-left:auto}`）改指向 `.langSwitch`。
    - 容器样式对齐 DD LanguageSwitcher：`background:#27272a`、`border:1px solid #3f3f46`、`border-radius:9999px`、`padding:4px`，总高恰好 34px（1px 边框+4px padding+24px 按钮+4px padding+1px 边框）。
    - 按钮：`font-size:12px`/`line-height:16px`、`padding:4px 12px`、透明背景无边框；激活 `background:#0891b2` 白字（hover 不变），未激活 `#a1a1aa`、hover `#e4e4e7` 且背景保持透明；`aria-pressed` 随激活态同步（`applyLanguage` 激活态同步逻辑顺带带上）。
    - 浅色主题按 DD `theme-light.css` 的 zinc/cyan 重映射逐色换算（容器 `#f4f6f9` + 边框 `#ccd5df` + 内描边 `#d5dce4`，激活段 `#5cc8ff` + 近黑字 `#061019` + 800 粗，未激活 `#5b6b7d`/hover `#1a2330`），与 DD 浅色表现一致；暗色保持 DD 深色原色不变。修正首版误把深色配色钉死到浅色主题导致的"浅色页面深色按钮"问题。
    - 右下角 fab 悬浮簇（fabToggle 光点/langFab 🌐/helpFab ?/langMenu 下拉）与 DD FabActions 本为 1:1 镜像，一律未动。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.78。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.78；`test_web_console_language_tabs_wired_to_set_language` 语言 span 断言从 `.langTabs` 改为 `.langSwitch`，并新增关键样式断言（`.langSwitch{`/`height:34px`/`#0891b2`/`#27272a`/`aria-pressed`，及浅色主题重映射色值三条）；窄屏头部布局断言 `.headerRow .langTabs:not([id])` 同步改指 `.langSwitch`；`#ledBlinkTabs`/`#themeTabs` 及右下角 fab/langFab/langMenu 相关断言一律不动。
  - 验证：编译通过；全量 322 项 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 的 `version=v1.7.78` 确认，首页 HTML 实测返回 `.langSwitch` 新样式（`height:34px`、`#27272a`/`#0891b2` 深色配色、`html[data-theme="light"]` 浅色重映射 `#f4f6f9`/`#5cc8ff` 系列）与按钮 `aria-pressed` 属性，旧 `langTabs:not([id])` 选择器已清除，#66 的 `#newTermBtn` 等新功能在同基上完整保留。
  - 涉及文件：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`、`MUS4_FW/libraries/mus4_core/src/BuildInfo.h`、`MUS4_FW/tests/test_firmware_feature_flags.py`

## 2026-08-15 v1.7.77

- 固件版本号从 `v1.7.76` 更新到 `v1.7.77`（避让：v1.7.76 已被 #65 `Tony-kimi-code-web` 的 DC 头部按键高度改动占用）。
- feat(WebConsole): Serial 终端工具行精简——隐藏输入框/发送/暂停键，新增"➕新开终端窗口"按钮
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - 工具行新增 `#newTermBtn`（加号图标，`onclick="openNewTerminal()"`，i18n `terminal.new` 中英词条）；`openNewTerminal()` 复用 `terminalUrl()` 的 `_launcherIp` 自动发现机制，`window.open(..., '_blank')` 新开浏览器标签页加载上位机终端页——每个标签页独立 PTY 会话，**不杀已有终端窗口**。
    - `applyCmdTarget()`：Serial 模式显示加号按钮、隐藏暂停/发送/输入框（垃圾桶与目标下拉保持显示）；Web 模式反之，排版与既有行为不变。
    - 元素常量表新增 `newTermBtn`。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.77。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.77；serial 终端切换测试更新为"Serial 隐藏暂停/发送/输入框、显示加号按钮"新行为断言；工具行 HTML 断言同步加号按钮。
  - 验证：全量 322 项 pytest 通过，内嵌 5 个 script 块 node --check 通过；编译通过并 HTTP OTA 上传，车上 `/api/status` 确认 `version=v1.7.77`。

## 2026-08-15 v1.7.76

- 固件版本号从 `v1.7.75` 更新到 `v1.7.76`。
- fix(WebConsole): DC 头部三个"打开"入口按键高度由 24px 提至 34px，与 DonkeyDrifter 侧"打开"按键对齐（DonkeyDrift 仓库 PR #110 已把 DD 按键对齐到其中英文切换键的 34px，本侧跟随）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：新增 `#enterDonkeyBtn,#enterDonkeyDrifterBtn,#openKimiCodeWebBtn{height:34px}` 规则（紧跟 `.otaButton` 基础规则之后），只覆盖头部三个入口按键，其余 `.otaButton`（如 OTA 按钮）保持 24px 不变。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.76。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.76；`test_web_console_header_entry_buttons` 补 34px 高度规则断言。
  - 验证：编译通过；全量 pytest 通过；已 HTTP OTA 上传并以车上 `/api/status` 的 `version=v1.7.76` 确认。
- 涉及文件：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`、`MUS4_FW/libraries/mus4_core/src/BuildInfo.h`、`MUS4_FW/tests/test_firmware_feature_flags.py`

## 2026-08-15 v1.7.75

- 固件版本号从 `v1.7.73` 更新到 `v1.7.75`（跳过 v1.7.74：该号已被 #61 `Tony-kimi-code-web` 占用）。
- fix(WebConsole): DC 终端板块 Serial 页面排版对齐 Web 模式，并删除 Serial 模式右侧白色竖条（Closes #57）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `applyCmdTarget()` 不再隐藏工具行：Serial 模式下暂停/清空(垃圾桶)/发送按钮与 `#cmd` 输入框保持显示，排版与 Web 模式一致（仅排版对齐，未新增功能）。
    - `#terminalWrap` 尺寸约束改为与 `#log` 完全相同（flex basis 与 min/max-height、padding 逐项对齐），serial↔web 切换时整个 `#serialPanel` 尺寸不再跳动。
    - 白色竖条三处联防：`#terminalFrame` iframe 加 `scrolling="no"` 与 `display:block`，`#terminalWrap` 加 `overflow:hidden` + 深色背景 + 圆角，消除 iframe 原生浅色滚动条与边缘缝隙两条白条路径。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.75。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.75；serial 切换相关过时中文注释同步（不再隐藏日志控件），未改测试断言逻辑。
  - 验证：编译通过；全量 322 项 pytest 通过，内嵌 5 个 script 块 node --check 通过；开发期间已 HTTP OTA 上传并在设备端确认新 CSS/`scrolling="no"`/新 `applyCmdTarget` 三处生效（当时沿用 v1.7.73 号段，以 build 时间戳确认）。注：开发验证后车上固件被 #61 的 v1.7.74 试刷覆盖，本条目合入时将 v1.7.75 完整固件（含 #61 按钮功能）重新 OTA 并以车上 `/api/status` 的 `version=v1.7.75` 确认。

## 2026-08-15 v1.7.74

- 固件版本号从 `v1.7.73` 更新到 `v1.7.74`。
- feat(WebConsole): "打开 Kimi Code Web" 按钮接上 launcher 启动端点（issue #59；配套 DonkeyDrift 侧 #103/#104）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `#openKimiCodeWebBtn` 从纯占位改为 `onclick="openKimiCodeWeb()"`；新增 `openKimiCodeWeb()` JS（插在 `_fetchLauncherIp()` 之后）：防重复点击守卫 → 点击同步上下文先 `window.open('about:blank','_blank')` 拿句柄（规避弹窗拦截）→ 按钮禁用并切"正在启动 Kimi Code Web..."文案 → `fetch` POST `http://<host_ip>:8090/api/launch/kimi-code-web`（空体 simple request 免 CORS 预检；`AbortController` 120s 超时，匹配服务端整体超时）→ 成功校验 `{status:'ok',url}` 后 `newTab.location.href=url`；失败/超时关句柄并 `showToast` 提示（超时单独文案）、`line()` 落日志；`finally` 恢复按钮。
    - 上位机地址沿用既有 `_launcherIp` 机制（`/api/status` 的 `host_ip` 字段自动发现，与"打开 DD"按钮同源），不发明新通道。
    - i18n 新增 zh/en 各 3 条：`button.openKimiCodeWebLaunching`、`toast.kimiCodeWebFailed`、`toast.kimiCodeWebTimeout`。
    - 跨域依赖：launcher 端点响应须带 `Access-Control-Allow-Origin`（DonkeyDrift 侧已在本端点放行，仅此端点），否则浏览器拦截响应。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.74。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_header_entry_buttons` 补按钮行为断言；版本号断言同步至 v1.7.74（161 项全过）。
  - 已编译并 HTTP OTA 上传验证：车上 `/api/status` 确认 `version=v1.7.74`，首页 HTML 确认 `onclick="openKimiCodeWeb()"`、`/api/launch/kimi-code-web` 与新词条已生效。
  - 涉及文件：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`、`MUS4_FW/libraries/mus4_core/src/BuildInfo.h`、`MUS4_FW/tests/test_firmware_feature_flags.py`

## 2026-08-14 v1.7.73

- 固件版本号从 `v1.7.72` 更新到 `v1.7.73`。
- docs(repo): 重写仓库根 README.md
  - 子项目表标注各子项目当前版本（MUS4_FW 固件版本与 BuildInfo.h 对齐）。
  - 新增 MUS4_FW 功能速览（无线控制台/Web Console/OTA/校准等核心能力一览）。
  - 新增构建、OTA、测试命令速查（`arduino-cli.py -c`、HTTP/ArduinoOTA 上传、`pytest` 入口）。
  - 新增安全说明（控制台密码、DEV 模式、免认证边界等注意事项）。
  - 补充与 DonkeyDrift 上位机仓库的配套链接。
  - 纯文档 + 版本号改动，无固件逻辑变更。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.73。
  - `tests/test_firmware_feature_flags.py`：版本号断言同步至 v1.7.73。

## 2026-08-14 v1.7.72

- 固件版本号从 `v1.7.71` 更新到 `v1.7.72`。（开发期间曾用 v1.7.68/70/71；因 v1.7.67~v1.7.69 已被 #49/#50 占用、v1.7.70 已被 #54 占用，合入前统一改号）
- feat(WebConsole): cmdTarget 下拉框恢复 Serial 选项并升级为上位机终端（xterm.js），Serial 排第一且为默认目标，选择持久化到 localStorage
  - `libraries/mus4_web/src/WebConsoleAssets.h`：
    - `<select id="cmdTarget">` 恢复 `<option value="serial">Serial</option>`（排第一位），`<option value="web">Web</option>` 移至第二；不加 serial1 选项（后端 v1.7.29 起 serial/serial1 均转发 Serial2，重复暴露会误导）。
    - 选中 Serial 时日志区切换为 iframe 嵌入的上位机终端页（新增 `#terminalWrap`/`#terminalFrame`/`#terminalHint` 及对应 CSS），终端页 URL 为 `http://<host_ip>:8090/terminal`，由上位机 Launcher 服务提供 xterm.js + WebSocket↔PTY 桥，浏览器里得到上位机完整 bash 终端（kimi/claude/codex/donkey 等全屏 TUI 程序可用）。终端数据走局域网 WebSocket，**不走 115200 波特的 Serial2**（带宽不足以支撑全屏 TUI 重绘）；Serial2 链路维持 `WIFI|` 配网协议与既有 target=serial 转发不变。
    - 上位机地址复用既有 `_launcherIp` 机制（`/api/status` 的 `host_ip` 字段自动发现，缺省 192.168.3.41）；加载 iframe 前先 no-cors 探测上位机 8090 可达性，不可达时显示 i18n 提示（新增 `terminal.loading`/`terminal.unreachable` 中英词条）。
    - 新增 `applyCmdTarget()`/`startTerminal()`/`restoreCmdTarget()`：选中 Serial 时隐藏日志控件（暂停/清空/发送/输入框，清空按钮新增 `id="clearBtn"`）并显示终端，切回 Web 恢复日志视图；选择写入 localStorage 键 `donkeydrifter.ui.cmdTarget`，页面加载时恢复（无记录默认 Serial）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.72。
  - `tests/test_firmware_feature_flags.py`：serial 选项断言翻正并新增"Serial 排在 Web 之前"顺序断言；清空按钮行内 HTML 断言同步 `id="clearBtn"`；新增 `test_web_console_serial_option_is_host_terminal_with_persistent_default` 覆盖终端容器/URL 自动发现/选择持久化/切换逻辑/i18n 词条；版本号断言同步至 v1.7.72。
  - 配套上位机改动在 DonkeyDrift 仓库 `Tony-serial-terminal` 分支（`donkeycar/launcher/terminal.py` WebSocket↔PTY 桥 + `terminal_static/` xterm.js 终端页 + Launcher `/terminal` 路由）。
  - 固件侧不新增后端代码，无 flash 压力（xterm.js 由上位机伺服）。
  - 已编译并 OTA 上传验证：车上 `/api/status` 确认 `version=v1.7.72`、`host_ip=192.168.3.41`；首页 HTML 确认 Serial 选项（第一位）与 `#terminalWrap` 容器已生效；上位机侧 `ws://192.168.3.41:8090/terminal/ws` 实测命令回显/窗口缩放/Ctrl-C/shell 退出通知全通；Playwright 无头浏览器对车上真实页面端到端复测：默认选中 Serial、选项顺序 serial→web、终端 iframe 自动加载并挂载 xterm、键入命令真实执行回显、切换 Web 恢复日志视图、localStorage 记住选择且刷新后恢复。

## 2026-08-14 v1.7.71

- 固件版本号从 `v1.7.70` 更新到 `v1.7.71`。（开发/首轮 OTA 期间曾用 v1.7.67；因 v1.7.67~v1.7.69 已被 #49/#50 占用、v1.7.70 已被 #54 占用，合入前统一改号）
- feat(WirelessConsole): 控制台密码为空时全通道免认证，任何工具发命令都不再需要 `AUTH:`
  - `libraries/mus4_core/src/WifiConsoleTypes.h`：新增编译期判定 `isWirelessConsoleAuthDisabled()`（`WIFI_CONSOLE_AP_PASSWORD[0]=='\0'`）。空密码 + 开放 AP 下任何人发 `AUTH:` 都必然成功，门禁只剩操作摩擦；一旦配置非空密码，所有门禁自动恢复原语义。
  - `libraries/mus4_command/src/WirelessConsole.cpp`：`isWirelessCommandAllowed` 新增 `authed = ws.consoleAuthenticated || isWirelessConsoleAuthDisabled()`，替代原 4 处 `ws.consoleAuthenticated` 判断；NACK 分流条件同步纳入免认证（Park 未锁时报 `NACK:PARK_REQUIRED`）。
  - `libraries/mus4_wifi/src/WifiOta.cpp`：`openWifiOtaWindow` 的 `NACK:AUTH_REQUIRED` 检查追加 `!isWirelessConsoleAuthDisabled()`。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：5 处 `/api/wifi-*` 配置端点 403 检查追加 `!isWirelessConsoleAuthDisabled()`；`isWifiWebUpdateAuthOk()` 开头新增免认证直通。
  - 安全边界不变：Park 锁定要求（`TEST`/`BENCH`/校准/`ENABLE_OTA` 等）原样保留；`AUTH:` 命令行为不变（空密码返回 `AUTH_OK`）；前端 `error.authRequired` 弹窗与校准密码 prompt 保留为非空密码场景的兜底路径。
  - `wireless_console_policy.py`：`is_wireless_command_allowed` / `is_web_command_allowed` 新增 `auth_disabled=False` 关键字参数，镜像固件免认证判定。
  - `tests/test_wireless_console_policy.py`：新增 `TestWirelessConsoleAuthDisabled` 7 个用例（免认证放行控制/配置/校准/OTA 命令，Park 规则不变，默认 `auth_disabled=False` 语义不变）。
  - `tests/test_firmware_feature_flags.py`：3 处门禁源码断言同步为新文本；`test_wifi_console_types_are_split_from_sketch` 新增 helper 存在性断言；版本号断言同步至 v1.7.71。
  - `docs/Plan/DEV模式影响面与运行逻辑映射.md`：新增 §3.2「控制台密码为空时全通道免认证」；`docs/Valid/无线串口调试验证指南.md`：认证说明补充空密码免认证口径。

## 2026-08-14 v1.7.70

- 固件版本号从 `v1.7.69` 更新到 `v1.7.70`。
- feat(WebConsole): 头部入口按钮"进入"改"打开"（英文 Enter→Open），并在"打开 DonkeyDrifter"右侧新增"打开 Kimi Code Web"占位按钮
  - `libraries/mus4_web/src/WebConsoleAssets.h`：headerRow 两个入口锚文本与 zh/en I18N 词典同步改名（打开 Donkey / 打开 DonkeyDrifter，Open Donkey / Open DonkeyDrifter）；`ghLink` 前插入 `openKimiCodeWebBtn` 占位 `<button type="button" class="otaButton">`（无 href/onclick，功能预留，zh"打开 Kimi Code Web"/en"Open Kimi Code Web"）；窄屏 `@media (max-width:820px)` 规则插入 `#openKimiCodeWebBtn{order:8}`，`.br2` 起后续元素 order 顺移 +1（桌面布局规则逐字不动）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.70。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_header_entry_buttons` 更新位置序断言（h1<donkey<drifter<kimi<gh）与中英词条断言、新增 kimi 词条断言；`test_web_console_mobile_header_layout` 第 2 行新增 kimi order:8、后续 order 顺移断言同步；版本号/日志一致性断言同步至 v1.7.70。全量测试通过。
  - 已编译并 HTTP OTA 上传验证：车上 `/api/status` 返回 `version=v1.7.70`，首页返回 `openKimiCodeWebBtn` 与"打开 Kimi Code Web"文本。

## 2026-08-14 v1.7.69

- 固件版本号从 `v1.7.68` 更新到 `v1.7.69`。
- feat(WebConsole): Drifter Console 窄屏（手机/平板竖屏，≤820px）头部重排为固定 4 行
  - `libraries/mus4_web/src/WebConsoleAssets.h`：新增 `@media (max-width:820px)` 窄屏规则（桌面布局规则逐字不动）。DOM 新增 3 个 `.rowBreak` 分隔 span（桌面 `display:none` 无感）；窄屏下 headerRow 保持 flex-wrap，`.rowBreak` 以 `display:block;flex-basis:100%` 强制换行，各元素以 `order` 重排为 4 行：第 1 行 logo + 标题 + GitHub 图标 + 版本号（紧跟 GitHub 右侧）；第 2 行 进入 Donkey / 进入 DonkeyDrifter；第 3 行 红绿蓝（最左）+ OTA + 静音（桌面右推的 `margin-left:auto` 复位为 0）+ DEV（`margin-left:auto` 贴合页面最右端）；第 4 行 主题切换（最左）+ 语言切换（`margin-left:auto` 贴合最右端）。曾评估 grid 具名区域方案，因标题宽度耦合撑宽共享列导致手机横向溢出，改用 flex + order 分行方案。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.69。
  - `tests/test_firmware_feature_flags.py`：新增 `test_web_console_mobile_header_layout`（rowBreak DOM 位置、4 行 order 分配、静音/DEV/语言切换 margin 处理、版本号不再右推等逐字断言）；版本号断言同步至 v1.7.69。全量 160 项测试通过。
  - 已编译并 OTA 上传验证：车上 `/api/status` 确认 `version=v1.7.69`，首页返回窄屏媒体查询与 3 个 rowBreak 标记。

## 2026-08-14 v1.7.68

- 固件版本号从 `v1.7.67` 更新到 `v1.7.68`。
- feat(WebConsole): Web UI 语言缺省改为跟随浏览器语言自动检测
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：NVS `webui`/`lang` 语言偏好从两态 `"zh"/"en"` 扩展为三态 `"auto"/"zh"/"en"`，缺省（含 NVS 读取失败/值非法兜底）由 `"zh"` 改为 `"auto"`——新设备首次开机不再钉死中文，而是由页面端按浏览器语言解析；`isValidWebUiLang()` 接受 `"auto"`，`POST /api/language?lang=auto` 可把偏好重置回自动检测；`GET /api/language` 在缺省时返回 `{"lang":"auto"}`。已显式切换过语言的设备 NVS 中存的是 zh/en，行为不变（覆盖自动检测、跨重启保持）。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：主控制台 + JUDGE/DRIFT/UPDATE 四个页面的自包含 i18n 核心各新增 `detectBrowserLanguage()`（`navigator.language` 小写后以 `zh` 开头→中文，其余一律→英文，异常兜底中文）；`initLanguage()` 新增 `j.lang==='auto'` 分支——设备偏好为 auto 时用检测结果并写入 localStorage（`mus4.ui.lang`）作离线兜底。手动切换语言仍走 `setLanguage()` POST 显式 zh/en，立即生效并持久化覆盖自动检测。
  - 行为变化：首次使用（NVS 无偏好）时界面语言跟随浏览器——中文浏览器=中文界面（与原默认一致），英文及其他语言浏览器=英文界面；用户一旦手动切换，选择跨关机重启保持，不再受浏览器语言影响。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_language_api_persists_nvs_preference` 断言同步三态与缺省 auto；`test_web_console_sub_pages_follow_device_language` 新增三子页面 `detectBrowserLanguage`/auto 分支断言；新增 `test_web_console_language_auto_detects_browser_language`（四页检测函数与 auto 分支逐页计数断言、主控制台手动切换仍显式 POST 持久化）；版本号断言同步至 v1.7.68。全量 160 项测试通过。
  - 已编译并 OTA 上传验证：车上 `/api/status` 确认 `version=v1.7.69`；本机 NVS 存有显式中文偏好，`GET /api/language` 返回 `{"lang":"zh"}`——显式选择优先于自动检测，行为符合设计；缺省 `auto` 路径由测试断言覆盖。

## 2026-08-14 v1.7.67

- 固件版本号从 `v1.7.66` 更新到 `v1.7.67`。
- feat(WebConsole): Drifter Console 默认主题由深色改回"跟随系统"，与 DonkeyDrifter `web_ui` / Donkey launcher 同口径
  - `libraries/mus4_web/src/WebConsoleAssets.h`：首屏防闪烁内联脚本改为 `if(t!=='light'&&t!=='dark')` 一律经 matchMedia 解析系统主题（无存储/`'auto'`/非法值均跟随系统）；`let uiTheme='dark'` 改回 `let uiTheme='auto'`；`readStoredTheme()` 无存储/异常时的回退值由 `'dark'` 改回 `'auto'`。用户显式点选浅色/深色后仍以存储值为准。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.67。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_theme_toggle` 断言同步（默认值 `'auto'`、首屏内联脚本新形式、docstring 更新）；浅色皮肤测试的"主题骨架"断言同步；版本号断言同步至 v1.7.67。

## 2026-08-14 v1.7.66

- 固件版本号从 `v1.7.65` 更新到 `v1.7.66`。
- feat(WebConsole): Drifter Console 默认主题由"跟随系统"改为深色，仅用户显式点选"跟随系统"后才经 matchMedia 解析/监听系统主题
  - `libraries/mus4_web/src/WebConsoleAssets.h`：首屏防闪烁内联脚本去掉 `||'auto'` 默认值（无存储或存储值非法时直接预置 `data-theme="dark"`，仅存储值为 `'auto'` 时才 matchMedia 解析系统主题）；`let uiTheme='auto'` 改为 `let uiTheme='dark'`；`readStoredTheme()` 无存储/异常时的回退值由 `'auto'` 改为 `'dark'`。`'auto'` 仍是合法存储值，系统主题 change 监听本就只有 `uiTheme==='auto'` 时才生效，该逻辑不变。与 DonkeyDrifter `web_ui` / Donkey launcher（PR DonkeyDrift#85）同口径。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.66。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_theme_toggle` 断言同步（默认值 `'dark'`、首屏内联脚本不再带 `||'auto'`、docstring 更新）；浅色皮肤测试的"主题骨架"断言同步；版本号断言同步至 v1.7.66。
  - 已编译并 OTA 上传验证：车上 `/api/status` 确认 `version=v1.7.66`。

## 2026-08-14 v1.7.65

- 固件版本号从 `v1.7.64` 更新到 `v1.7.65`。
- feat(WebConsole): Drifter Console 顶栏标题左侧新增头盔 logo 图标
  - `libraries/mus4_web/src/WebConsoleAssets.h`：主标题 `<h1>` 前新增 `<img class="headerLogo" src="/favicon.png" alt="Drifter Console">`；新增 `.headerLogo` 样式（32x32、8px 圆角、1px 描边 `#2b3441`、`align-self:center`），与 Donkey 页面（launcher）顶栏 logo 完全同款；浅色主题新增 `html[data-theme="light"] .headerLogo{border-color:#d5dce4}` 覆盖。图标直接复用固件已内嵌的 `/favicon.png`（与 `Projects/logo.png` 逐字节相同的头盔图，md5 一致），不新增二进制资源、固件体积零增长。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号升至 v1.7.65。
  - `tests/test_firmware_feature_flags.py`：新增 `test_web_console_header_logo_left_of_title`（logo 标签、深色/浅色 `.headerLogo` 样式、位于 headerRow 内主标题左侧的位置断言）；版本号断言同步至 v1.7.65。全量 158 项测试通过。
  - 已编译并 OTA 上传验证：车上 `/api/status` 确认新构建，首页返回含 headerLogo 标签，`/favicon.png` 正常服务 13782 字节。

## 2026-08-14 v1.7.64

- 固件版本号从 `v1.7.63` 更新到 `v1.7.64`。
- feat(WebConsole): RGB LED 灯色切换框按各自颜色着色
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`#ledBlinkTabs` 三个选项的选中背景从统一蓝色 `#5cc8ff` 改为各自颜色——红 `#ff6b6b`、绿 `#39d98a`、蓝 `#5cc8ff`（与图表 gz/thr/str 曲线同色）；悬停提亮同步按各自颜色（红 `#ff9797`、绿 `#74e4ad`、蓝沿用 `#8bdcff`）；连体 `box-shadow` 与悬停垫底伪元素的延伸色也按按钮各自颜色（新增 `LED_BLINK_TAB_COLORS` 映射），语言切换等其他 `.langTabs` 保持原蓝色不变。
  - `tests/test_firmware_feature_flags.py`：断言同步更新（版本号升至 v1.7.64）。
- feat(WebConsole): Drifter Console 新增浅色主题，头部主题切换按钮（浅色/跟随系统/深色）真正生效
  - `libraries/mus4_web/src/WebConsoleAssets.h`（仅 Console 页区域，深色规则原文逐字不动，DRIFT/JUDGE/UPDATE 三页不受影响）：
    - 新增第三个 `<style>` 块：85 条 `html[data-theme="light"]` 浅色覆盖规则 + `@keyframes pulseLight`。设计语言对标深色：青胶囊身份（`#5cc8ff` 底 + `#061019` 字）与青色辉光保留；文字/边框/状态语义色（成功/警告/错误/漂移紫）等比加深适配白底；日志终端改浅底深绿字；遮罩改浅。
    - JS 新增 `CHART_THEMES` 双主题色表与 `systemTheme()`/`resolvedTheme()`/`applyTheme()`：`setTheme`/`initTheme` 把解析结果写入 `<html data-theme>` 并使 canvas 网格缓存失效重绘；canvas 网格/坐标轴/三条曲线/屏保文字与 toast 边框色改从色表取值。"跟随系统"（auto）经 `matchMedia('(prefers-color-scheme: light)')` 实时解析，并监听系统主题 change 自动跟随；`<head>` 内防闪烁内联脚本同样经 matchMedia 解析。
    - 浅色特异性修正（浅色通用 `button` 白底规则特异性高于深色透明/填充规则导致的净效果）：`.langTabs button` 恢复 `background:transparent`（语言/主题/LED 三组胶囊未激活段与缝隙同色贴合）；`.otaButton`/`.rcSetBtn`/`.fabToggle` 保持青色填充；`.muteButton`/`.rcNum` 恢复透明；`.langTabs` 容器底色 `#dde3ec`、描边 `#aeb9c7`，使激活胶囊与外容器的嵌套轮廓在浅底下与深色同样清晰。
  - `tests/test_firmware_feature_flags.py`：`test_web_console_theme_toggle` 扩展跟随系统 matchMedia 断言；新增 `test_web_console_light_theme_overrides`（浅色覆盖规则、`systemTheme`/`resolvedTheme`/`applyTheme` 接线、`CHART_THEMES` 色表、原主题骨架不回退等逐字断言）；版本号断言同步至 v1.7.64。全量 157 项测试通过。
  - 已编译并 OTA 上传验证：车上 `/api/status` 确认新版本，实际服务页面含浅色覆盖规则。

## 2026-08-12 v1.7.63

- 固件版本号从 `v1.7.62` 更新到 `v1.7.63`。
- fix(WebConsole): 修复 RGB LED 灯色切换时椭圆形容器尺寸抖动
  - 背景：`renderLedBlinkTabs()` 在相邻按钮激活时动态设置 `marginLeft:-2px` 抵消 flex `gap:2px`，取消某色后 margin 重置导致 gap 恢复，容器总宽度变化 2~4px。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：移除 `marginLeft` 动态修改，改用 `box-shadow` 向相邻方向延伸 2px 填充 gap（`#5cc8ff`），视觉上无缝合并且不影响布局尺寸。`borderRadius` 直角处理保持不变，box-shadow 在直角边上形成干净矩形填充。
  - `tests/test_firmware_feature_flags.py`：断言同步更新（`style.boxShadow` 替换 `style.marginLeft`，版本号升至 v1.7.63）。

## 2026-08-12 v1.7.62

- 固件版本号从 `v1.7.61` 更新到 `v1.7.62`。
- fix(WebConsole): "进入 DonkeyDrifter"改用 `#drive` hash 与"进入 Donkey"同路径
  - 背景：Safari 无法加载 `/launch/drive` 路径（curl 正常但 Safari 报"无法连接服务器"），可能与 HTTP/1.0 + 非标准路径有关。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`enterDonkeyDrifterBtn` 的 href 从 `http://<ip>:8090/launch/drive` 改为 `http://<ip>:8090/#drive`，与"进入 Donkey"使用相同的 `/` 路径（仅 hash 不同），由 Launcher 菜单页 JS 检测 `#drive` 并自动触发启动。
  - `tests/test_firmware_feature_flags.py`：断言同步更新。

## 2026-08-12 v1.7.61

- 固件版本号从 `v1.7.60` 更新到 `v1.7.61`。
- fix(WebConsole): 改用 `<a target="_blank">` 原生链接替代 `window.open`，彻底解决 Safari 弹窗拦截
  - 背景：v1.7.60 虽将 `window.open` 改为同步调用，但 iOS Safari 仍会拦截非用户手势触发的 `window.open` 弹窗。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：两个入口按钮从 `<button onclick="window.open(...)">` 改为 `<a href="..." target="_blank" rel="noopener">`，页面加载时由 JS 动态设置 `href`（预取 host_ip）。Safari 原生支持 `<a target="_blank">` 新标签页打开，不会拦截。
  - CSS `.otaButton` 新增 `text-decoration:none;display:inline-flex;align-items:center;cursor:pointer` 适配 `<a>` 标签。
  - `tests/test_firmware_feature_flags.py`：断言同步更新。

## 2026-08-12 v1.7.60

- 固件版本号从 `v1.7.59` 更新到 `v1.7.60`。
- fix(WebConsole): 修复 Safari 弹窗拦截导致"进入 DonkeyDrifter"无法打开新标签页
  - 背景：`enterDonkeyDrifter()` / `enterDonkeyLauncher()` 为 `async` 函数，先 `await getLauncherHostIp()` 再 `window.open()`；Safari 认为 `await` 之后已脱离用户手势上下文，静默拦截弹窗。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：页面加载时预取 host_ip 并缓存到全局变量 `_launcherIp`（`_fetchLauncherIp()` 异步执行，不阻塞渲染）；两个按钮改为同步函数，点击时直接 `window.open('http://'+_launcherIp+':8090/...,'_blank')`，不再有 `await`，Safari 识别为用户手势，不拦截。
  - `tests/test_firmware_feature_flags.py`：断言同步更新（`_launcherIp` / `_fetchLauncherIp` 替换 `getLauncherHostIp`）。

## 2026-08-11 v1.7.59

- 固件版本号从 `v1.7.58` 更新到 `v1.7.59`。
- fix(WebConsole): "进入 DonkeyDrifter"按钮改为新标签页打开 GET /launch/drive 跳转页，修复原先 GET /api/launch/drive（仅接受 POST）导致 404 的问题
  - 背景：原 `enterDonkeyDrifter()` 用 `location.href` 导航到 `/api/launch/drive`（POST-only 端点），GET 请求返回 404；且在当前页跳转会离开 Drifter Console。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：`enterDonkeyDrifter()` 改为 `window.open('http://'+ip+':8090/launch/drive','_blank')`，在新标签页打开 Launcher 的 GET 跳转页，由该页同源 POST `/api/launch/drive` 启动 Drive 并重定向。
- fix(Launcher): 新增 GET `/launch/drive` 端点，返回极简跳转 HTML 页
  - `donkeycar/launcher/server.py`：`do_GET` 新增 `/launch/drive` 路由，返回 `LAUNCH_DRIVE_HTML` 页面；页面加载后自动 fetch POST `/api/launch/drive`（同源），拿到 drive URL 后 `window.location.href` 重定向到 Drive 页面。
- style(WebConsole): "进入 donkey"按钮文本 D 大写为"进入 Donkey"
  - `libraries/mus4_web/src/WebConsoleAssets.h`：HTML 默认文本、zh i18n、en i18n 三处 `donkey` 改为 `Donkey`。
  - `tests/test_firmware_feature_flags.py`：断言同步更新。

## 2026-08-11 v1.7.58

- 固件版本号从 `v1.7.57` 更新到 `v1.7.58`。
- feat(WebConsole): 顶栏恢复固件版本号显示，放在 GitHub 图标右侧
  - 背景：版本号此前被移除，用户要求恢复；Drifter Console 顶栏在 GitHub 图标（跳转 DonkeyDrift/Firmware）右边显示版本号，与 DonkeyDrifter Web UI 头部版本号（GitHub 图标左侧）对应。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：ghLink 的 `</a>` 后新增 `<span class="version" id="versionLabel">--</span>`；JS const 声明新增 `versionLabel=document.getElementById('versionLabel')`；`updateNetworkCard()` 末尾从 `/api/status` 的 `version` 字段写入显示（`V` 前缀归一为 `v`）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.58。
  - `tests/test_firmware_feature_flags.py`：版本断言更新至 v1.7.58；`test_web_console_header_ota_button_and_log_area_are_compact` 中 versionLabel「不存在」断言翻正为「存在」，并新增位置断言（ghLink < versionLabel < muteToggle）。
  - 验证：`pytest tests/test_firmware_feature_flags.py` 通过；编译通过；已 OTA 刷机。

## 2026-08-11 v1.7.57

- 固件版本号从 `v1.7.56` 更新到 `v1.7.57`。
- fix(WebConsole): "进入 donkey"/"进入 DonkeyDrifter" 按钮改为动态获取上位机 IP，不再硬编码
  - 问题：v1.7.56 中按钮 onclick 硬编码 `192.168.3.150`，但宿主机实际 IP 为 `192.168.3.41`（DHCP 动态分配），导致点击按钮后无法打开 Launcher 页面。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：按钮 onclick 改为调用 JS 函数 `enterDonkeyLauncher()` / `enterDonkeyDrifter()`；新增 `getLauncherHostIp()` 函数从 `/api/status` 响应中解析 `host_ip` 字段，回退到 `192.168.3.41`。
  - 配合 DonkeyDrift 侧 Launcher 服务启动时通过串口向 ESP32 发送 `HOSTIP|<ip>` 命令（每 30 秒），使 `/api/status` 动态输出正确的上位机 IP。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.57。
  - `tests/test_firmware_feature_flags.py`：版本断言更新至 v1.7.57；按钮测试改为验证 JS 函数调用和动态 IP 获取逻辑。
  - 验证：`pytest tests/test_firmware_feature_flags.py` 通过；编译通过；已 OTA 刷机。

## 2026-08-11 v1.7.56

- 固件版本号从 `v1.7.55` 更新到 `v1.7.56`。
- feat(WebConsole): "进入 donkey" / "进入 DonkeyDrifter" 头部按钮接上 onclick 跳转，跳转目标为宿主机 Launcher 服务（端口 8090）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：两个按钮分别添加 `onclick` 属性--"进入 donkey"新标签页打开 `http://192.168.3.150:8090/`（Launcher 菜单页面），"进入 DonkeyDrifter"当前页跳转 `http://192.168.3.150:8090/api/launch/drive`（直接启动 donkey web + manage.py drive 并重定向到驾驶页面）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.56。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.56；`test_web_console_header_entry_buttons` 改为验证 onclick 跳转属性（不再断言无 onclick）。
  - 验证：`pytest tests/test_firmware_feature_flags.py` 通过；编译通过；已 OTA 刷机（ArduinoOTA -> `192.168.3.46`）。

## 2026-08-10 v1.7.55

- 固件版本号从 `v1.7.54` 更新到 `v1.7.55`。
- feat(WebConsole): RC Channels 的 Min T / Max T 数字可直接点击打字输入；数字与滑块同行居中对齐；Mid S / Mid T 的 Set 键改用 OTA 按钮样式并与数字对齐
  - 数字可输入：Min T / Max T 的数值由 `<span>` 改为 `<input type="number" class="rcNum">`，点击即可键入，回车/失焦生效；新增 `commitThrottleLimit()`（含 `commitThrottleMin`/`commitThrottleMax` 包装）按滑块动态上下限钳制、同步滑块并发 `THROTTLE_MIN`/`THROTTLE_MAX` 命令；`updateState()` 轮询刷新跳过正在编辑的输入框；i18n 新增 `rc.numInput` 中英词条。
  - 布局对齐：滑块+数字改为 flex 行（`align-items:center;justify-content:center;gap:10px`，滑块宽 70%），整体在单元格内居中；Mid S / Mid T 单元格数字与 Set 键包为 flex 行垂直居中。
  - 样式：`.rcNum` 最终定为 14px Consolas、宽 `4.5ch`、无边框（hover/focus 仅淡底色 `#1a2230`，无输入框外观），并显式 `flex:0 0 auto;min-width:0;max-width:none` 以抵消全局 `input{flex:0 1 180px;min-width:120px;max-width:220px}` 规则对 flex 项主轴尺寸的覆盖（此前数字输入框一直被撑到约 180px 宽）；Set 键新增 `.rcSetBtn` 复用 OTA 按钮蓝底胶囊样式（尺寸不变）。
  - `libraries/mus4_web/src/WebConsoleAssets.h`：上述 HTML/CSS/JS 全部改动；`libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.55。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.55（顺序断言 v1.7.55 先于 v1.7.54）。
  - 说明：Min/Max T 输入与 Set 键样式的主体代码经 PR #22 合入时未记录日志，本条目一并补记。
  - 验证：`tests/` 全量 pytest 通过；编译通过；已 OTA 刷机（ArduinoOTA → `192.168.3.46`）。

## 2026-08-10 v1.7.54

- 固件版本号从 `v1.7.53` 更新到 `v1.7.54`。
- feat(WebConsole): 头部主标题右侧、GitHub 图标左侧依次新增"进入 donkey""进入 DonkeyDrifter"两个入口按钮（功能预留，暂未实现跳转）
  - `libraries/mus4_web/src/WebConsoleAssets.h`：headerRow 在主标题 `<h1>` 后、`.ghLink` 前插入两个 `<button type="button" class="otaButton">`（样式完全复用 OTA 按钮蓝色胶囊，无 `onclick`，点击跳转功能预留）；i18n 新增 `button.enterDonkey` / `button.enterDonkeyDrifter` 中英词条（进入 donkey / 进入 DonkeyDrifter，Enter donkey / Enter DonkeyDrifter）。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.54。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.54；新增 `test_web_console_header_entry_buttons`（位置顺序——标题后、GitHub 图标前，donkey 左 DonkeyDrifter 右/复用 otaButton 样式/i18n 中英词条断言）。
  - 验证：`tests/` 全量 pytest 通过（309 项）；编译通过；已 OTA 刷机（ArduinoOTA → `192.168.3.46`）。

## 2026-08-10 v1.7.53

- 固件版本号从 `v1.7.52` 更新到 `v1.7.53`。
- fix(LED): 上电自检改为 RGB 三通道齐亮（白色）常亮 3 秒——v1.7.51 误实现为红/绿/蓝轮流各亮 1 秒
  - 需求本义：RGB 灯珠的红、绿、蓝三个通道一起点亮（即白色）并保持 3 秒；v1.7.51 的 `runLedPowerOnSelfTest()` 做成三色轮流各 1 秒，与需求不符。
  - `libraries/mus4_ui/src/LedStatus.cpp`：`runLedPowerOnSelfTest()` 改为 `setLEDColor(CRGB::White)` + `delaySelfTestHold(3000)` 一次保持；自检期间持续驱动 `buzzer.update()` 的 v1.7.52 修复保留。
  - `MUS4_FW.ino`：自检调用点注释同步为 "all RGB channels on (white) for 3s"。
  - `docs/Hardware/pin_definitions.md`：WS2812B 颜色定义同步为"红/绿/蓝三通道齐亮（白色）常亮 3 秒"。
  - `libraries/mus4_core/src/BuildInfo.h`：版本号 v1.7.53。
  - `tests/test_firmware_feature_flags.py`：版本号断言更新至 v1.7.53；自检断言同步（`delaySelfTestHold(3000)`、`setLEDColor(CRGB::White)`、不再逐色 `delaySelfTestHold`）。
  - 验证：`tests/` 全量 pytest 通过（308 项）；编译通过；已 OTA 刷机（HTTP `/update` → `192.168.3.46`）。

## 2026-08-09 v1.7.52

- 固件版本号从 `v1.7.51` 更新到 `v1.7.52`。
- feat(LED): OTA 固件传输期间状态灯随机乱闪（故障灯效），成功后跨重启延续到开机蜂鸣器旋律播完
  - `libraries/mus4_ui/src/LedStatus.h` / `LedStatus.cpp`：新增 `startLedOtaGlitch()` / `scanLedOtaGlitch()` / `stopLedOtaGlitch()`——灭/红/绿/蓝随机颜色 + 30-120ms 随机间隔，由上传回调直接驱动（传输期间主循环及 `scanLEDToggle()` 阻塞在 handler 内）；`startLedOtaGlitchUntilBuzzerIdle()` 等蜂鸣器模式 + `isLedOtaGlitchActive()` / `ledOtaGlitchWaitsForBuzzer()` 查询；乱闪期间正常状态机静默（`setLEDColor` / `setLEDToggle`×2 / `scanLEDToggle` 四处 early-return），`stopLedOtaGlitch()` 清 toggle 状态、ControlMixer 下一循环自动恢复；`markLedOtaGlitchAfterReboot()` / `takeLedOtaGlitchAfterReboot()` 经 `RTC_DATA_ATTR`（软重启保持、冷上电清零）把灯效带过 `ESP.restart()`。
  - `libraries/mus4_web/src/WebConsoleServer.cpp`：HTTP `/update` 通道挂载——`UPLOAD_FILE_START` 启动、`UPLOAD_FILE_WRITE` 每块推进、`UPLOAD_FILE_END`/`UPLOAD_FILE_ABORTED` 停止；`handleWifiWebUpdatePost()` 在 `ESP.restart()` 前写 RTC 标记。
  - `MUS4_FW.ino`：ArduinoOTA 通道同款挂载（`onStart`/`onProgress`/`onEnd`/`onError`，`onEnd` 写 RTC 标记）；`setup()` 上电自检后取标记、以等蜂鸣器模式重启乱闪；`loop()` 尾随 `buzzer.update()` 后推进乱闪并盯 `buzzer.isPlaying()`，蜂鸣器空闲满 800ms（覆盖多段旋律间停顿）才 `stopLedOtaGlitch()`。
  - `docs/Hardware/pin_definitions.md`：WS2812B 颜色定义新增 OTA 故障灯效条目（传输乱闪、中止/失败即恢复、成功跨重启延续到开机旋律播完）。
  - `tests/test_firmware_feature_flags.py`：新增 `test_ota_glitch_led_effect`（API 声明/随机颜色与间隔/HTTP 与 ArduinoOTA 双通道挂载/RTC 标记/setup 取标记/loop 800ms 宽限/状态机静默断言）。
- fix(LED): 上电自检 3 秒阻塞拖长开机旋律第一个音符 → 自检保持切片化、期间持续驱动 `buzzer.update()`
  - v1.7.51 引入的上电自检（红绿蓝各常亮 1 秒，`delay()` 阻塞共 3 秒）位于 `setupWifiConsole()` 之后——AP 启动音已开始播放（非阻塞旋律，音符切换靠主循环 `buzzer.update()`），自检期间音符无法切换，开机旋律第一个音被拖长约 3 秒，每次上电/重启均复现。
  - `libraries/mus4_ui/src/LedStatus.cpp`：`runLedPowerOnSelfTest()` 的 `delay(1000)` 改为 `delaySelfTestHold(1000)`——10ms 小片循环、每片调用 `buzzer.update()`（`extern Buzzer buzzer` 全局实例），自检期间旋律正常推进。
  - `tests/test_firmware_feature_flags.py`：新增自检期间驱动蜂鸣器断言。
  - 验证：`tests/` 全量 pytest 通过（308 项）；编译通过；已 OTA 刷机（HTTP `/update` → `192.168.3.46`，限速 20KB/s 拉长传输窗口实测乱闪灯效与重启后蜂鸣器延续）。

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
