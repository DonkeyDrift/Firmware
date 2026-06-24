# Web Console AP/STA 合并标签页方案

> **历史方案（v1.7.x 早期）**：本文写于 AP+STA 长期共存设计下，「AP/STA 两个标签同时可见、并行可用」的前提已被 v1.7.18 起的 **AP/STA 互斥切换**覆盖：实际任意时刻设备只在 AP 或 STA 一种模式下广播。当前 Web Console 仍保留 AP/STA 合并标签页布局，但两块状态卡片中只会有一个对应"当前活跃接口"。详见 [`docs/Plan/AP_STA互斥切换方案.md`](./AP_STA互斥切换方案.md)。

## 背景

MUS4 Web Console 顶部状态区域当前使用独立 `AP` 与 `STA` 两张卡片展示网络状态。随着 `Voltage` 卡片移动到 `Drift` 与网络状态之间，顶部状态栏横向空间进一步紧张。

用户希望将 `AP` 与 `STA` 整合为一个界面，通过左上角标签页切换，并且 AP 与 STA 两个标签页都需要同时展示 SSID 与 IP。

本方案仅调整 Web Console 网络状态展示与 `STATUS` 文本字段，不改变 Wi-Fi AP/STA 连接逻辑、认证策略、OTA 行为、STA 配置保存流程或控制命令权限。

## 目标

- 将独立 `AP`、`STA` 卡片合并为单张 `Network` 卡片。
- `Network` 卡片左上角提供 `AP` / `STA` 标签页切换。
- `Network` 卡片右上角保留 STA 配置齿轮，继续打开现有 STA Wi-Fi 配置弹窗。
- 默认标签页使用自动规则：
  - `sta_connected=1` 时默认显示 `STA`。
  - 其它情况默认显示 `AP`。
- 用户手动点击标签后，本页面保持用户选择，避免周期性状态刷新自动覆盖。
- AP 标签页同时展示 AP SSID 与 AP IP。
- STA 标签页同时展示 STA SSID 与 STA IP。

## 非目标

- 不改变 AP SSID、AP 密码或 STA 凭据保存方式。
- 不修改 `/api/wifi-sta` 的 JSON 契约。
- 不修改 `WIFI_STA_STATUS` 串口命令输出格式。
- 不调整 WebSocket 遥测、曲线、Tub JSON 或 OTA 上传端点。
- 不重构 `mus4.ino` 中 Web Console HTML/CSS/JS 的整体组织方式。

## 设计

### 顶部卡片布局

当前状态卡片顺序为：

```text
Mode / Park / Drift / Voltage / AP / STA
```

调整后为：

```text
Mode / Park / Drift / Voltage / Network
```

删除独立的 `apCard` 与 `staCard` DOM，新增 `networkCard`。`Network` 卡片继续参与 `.stateGrid` flex 布局，使用明确 flex 权重，例如：

```css
#networkCard{flex:1.4}
```

具体数值在实现时可按现有卡片宽度微调，但应保证 `Network` 足以容纳标签页、SSID、IP 与状态说明。

### Network 卡片结构

`Network` 卡片包含：

- 左上角标签：`AP` 与 `STA`。
- 右上角齿轮：继续调用 `openWifiStaModal()`。
- 主值：当前标签页对应的 IP 或禁用状态。
- 副标题：当前标签页对应的连接状态。
- 详情行：当前标签页的 `SSID` 与 `IP`。

示意结构：

```html
<div id="networkCard" class="stateCard">
  <div class="netTabs">
    <button id="networkApTab">AP</button>
    <button id="networkStaTab">STA</button>
  </div>
  <button class="gear" onclick="event.stopPropagation();openWifiStaModal()">⚙</button>
  <div class="stateHead">Network</div>
  <div class="stateValue" id="networkValue">--</div>
  <div class="stateSub" id="networkSub">waiting</div>
  <div class="networkMeta">
    <b>SSID</b><span id="networkSsidValue">--</span>
    <b>IP</b><span id="networkIpValue">--</span>
  </div>
  <span class="stateDot"></span>
</div>
```

标签页按钮应使用 `type="button"`，避免默认提交行为。为了适配固件内嵌 HTML 字符串，样式保持现有压缩 CSS 风格。

### 标签页状态

前端维护两个变量：

```javascript
let networkTab='auto';
let networkTabPinned=false;
```

含义：

- `networkTab='auto'`：还未手动选择，`updateNetworkCard()` 根据状态自动决定显示 AP 或 STA。
- `networkTabPinned=true`：用户点击过 AP/STA 标签，后续 `refreshStatus()` 不再自动切换当前标签。

