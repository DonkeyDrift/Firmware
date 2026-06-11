# Web Console Network 与 Voltage 卡片视觉优化方案

## 背景

`AP` 与 `STA` 已合并为 `Network` 状态卡片后，顶部状态栏的信息密度提升，但仍存在几处视觉层级问题：

1. `AP` / `STA` 标签位于卡片左上角，和 `Network` 标题抢占视觉入口。
2. `Network` 的下方详情同时显示 `SSID` 与 `IP`，其中 `IP` 与主值重复。
3. `Voltage` 卡片当前宽度偏大，不符合它只展示电压和剩余电量的轻量信息密度。
4. `Network` 卡片当前宽度偏大，和顶部状态栏其它卡片比例不够协调。
5. `Voltage` 的剩余百分比作为普通副标题显示，视觉权重偏弱。

本方案只调整 Web Console 内嵌 UI 的 HTML/CSS/JS 展示，不改变电压采样、Wi-Fi 状态数据、AP/STA 连接逻辑、认证策略、OTA 行为或控制命令权限。

## 目标

- 将 `AP` / `STA` 标签移动到 `Network` 卡片右上角状态圆点左侧。
- `Network` 卡片下方只显示放大的 `SSID`，不再重复显示 `IP`。
- `Network` 卡片不再显示 `clients` / `connected` 等副标题；这些信息仍保留在 `STATUS Details` 中。
- 将 `Voltage` 宽度调整为与 `Mode` 相同。
- 将 `Network` 宽度调整为 `flex:0.80`。
- `Voltage` 主值显示保留 1 位小数，例如 `12.1V`。
- `Voltage` 下方使用 `REMAIN` 标签展示剩余电量百分比，百分比字体与 `Network` 的 SSID 值保持一致。

## 非目标

- 不修改 `STATUS` 输出字段。
- 不修改 `/api/status`、`/api/data` 或 `/api/wifi-sta` 的接口契约。
- 不改变 AP/STA 标签页默认选择与手动锁定逻辑。
- 不改变电量百分比计算公式。
- 不调整 `Mode`、`Park`、`Drift` 的业务文案与状态逻辑。

## 设计

### 顶部卡片宽度

按现有 flex 风格最小调整：

```css
#modeCard{flex:0.30}
#voltageCard{flex:0.30}
#networkCard{flex:0.80}
```

`Voltage` 与 `Mode` 等宽，`Network` 保持比 `Voltage` 更宽，容纳 IP、SSID、标签页与齿轮。

### Network 标签位置

将 `.netTabs` 从左上角移动到右上角状态圆点左侧：

```css
.netTabs{position:absolute;right:28px;top:8px;display:flex;gap:4px}
```

右侧状态圆点仍使用 `.stateDot{right:12px;top:12px}`，因此标签与状态圆点形成同一视觉行。

### Network 信息层级

`Network` 保留：

- 标题：`Network`
- 主值：当前标签页对应 IP，继续使用 `networkValue`
- 下方详情：`SSID` 标签 + 放大 SSID 值

移除或停止使用：

- `networkSub` 对 `clients` / `connected` / `pending` / `not configured` 的展示。
- `networkIpValue` 与重复 IP 元信息行。

状态仍通过卡片边框与圆点颜色表达：

- AP 页：在线语义，使用 `mode0`。
- STA connected：使用 `mode0`。
- STA pending / disabled：使用 `driftOff`。

### Voltage 信息层级

`Voltage` 保留主值：

```text
12.1V
```

下方改为与 `Network` 的 SSID 详情一致的视觉结构：

```text
REMAIN
76%
```

实现上可将原 `voltageSub` 从普通 `.stateSub` 改为元信息容器，例如：

```html
<div class="stateMeta"><b>REMAIN</b><span id="voltageSub">battery</span></div>
```

`Network` 可复用同一 `.stateMeta` 样式：

```html
<div class="stateMeta"><b>SSID</b><span id="networkSsidValue">--</span></div>
```

这样 `76%` 和 `Home WiFi` 使用同一字体层级。

### Voltage 数值精度

将前端 `updateState()` 中电压主值格式从：

```javascript
v.toFixed(2)+'V'
```

调整为：

```javascript
v.toFixed(1)+'V'
```

电量百分比计算公式保持不变。

## 测试策略

后续实现继续遵守 TDD，先写失败测试，再做最小实现。建议更新 `tests/test_firmware_feature_flags.py`：

- 断言 `#voltageCard{flex:0.30}`。
- 断言 `#networkCard{flex:0.80}`。
- 断言 `.netTabs` 使用 `right:28px;top:8px`。
- 断言 `networkIpValue` 不再存在。
- 断言 `networkSub` 不再用于 Network 卡片展示。
- 断言存在 `REMAIN` 标签。
- 断言存在共享的放大元信息样式，例如 `.stateMeta span{...font-size:15px;font-weight:700...}`。
- 断言电压显示使用 `v.toFixed(1)+'V'`，不再使用 `v.toFixed(2)+'V'`。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -q
.\arduino-cli-wsl.ps1 -Compile
```

编译通过后，按项目偏好通过 HTTP OTA 上传。

## 风险与缓解

- **风险：Network 不显示 connected/clients 后状态信息减少。** 通过边框/圆点颜色保留快速状态提示，详细信息仍可在 `STATUS Details` 查看。
- **风险：Voltage 变窄后文字拥挤。** 电压改为 1 位小数，剩余电量使用更短的百分比显示。
- **风险：SSID 较长时溢出。** 元信息值继续使用 `overflow-wrap:anywhere` 或等价截断/换行策略。
- **风险：标签移到右侧后与状态圆点重叠。** 使用 `right:28px` 为状态圆点保留空间。

## 验收标准

- `AP` / `STA` 标签显示在 `Network` 卡片右上角状态圆点左侧。
- `Network` 下方只显示放大的 SSID，不再显示重复 IP。
- `Network` 不再显示 `clients` / `connected` 副标题。
- `Voltage` 与 `Mode` 等宽。
- `Network` 使用 `flex:0.80`。
- `Voltage` 主值显示 1 位小数。
- `Voltage` 下方显示 `REMAIN` 与放大百分比。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
