# DonkeyDrift Console 帮助浮窗设计

## 背景

Web Console 当前品牌文案为 `Donkey Console`。本次改动需要将面向用户的品牌显示改为 `DonkeyDrift Console`，并在页面右下角增加一个 `?` 悬浮帮助按钮。点击后显示简洁功能说明，帮助用户快速理解主要面板和常用入口。

## 目标

- 将 Web Console 用户可见品牌文案统一为 `DonkeyDrift Console`。
- 在右下角增加固定悬浮 `?` 按钮。
- 点击按钮后显示简洁帮助浮窗。
- 支持点击浮窗右上角 `×` 关闭。
- 支持点击浮窗外半透明遮罩关闭。
- 通过现有 `tests/test_firmware_feature_flags.py` 的源码断言保护 UI 结构与关键文案。

## 非目标

- 不修改后端 API、WebSocket、OTA、Wi-Fi 权限或车辆控制逻辑。
- 不新增帮助内容的持久化配置。
- 不引入外部前端依赖。
- 不重构当前内嵌 HTML/CSS/JS 的组织方式。

## 方案

### 品牌文案

在 `MUS4_FW.ino` 的内嵌 Web UI 中，将以下面向用户的 `Donkey Console` 改为 `DonkeyDrift Console`：

- HTML `<title>`。
- 页面顶部 `<h1>`。
- captive portal / 跳转页中的打开链接文案。

只修改用户可见品牌，不修改 API 路径、日志 source、数据 schema 或内部变量命名。

### 帮助按钮

在 Web UI 根级结构末尾增加一个固定定位按钮：

- 文案为 `?`。
- 位置固定在右下角。
- 使用圆形按钮样式。
- z-index 高于常规面板，避免被图表和日志区域遮挡。
- 不参与现有 `.grid` 布局，避免影响 Diagnostics、Serial、RC、Status 等面板顺序。

### 帮助浮窗

点击 `?` 后显示帮助浮窗和半透明遮罩。浮窗内容采用简洁导览，覆盖：

- 状态卡片：查看模式、Park、OTA、连接状态。
- Network：查看 AP/STA IP，配置 Wi-Fi。
- Diagnostics：运行测试、回归、维护命令。
- Serial Log：查看设备日志和命令反馈。
- Tub JSON：记录并下载遥测样本。
- OTA / DEV：固件更新与开发模式开关。

关闭方式：

- 点击浮窗右上角 `×` 关闭。
- 点击遮罩空白区域关闭。

### 前端行为

新增轻量 JavaScript 函数：

- `openHelpModal()`：显示遮罩和帮助浮窗。
- `closeHelpModal()`：隐藏遮罩和帮助浮窗。

按钮点击调用 `openHelpModal()`；关闭按钮和遮罩点击调用 `closeHelpModal()`。不记录状态，不请求网络，不影响现有轮询、WebSocket、日志或图表逻辑。

## 测试策略

先更新 `tests/test_firmware_feature_flags.py`，再实现源码改动。

新增或调整断言：

- `DonkeyDrift Console` 存在。
- 旧品牌 `Donkey Console` 不再作为 Web UI 品牌出现。
- 存在 `id="helpFab"`、`id="helpOverlay"`、`id="helpModal"` 和关闭按钮。
- 存在 `openHelpModal()` / `closeHelpModal()`。
- 帮助内容包含 `状态卡片`、`Network`、`Diagnostics`、`Serial Log`、`Tub JSON`、`OTA / DEV` 等关键说明。
- 保留现有 Diagnostics / Serial / RC / Status 面板顺序断言。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```

## 风险与约束

- `MUS4_FW.ino` 内嵌 HTML 已较大，新增内容应保持短小，避免无必要的大段文案。
- Web Console 运行在 ESP32 上，不能引入外部资源或依赖大型前端框架。
- 帮助按钮需要避开底部交互内容，使用固定右下角但保持足够边距。
- 浮窗不应吞掉现有按钮的事件；关闭后必须恢复页面正常操作。

## 验收标准

- 页面标题和顶部品牌显示为 `DonkeyDrift Console`。
- 右下角显示 `?` 悬浮按钮。
- 点击 `?` 后显示简洁功能说明浮窗。
- 点击 `×` 或遮罩空白区域可关闭浮窗。
- `tests/test_firmware_feature_flags.py` 通过。
- 固件 WSL 编译通过。
