# Web Console Voltage 卡片位置调整方案

## 背景

MUS4 Web Console 顶部状态卡片当前顺序为：

```text
Mode / Park / Drift / AP / STA / Voltage
```

用户希望将 `Voltage` 状态卡片移动到 `AP` 卡片左边，并且紧挨 `Drift` 卡片，便于在观察漂移辅助状态时同步查看电压状态。

本方案仅调整 Web Console 内嵌 HTML/CSS 布局，不改变固件控制逻辑、传感器读取、遥测数据、权限策略、OTA 行为或 Web API 契约。

## 目标

- 将顶部状态卡片顺序调整为：

```text
Mode / Park / Drift / Voltage / AP / STA
```

- `Voltage` 卡片位于 `AP` 左侧。
- `Voltage` 卡片与 `Drift` 卡片在 DOM 和视觉布局上相邻。
- 为 `Voltage`、`AP`、`STA` 三个卡片增加明确 flex 权重，减少不同屏宽下的随机宽度差异。
- 保持现有卡片样式、字体层级、数据绑定和状态刷新逻辑不变。

## 非目标

- 不重构 `mus4.ino` 中 Web Console HTML/CSS 字符串组织方式。
- 不修改 INA219 电压采样、WebSocket 遥测、`/api/status` 或 `/api/data` 数据结构。
- 不调整 `Mode`、`Park`、`Drift` 卡片的现有比例与样式。
- 不改变 `Voltage` 数值格式、阈值、颜色状态或业务含义。

## 设计

### DOM 顺序

在 `mus4.ino` 的 Web Console 状态卡片区域，将 `voltageCard` 从 `staCard` 后方移动到 `driftCard` 与 `apCard` 之间。

调整前：

```html
<div id="driftCard" class="stateCard">...</div>
<div id="apCard" class="stateCard">...</div>
<div id="staCard" class="stateCard">...</div>
<div id="voltageCard" class="stateCard">...</div>
```

调整后：

```html
<div id="driftCard" class="stateCard">...</div>
<div id="voltageCard" class="stateCard">...</div>
<div id="apCard" class="stateCard">...</div>
<div id="staCard" class="stateCard">...</div>
```

这样源码阅读顺序与页面视觉顺序一致，避免使用 CSS `order` 造成维护时的认知差异。

### flex 权重

当前状态卡片容器 `.stateGrid` 使用 flex 布局并允许换行。`Mode`、`Park` 和 `Drift` 已有专用 flex 配置，本次仅补充 `Voltage`、`AP`、`STA` 的明确宽度规则。

建议新增或合并为：

```css
#voltageCard{flex:1}
#apCard{flex:1}
#staCard{flex:1}
```

如果源码中已有相邻卡片选择器，应保持当前一行压缩 CSS 风格，避免扩大固件字符串体积。

### 数据流

本次不改变任何数据流：

1. INA219 仍由现有采样逻辑更新 `ina219Data.loadVoltage`。
2. Web 数据点仍通过现有逻辑写入 `point.voltage`。
3. 前端仍通过现有 JavaScript 更新 `#voltageValue` 和 `#voltageSub`。
4. 卡片移动不影响 DOM id，因此现有数据绑定无需修改。

### 边界与异常

- 窄屏下 `.stateGrid` 继续依赖 `flex-wrap:wrap` 自动换行。
- 由于 `Voltage`、`AP`、`STA` 使用相同 flex 权重，它们在同一行空间不足时会以一致宽度参与换行。
- 如果浏览器缓存旧页面，刷新页面即可看到新布局；固件端不需要额外迁移逻辑。

## 测试策略

本次是固件内嵌 Web UI 布局调整，优先使用源码断言和编译验证：

1. 更新或新增 `tests/test_firmware_feature_flags.py` 中的源码断言，验证：
   - `voltageCard` 在 `driftCard` 之后。
   - `voltageCard` 在 `apCard` 之前。
   - CSS 中存在 `#voltageCard{flex:1}`、`#apCard{flex:1}`、`#staCard{flex:1}` 或等价规则。
2. 运行相关 Python 测试：

```powershell
pytest tests/test_firmware_feature_flags.py
```

3. 修改固件源码后运行 WSL 编译：

```powershell
.\arduino-cli-wsl.ps1 -Compile
```

4. 编译通过后，按项目偏好尝试 HTTP OTA 上传；若当前目标 IP 不通，再确认设备 IP 后重试。

## 风险与缓解

- **风险：窄屏换行后视觉顺序变化。** 由于 DOM 顺序即目标顺序，换行后的阅读顺序仍为 `Drift / Voltage / AP / STA`。
- **风险：CSS 增加导致 Web Console 字符串体积略增。** 使用现有压缩风格，仅增加最小必要选择器。
- **风险：测试过度绑定实现细节。** 断言只保护用户可见顺序和关键 flex 规则，不检查无关样式。

## 验收标准

- Web Console 顶部卡片顺序为 `Mode / Park / Drift / Voltage / AP / STA`。
- `Voltage` 卡片在 `AP` 左侧，并紧挨 `Drift` 卡片。
- `Voltage`、`AP`、`STA` 三个卡片拥有明确且一致的 flex 权重。
- `Voltage` 数值刷新逻辑保持不变。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
