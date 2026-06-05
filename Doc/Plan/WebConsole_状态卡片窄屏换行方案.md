# Web Console 状态卡片窄屏换行方案

## 背景

Web Console 顶部状态卡片当前使用 `flex-wrap` 自动换行。页面宽度缩小时，浏览器会根据剩余空间自动决定卡片落点，导致 `Drift`、`Voltage`、`Network` 的换行顺序不稳定；同时 `Park` 卡片中的 `UNLOCKED` 较长，在部分过渡宽度下容易显示不完整。

用户确认的最终目标是：宽度变小时先让 `Voltage` 和 `Network` 同时移动到第二行且不改变次序；再变小时进入最终三行形态，并确保文字不使用省略号，允许在框内自动换行，`UNLOCKED` 必须完整显示。

## 目标

- 宽屏：第一行保持 `Mode / Park / Drift / Voltage / Network`。
- 中等宽度：第一行 `Mode / Park / Drift`，第二行 `Voltage / Network`。
- 最终窄屏：第一行 `Mode / Park / Voltage`，第二行 `Drift`，第三行 `Network`。
- `Voltage` 和 `Network` 在中等宽度下同时进入第二行，且保持 `Voltage` 在前、`Network` 在后。
- `Park` 使用比 `Mode` 更大的宽度比例，确保 `UNLOCKED` 完整显示。
- 变窄过程中允许文字自动换行，不使用省略号。
- 不改变 HTML DOM 顺序、数据逻辑、控制逻辑或 OTA 逻辑。

## 非目标

- 不修改 Web Console 数据字段或 WebSocket 协议。
- 不修改 `updateState()` 中各状态的业务含义。
- 不改变 Network 标签页、IP 复制、Voltage 电压阈值等既有行为。
- 不引入外部 CSS/JS 依赖。

## 设计

将 `.stateGrid` 从 `flex` 自动换行调整为 CSS Grid 显式区域布局。通过媒体查询定义三套 `grid-template-areas`：

```css
.stateGrid{display:grid;gap:10px;align-items:stretch}
#modeCard{grid-area:mode}
#parkCard{grid-area:park}
#driftCard{grid-area:drift}
#voltageCard{grid-area:voltage}
#networkCard{grid-area:network}
```

### 宽屏布局

```css
.stateGrid{
  grid-template-columns:minmax(96px,.30fr) minmax(160px,.56fr) minmax(260px,1.30fr) minmax(112px,.30fr) minmax(220px,.80fr);
  grid-template-areas:"mode park drift voltage network";
}
```

### 中等宽度布局

```css
@media(max-width:860px){
  .stateGrid{
    grid-template-columns:minmax(96px,.30fr) minmax(160px,.56fr) minmax(260px,1.30fr);
    grid-template-areas:
      "mode park drift"
      "voltage network network";
  }
}
```

### 最终窄屏布局

```css
@media(max-width:620px){
  .stateGrid{
    grid-template-columns:84px 154px 100px;
    grid-template-areas:
      "mode park voltage"
      "drift drift drift"
      "network network network";
  }
}
```

窄屏下对所有状态卡使用更紧凑的字号；前三张小卡额外收紧内边距，`Drift` 与 `Network` 的主值、副标题、SSID 等字体也随 `Mode` 一起缩小，但不省略文本。

## 文本显示策略

移除状态卡主值、副标题、Meta 文本的省略号策略：

- 使用 `white-space:normal` 允许自动换行。
- 使用 `overflow:visible` 与 `text-overflow:clip`，不显示 `...`。
- 对普通英文单词保持 `word-break:normal`，避免 `UNLOCKED` 被拆开。
- 对 Network IP、SSID、Drift 细节这类长串内容，保留局部 `overflow-wrap:anywhere`，允许必要时在框内换行。

## 测试策略

遵守 TDD，先更新 `tests/test_firmware_feature_flags.py`：

- 断言 `.stateGrid{display:grid;gap:10px;align-items:stretch}` 存在。
- 断言五张卡分别绑定 `grid-area`。
- 断言宽屏、中等、窄屏三套 `grid-template-areas` 存在。
- 断言 Park 宽度使用 `minmax(160px,.56fr)`，窄屏使用 `154px`。
- 断言窄屏下 `Mode`、`Park`、`Voltage`、`Drift`、`Network` 主值字号统一缩小到 `18px`。
- 断言窄屏下 `Drift` 副标题、`Network` 的 SSID 等辅助文本也跟随缩小。
- 断言状态文本使用自动换行，不使用 `text-overflow:ellipsis`。
- 保留 DOM 顺序测试，确保 HTML 顺序不被改变。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py tests/test_wireless_console_policy.py -q
.\arduino-cli-wsl.ps1 -Compile
```

编译通过后，按项目偏好通过 HTTP OTA 上传。

## 风险与缓解

- **风险：CSS Grid 改动影响顶部状态卡整体宽度。** 使用显式区域布局只作用于 `.stateGrid`，不影响页面其它区域。
- **风险：窄屏下文字换行导致卡片高度增加。** 这是为了保持文字完整显示，且只影响顶部状态卡视觉高度。
- **风险：长 IP 或 SSID 撑开卡片。** 对 Network 局部启用 `overflow-wrap:anywhere`，允许在框内换行。
- **风险：媒体查询断点不适合所有屏幕。** 断点来自浏览器预览确认；后续可按实机显示继续微调。

## 验收标准

- 宽屏显示为 `Mode / Park / Drift / Voltage / Network`。
- 中等宽度显示为第一行 `Mode / Park / Drift`，第二行 `Voltage / Network`。
- 最终窄屏显示为第一行 `Mode / Park / Voltage`，第二行 `Drift`，第三行 `Network`。
- `UNLOCKED` 在过渡区间和窄屏下完整显示。
- 最终窄屏下 `Drift` 和 `Network` 的字体也像 `Mode` 一样相应变小。
- 状态卡文本不出现省略号。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
