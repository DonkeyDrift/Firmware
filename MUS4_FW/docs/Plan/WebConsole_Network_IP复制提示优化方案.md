# Web Console Network IP 复制提示优化方案

## 背景

`Network` 卡片主 IP 已支持点击复制，并通过右下角 toast 显示复制结果。当前 IP 可点击样式使用虚线下划线，并通过浏览器原生 `title` 显示“点击复制 IP”。用户反馈：

1. IP 值下方不需要虚线。
2. 鼠标悬停提示标签需要右移，避免遮挡 IP。

本方案只调整点击复制入口的视觉提示，不改变复制逻辑、toast 逻辑、AP/STA 标签切换或网络状态数据。

## 目标

- 移除 IP 下方的虚线下划线。
- 保留 IP 可点击语义：鼠标手势与 hover 颜色变化。
- 移除浏览器原生 `title`，避免提示位置不可控。
- 增加自定义 hover 提示标签。
- 自定义提示向右偏移，避免遮挡 IP 主值。
- 保留点击复制后的右下角 toast。

## 非目标

- 不修改 `copyNetworkIp()` 的复制逻辑。
- 不修改 Clipboard API 与降级复制路径。
- 不修改 AP/STA 标签、SSID、Voltage 或其它卡片布局。
- 不改变 toast 的显示位置、文案或自动消失时长。

## 设计

### IP 可点击样式

当前 `.copyValue` 包含虚线下划线：

```css
.copyValue{cursor:pointer;text-decoration:underline;text-decoration-style:dotted;text-underline-offset:4px}
```

调整为：

```css
.copyValue{cursor:pointer;position:relative}
.copyValue:hover{color:#5cc8ff}
```

这样 IP 本身保持干净，仅通过鼠标手势和 hover 变色表达可点击。

### 自定义 hover 提示

新增伪元素提示：

```css
.copyValue:hover:after{
  content:'点击复制 IP';
  position:absolute;
  left:72px;
  top:-26px;
  ...
}
```

由于提示从 IP 起点右移，避免遮挡 IP 主值开头。提示使用深色背景、蓝色边框，保持 Web Console 现有深色视觉风格。

### DOM 属性

从 `networkValue` 移除原生 `title="点击复制 IP"`：

```html
<div class="stateValue copyValue" id="networkValue" onclick="copyNetworkIp()">--</div>
```

避免浏览器原生 title 与自定义提示同时出现，也避免原生提示遮挡位置不可控。

## 测试策略

后续实现遵守 TDD。建议更新 `tests/test_firmware_feature_flags.py`：

- 断言 `.copyValue{cursor:pointer;position:relative}` 存在。
- 断言 `.copyValue:hover:after` 存在。
- 断言提示文案 `点击复制 IP` 存在于 CSS 伪元素中。
- 断言 `text-decoration:underline`、`text-decoration-style:dotted`、`text-underline-offset` 不再存在。
- 断言 `title="点击复制 IP"` 不再存在。
- 保持既有复制函数、Clipboard API、降级复制和 toast 测试不变。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -q
.\arduino-cli-wsl.ps1 -Compile
```

编译通过后，按项目偏好通过 HTTP OTA 上传。

## 风险与缓解

- **风险：自定义提示在很窄卡片中溢出。** `Network` 卡片当前为 `flex:0.80`，提示较短；即使溢出也只在 hover 时显示，不影响默认布局。
- **风险：没有下划线后可点击性降低。** 保留鼠标手势和 hover 变蓝，提示用户该值可点击。
- **风险：原生 title 与自定义提示冲突。** 移除 `title`，仅保留自定义提示。

## 验收标准

- IP 下方不再显示虚线。
- 鼠标悬停 IP 时，IP 变蓝并显示右移后的“点击复制 IP”提示。
- 提示不会遮挡 IP 主值开头。
- 点击复制后的右下角 toast 行为保持不变。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
