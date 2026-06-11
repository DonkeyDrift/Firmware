# Web Console DEV MODE 与状态卡片布局优化方案

## 背景

MUS4 Web Console 顶部区域和状态卡片当前存在三处视觉优化点：

1. 顶部开发模式开关显示为 `DEBUG MODE`，与代码和说明中的 Dev Mode 概念不完全一致。
2. `MODE` 与 `PARK` 两个状态卡片横向占比偏大，挤占同一行其它状态卡片空间。
3. 版本号文字与 `MUS4 Web Console` 标题在垂直方向上居中对齐，视觉上没有贴合标题底边。

本方案仅调整 Web Console 静态 HTML/CSS 文案与布局，不改变固件控制逻辑、权限策略、OTA 行为或 Web API 契约。

## 目标

- 将顶部开关标签从 `DEBUG MODE` 改为 `DEV MODE`。
- 将 `MODE` 与 `PARK` 状态卡片的 flex 比例从当前 `0.7` 调整为 `0.30`。
- 保留 `MODE` 与 `PARK` 卡片原有字体大小、内边距和状态文案层级。
- 将版本号文字与 `MUS4 Web Console` 标题底边对齐。
- 增加或更新源码断言测试，防止 UI 文案和关键布局规则回退。

## 非目标

- 不重构 Web Console 的 HTML 字符串组织方式。
- 不调整 Dev Mode 持久化、认证豁免或 Park Locked 安全限制。
- 不调整 `DRIFT`、`AP`、`STA`、`Voltage` 等其它状态卡片的比例。
- 不修改 `/api/status`、`/api/devmode`、WebSocket 遥测或 OTA 上传端点。

## 设计

### 顶部标题行

Web Console 标题行继续使用 `.headerRow` flex 布局。将其垂直对齐方式从 `align-items:center` 调整为 `align-items:flex-end`，使标题、版本号和右侧 Dev Mode 开关在底边方向对齐。

版本号 `.version` 保留当前视觉风格：

- 12px 字号。
- uppercase。
- 字间距 `.08em`。
- 灰蓝色弱化显示。

为了避免引入额外复杂度，本次不单独给版本号设置绝对定位，也不改变标题字号。

### Dev Mode 标签

顶部开关仅修改用户可见标签：

- 原文案：`DEBUG MODE <状态>`
- 新文案：`DEV MODE <状态>`

JavaScript 函数名、DOM id、NVS key、接口路径仍继续使用现有 `devMode` 命名，因为这些是内部实现标识，且已经准确表达开发模式语义。

错误提示中的“开启 DEBUG MODE 后重试”应同步改为“开启 DEV MODE 后重试”，避免用户在页面上看到两个不同名称。

### MODE/PARK 卡片宽度

当前 `#modeCard` 和 `#parkCard` 使用 `flex:0.7`。本次调整为：

```css
#modeCard{flex:0.30}
#parkCard{flex:0.30}
```

不修改以下共享样式：

- `.stateCard` 的 `padding:12px`。
- `.stateValue` 的 `font-size:24px` 与 `font-weight:800`。
- `.stateSub` 的 `font-size:12px`。

这样可以缩小卡片横向占比，同时保持 MODE/PARK 的可读性和触控空间。

## 测试策略

本次属于固件内嵌 Web UI 文案与 CSS 调整。优先更新 `tests/test_firmware_feature_flags.py` 中的源码断言测试，覆盖：

- Web Console HTML 中不再出现 `DEBUG MODE` 标签。
- Web Console HTML 中出现 `DEV MODE` 标签。
- `#modeCard` 和 `#parkCard` 使用 `flex:0.30`。
- `.headerRow` 使用 `align-items:flex-end`。

如果现有测试文件已有相近的 Web Console 源码断言，应在现有测试中扩展，不新增重复测试文件。

## 风险与边界

- `flex:0.30` 在较窄屏幕上可能让 `MANUAL` 或 `LOCKED` 更接近卡片边缘，但由于保留 flex-wrap 和原字体/内边距，卡片仍可换行布局。
- 页面 HTML 当前压缩在 `mus4.ino` 的原始字符串中，修改时应保持现有一行 CSS/HTML 风格，避免无关体积膨胀。
- 文案改为 `DEV MODE` 后，应同步所有面向用户的错误提示，避免 UI 术语不一致。

## 验收标准

- 顶部开关显示为 `DEV MODE OFF/ON`。
- 页面不再显示 `DEBUG MODE` 用户可见标签。
- `MODE` 与 `PARK` 卡片为 `flex:0.30`。
- `MODE` 与 `PARK` 字体和内边距保持原大小。
- 版本号与 `MUS4 Web Console` 标题底边视觉对齐。
- 相关 Python 源码断言测试通过。