默认选择规则：

```javascript
const selected = networkTabPinned ? networkTab : (staConnected ? 'sta' : 'ap');
```

浏览器刷新后变量重置，重新按自动规则初始化。

### AP 标签页内容

AP 页字段：

- SSID：`ap_ssid`，当前值来自固件常量 `WIFI_CONSOLE_AP_SSID`。
- IP：`ap_ip`，当前 SoftAP IP。
- 状态：`clients <ap_clients>`。

展示规则：

```text
Network
<ap_ip>
clients <ap_clients>
SSID  <ap_ssid>
IP    <ap_ip>
```

AP 页卡片状态类保持在线语义，使用类似现有 `mode0` 的绿色边框。

### STA 标签页内容

STA 页字段：

- SSID：`sta_ssid`，来自当前保存的 STA SSID。
- IP：`sta_ip`，连接成功时为真实 IP，否则为 `0.0.0.0`。
- 状态：`connected` / `pending` / `disabled`。

展示规则：

- `sta_connected=1`：

```text
Network
<sta_ip>
connected
SSID  <sta_ssid>
IP    <sta_ip>
```

- `sta_configured=1` 且未连接：

```text
Network
0.0.0.0
pending
SSID  <sta_ssid>
IP    0.0.0.0
```

- `sta_configured=0`：

```text
Network
disabled
not configured
SSID  --
IP    0.0.0.0
```

STA 页卡片状态类：

- connected：使用 `mode0`。
- pending / disabled：使用 `driftOff`。

### STATUS 数据契约

当前 `STATUS` 文本已提供：

- `ap_ip`
- `ap_clients`
- `sta_configured`
- `sta_connected`
- `sta_ip`

为满足 SSID 展示，本次扩展 `printWirelessStatus()` 输出，新增：

```text
ap_ssid="MUS4-DEBUG"
sta_ssid="<wifiStaSsid>"
```

字段应使用引号包裹，以兼容 SSID 中的空格。现有 `parseStatusPairs()` 已支持带引号值，因此前端无需新增解析器。

### STA 配置入口

右上角齿轮仍调用：

```javascript
openWifiStaModal()
```

现有弹窗、保存、清除、失败提示、等待连接结果逻辑保持不变。

## 测试策略

后续实现必须遵守 TDD，先写失败测试，再最小实现。

建议更新 `tests/test_firmware_feature_flags.py`，覆盖：

- 不再存在独立 `id="apCard"` 与 `id="staCard"`。
- 存在 `id="networkCard"`。
- 存在 `id="networkApTab"` 与 `id="networkStaTab"`。
- `networkCard` 位于 `voltageCard` 之后。
- `printWirelessStatus()` 输出包含 `ap_ssid=` 与 `sta_ssid=`。
- 前端存在自动默认选择规则：`staConnected ? 'sta' : 'ap'` 或等价逻辑。
- 前端存在用户手动选择保护变量，例如 `networkTabPinned`。
- 现有 STA 配置弹窗入口 `openWifiStaModal()` 仍可从齿轮触发。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -q
.\arduino-cli-wsl.ps1 -Compile
```

编译通过后，按项目偏好通过 HTTP OTA 上传。

## 风险与缓解

- **风险：合并后 Network 卡片内容变多，窄屏下拥挤。** 通过 `networkMeta` 小字号详情行和 `#networkCard{flex:1.4}` 预留宽度；窄屏继续依赖 `.stateGrid` 换行。
- **风险：状态刷新覆盖用户正在查看的标签。** 使用 `networkTabPinned`，用户点击后不再自动切换。
- **风险：SSID 中包含空格导致解析错误。** `STATUS` 中 SSID 字段加引号，复用现有 `parseStatusPairs()` 的引号解析能力。
- **风险：测试过度绑定 CSS 细节。** 测试只断言关键 DOM、字段和交互变量，不约束无关视觉样式。

## 验收标准

- 顶部状态卡片显示为 `Mode / Park / Drift / Voltage / Network`。
- `Network` 卡片左上角显示 `AP` / `STA` 标签页。
- STA 已连接时首次加载默认显示 STA；否则默认显示 AP。
- 用户手动切换标签后，周期性状态刷新不会覆盖当前标签。
- AP 页同时显示 AP SSID 与 AP IP。
- STA 页同时显示 STA SSID 与 STA IP。
- STA 配置齿轮仍能打开现有配置弹窗。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
