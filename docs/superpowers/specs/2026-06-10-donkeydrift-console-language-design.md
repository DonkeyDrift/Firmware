# DonkeyDrift Console 语言悬浮球设计

## 背景

Web Console 当前已有右下角 `?` 帮助悬浮球。用户希望在它上方增加一个“地球”悬浮球，用来选择界面显示语言。默认语言为中文，另一个可选项为英文。语言选择需要在刷新页面后保持，英文覆盖核心用户可见界面文案。

## 目标

- 在现有 `?` 帮助悬浮球上方增加 `🌐` 语言悬浮球。
- 默认显示中文。
- 支持在中文与英文之间切换。
- 使用 `localStorage` 持久化用户选择，刷新页面后保持上次语言。
- 英文覆盖核心界面文案：标题、按钮、面板标题、状态卡片标签、主要弹窗、Toast 和帮助内容。
- 通过 `tests/test_firmware_feature_flags.py` 的源码断言保护关键结构、字典与交互函数。

## 非目标

- 不修改后端 API、WebSocket、OTA、Wi-Fi 权限策略或车辆控制逻辑。
- 不翻译设备返回的日志、命令输出和协议响应。
- 不修改 JSON 字段、API 路径、命令名或内部变量命名。
- 不引入外部前端依赖、构建工具或额外静态资源。
- 不重构当前内嵌 HTML/CSS/JS 的整体组织方式。

## 方案

采用轻量前端字典与 `data-i18n` 标记方案。所有改动位于 `MUS4_FW.ino` 的 `WIFI_WEB_CONSOLE_HTML` 内，保持单文件内嵌 Web UI 的现有结构。

### UI 组件

新增两个前端元素：

- `button#langFab.langFab`：固定定位在右下角，位于 `button#helpFab` 上方，显示 `🌐`。
- `div#langMenu.langMenu`：语言菜单，点击地球按钮后展开，包含 `中文` 与 `English` 两项。

交互规则：

- 点击 `🌐` 切换语言菜单显示状态。
- 点击 `中文` 调用 `setLanguage('zh')`。
- 点击 `English` 调用 `setLanguage('en')`。
- 当前语言项显示选中状态。
- 点击页面其他区域关闭语言菜单。
- 帮助悬浮球和帮助弹窗逻辑保持独立。

样式沿用现有帮助按钮的圆形悬浮风格，使用固定定位、相同右边距和较高 `z-index`。语言按钮不参与 `.grid` 布局，避免影响现有面板顺序。

### 国际化数据结构

新增 `I18N` 字典：

```js
const I18N = {
  zh: {
    language: '语言',
    statusCards: '状态卡片',
    network: 'Network',
    diagnostics: 'Diagnostics'
  },
  en: {
    language: 'Language',
    statusCards: 'Status Cards',
    network: 'Network',
    diagnostics: 'Diagnostics'
  }
}
```

实际实现时字典应覆盖核心用户可见文案，包括但不限于：

- 页面标题与顶部品牌附近说明。
- 状态卡片标签。
- Network、Diagnostics、Serial Log、Tub JSON、Chart、OTA / DEV 等面板标题和按钮。
- Wi-Fi AP/STA 配置弹窗、STA 切换提示和失败提示。
- Toast 文案。
- 帮助浮窗标题、关闭按钮和功能说明。
- 图表暂停/绘制、全屏/分屏、日志暂停/继续等动态按钮文案。

### DOM 标记

核心静态文本使用 `data-i18n="key"` 标记，由 `applyLanguage(lang)` 统一替换 `textContent`。

需要翻译属性时使用专用标记：

- `data-i18n-placeholder="key"` 更新 `placeholder`。
- `data-i18n-aria="key"` 更新 `aria-label`。
- 如需要保留 HTML 结构，优先拆分为多个可翻译文本节点，避免使用大段 `innerHTML`。

### 数据流

页面初始化：

