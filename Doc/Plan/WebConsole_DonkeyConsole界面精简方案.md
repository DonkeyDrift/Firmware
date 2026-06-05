# Web Console Donkey Console 界面精简方案

## 背景

当前 Web Console 顶部仍显示 `MUS4 Web Console`、`DEV MODE` 和独立的 `OTA Upload` 按钮；串口区域还保留 `PING`、`STATUS`、`AUTH`、`ENABLE_OTA`、`OTA_STATUS` 快捷按钮和开发模式说明行。界面信息密度偏高，日志区域也占用较多垂直空间。

本次目标是将页面精简为更适合日常操作的 `Donkey Console`，保留必要入口，减少按钮和计数噪声。

## 目标

- 页面标题和主标题改为 `Donkey Console`。
- 版本号以小写 `v` 开头显示，例如 `v1.5.23`。
- `DEV MODE` 顶部开关标签改为 `DEV`。
- 删除串口区域快捷按钮 `PING`、`STATUS`、`AUTH`、`ENABLE_OTA`、`OTA_STATUS`。
- `OTA Upload` 改为高亮 `OTA` 按钮，并移动到顶部 `DEV` 左侧。
- 原常驻开发模式说明改为鼠标悬浮在 `DEV` 标签时显示的说明窗。
- 串口日志显示高度改为 5 行。
- 删除正常日志轮询下的 `seq=... dropped=...` 计数显示。

## 非目标

- 不修改 OTA 后端权限、上传端点或安全策略。
- 不修改 Web Console 认证、Park Locked 约束或控制命令处理。
- 不修改串口协议、WebSocket 协议、状态数据字段。
- 不修改 DEV 模式持久化和确认弹窗逻辑。

## 设计

### 顶部标题与版本

将 HTML 的 `<title>` 与页面 `<h1>` 改为 `Donkey Console`。版本号仍来自 `STATUS` 中的 `version` 字段，但前端显示时执行首字母规范化：

```javascript
versionLabel.textContent=s.version.replace(/^V/,'v')
```

这样后端即使返回 `V1.5.23`，页面也会显示为 `v1.5.23`；后端已返回小写时不受影响。

### DEV 与 OTA

顶部顺序调整为：

```text
Donkey Console  v1.5.23                         OTA  DEV OFF [switch]
```

`OTA` 使用 `.otaButton` 高亮样式，链接仍指向 `/update` 并在新窗口打开。`DEV` 仍复用现有 `toggleSwitch`、`devModeCheck`、`toggleDevModeFromSwitch()` 与确认弹窗。

`DEV` 标签增加 `.devHint`，用 CSS `:hover:after` 显示说明：

```text
开发模式会持久化；Web Console 免 AUTH，但仍保留 Park Locked 安全限制。OTA 传输期间会默认 Park Locked。
```

### 串口区域

保留输入框、发送、清空、暂停日志按钮。删除快捷命令行和常驻开发模式说明行。用户仍可在输入框中手动发送 `PING`、`STATUS`、`AUTH`、`ENABLE_OTA`、`OTA_STATUS`，后端能力不变。

### 日志显示

将日志区域从固定 `280px` 改为 5 行高度：

```css
.log{height:calc(5 * 1.35em + 16px);...}
```

其中 `1.35em` 对应当前日志行高，`16px` 对应上下 padding。

正常 `pollLog()` 成功后不再显示 `seq=... dropped=...`，仅在异常时保留 `log error: ...` 方便排障。

## 测试策略

遵守 TDD，先更新 `tests/test_firmware_feature_flags.py`：

- 断言 `Donkey Console` 存在，`MUS4 Web Console` 不存在。
- 断言 DEV 标签改为 `DEV <b id="devModeSwitchText">OFF</b>`。
- 断言顶部存在高亮 `OTA` 按钮且位于 DEV 开关左侧。
- 断言串口快捷按钮和 `OTA Upload` 不存在。
- 断言常驻开发模式说明行不存在，悬浮说明 CSS 存在。
- 断言日志高度为 5 行表达式。
- 断言版本显示使用 `replace(/^V/,'v')`。
- 断言不再输出 `seq=... dropped=...`。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py tests/test_wireless_console_policy.py -q
.\arduino-cli-wsl.ps1 -Compile
```

编译通过后，按项目偏好通过 HTTP OTA 上传到当前目标。

## 风险与缓解

- **快捷按钮删除后调试路径变长。** 保留输入框手动命令能力，命令处理逻辑不变。
- **DEV 悬浮说明在触摸屏上不易触发。** 本次需求明确为鼠标悬浮说明，保持最小实现。
- **日志显示行数减少。** 日志区域仍可滚动，缓冲逻辑不变。
- **隐藏 seq/dropped 后少一个诊断信号。** 只隐藏正常 UI 计数，后端仍维护 dropped 数据，错误路径仍显示。

## 验收标准

- 顶部显示 `Donkey Console` 和小写 `v1.5.23`。
- 高亮 `OTA` 按钮位于 `DEV` 左侧。
- `DEV` 悬浮显示开发模式说明，移开后消失。
- 串口区域不显示 `PING`、`STATUS`、`AUTH`、`ENABLE_OTA`、`OTA_STATUS` 快捷按钮。
- 串口日志高度约为 5 行。
- 页面不显示 `seq=... dropped=...`。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