1. 从 `localStorage.getItem('mus4.ui.lang')` 读取语言。
2. 仅接受 `zh` 或 `en`。
3. 缺失、异常或非法值统一回退 `zh`。
4. 调用 `applyLanguage(lang)` 更新 UI。

用户切换语言：

1. `setLanguage(lang)` 校验目标语言。
2. 将合法语言写入 `localStorage.setItem('mus4.ui.lang', lang)`。
3. 调用 `applyLanguage(lang)`。
4. 更新 `document.documentElement.lang`。
5. 更新语言菜单选中态。
6. 关闭语言菜单。

动态文案：

- `togglePause()` 使用当前语言字典更新日志按钮为“暂停/继续”或 “Pause/Resume”。
- `toggleChart()` 使用当前语言字典更新图表按钮为“暂停/绘制”或 “Pause/Draw”。
- `toggleChartFullscreen()` 使用当前语言字典更新按钮为“全屏/分屏”或 “Fullscreen/Split”。
- DEV、AP、STA、ON、OFF、ACK、NACK 等协议或状态值保持原样。

## 错误处理与边界

- `localStorage` 读取或写入失败时捕获异常，页面继续以中文运行。
- 字典缺少 key 时保留元素当前文本，避免出现空白 UI。
- 非法语言值回退中文。
- 语言菜单点击需要阻止事件冒泡，避免刚打开又被全局点击关闭。
- 语言切换不触发网络请求，不影响轮询、WebSocket、OTA 或车辆控制链路。
- 语言按钮的固定定位必须避开帮助按钮，保证两个悬浮球都可点击。

## 测试策略

遵守 TDD 顺序，先更新 `tests/test_firmware_feature_flags.py`，再修改 `MUS4_FW.ino`。

新增或调整源码断言：

- 存在 `id="langFab"` 与 `id="langMenu"`。
- 存在 `mus4.ui.lang`。
- 存在 `function applyLanguage`、`function setLanguage` 和语言菜单切换函数。
- 存在 `data-i18n`、`data-i18n-placeholder` 或 `data-i18n-aria` 标记。
- `I18N` 字典包含 `zh` 与 `en`。
- 英文字典包含 `Language`、`Status Cards`、`Network`、`Diagnostics`、`Serial Log`、`Tub JSON`、`OTA / DEV` 等关键文案。
- `langFab` 在源码结构上位于 `helpFab` 前，代表视觉上位于帮助按钮上方。
- 默认语言回退为 `zh`，非法语言不应被保存为有效状态。
- 图表和日志动态按钮文案通过当前语言字典更新。
- 现有 `helpFab`、`helpOverlay`、`helpModal`、`openHelpModal()` 与 `closeHelpModal()` 断言继续通过。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

## 风险与约束

- `MUS4_FW.ino` 内嵌 HTML 已较大，新增字典需要控制范围，避免无关文案膨胀。
- ESP32 Web UI 不应引入外部资源或大型前端框架。
- 语言切换必须是前端展示层能力，不得影响安全关键控制路径。
- 部分动态内容来自设备状态或命令输出，不应为了翻译而改动协议语义。
- 测试以源码断言为主，不能替代实机浏览器体验验证；编译通过后可再进行 Web Console 手动检查。

## 验收标准

- 页面右下角显示 `?` 帮助悬浮球，并在其上方显示 `🌐` 语言悬浮球。
- 初次打开页面显示中文。
- 点击 `🌐` 后可选择 `中文` 或 `English`。
- 选择英文后核心界面文案切换为英文。
- 刷新页面后保持上次选择的语言。
- `localStorage` 不可用或语言值非法时回退中文。
- 帮助浮窗、Network、Diagnostics、Serial Log、Tub JSON、Chart、OTA / DEV 等核心文案受语言切换影响。
- 命令输出、协议值、API 字段和车辆控制行为不变。
- `pytest tests/test_firmware_feature_flags.py` 通过。
- `.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino` 通过。
