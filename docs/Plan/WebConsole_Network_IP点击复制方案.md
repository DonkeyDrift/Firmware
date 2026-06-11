# Web Console Network IP 点击复制方案

## 背景

`Network` 卡片当前通过 AP/STA 标签页展示当前网络 IP 与 SSID。调试、OTA、浏览器访问和日志排查时，经常需要复制当前 AP 或 STA IP。手动选中 IP 在小屏或触控板场景下不够方便。

本方案为 `Network` 卡片的主 IP 值增加左键点击复制能力，并使用非阻塞、自动消失的 toast 提示复制结果。

## 目标

- 左键点击 `Network` 卡片主值中的 IP 地址后，自动复制该 IP。
- AP 标签页复制 `ap_ip`。
- STA 标签页复制 `sta_ip`。
- 使用不阻塞屏幕的 toast 提示，不使用 `alert()`。
- toast 自动消失，避免用户手动关闭。
- 保持 AP/STA 标签切换、STA 配置齿轮、状态刷新逻辑不变。

## 非目标

- 不改变 `STATUS` 输出字段。
- 不改变 AP/STA 默认选择与手动锁定逻辑。
- 不修改 Wi-Fi 连接、STA 配置保存、OTA 或控制命令权限。
- 不引入外部 JavaScript/CSS 依赖。

## 设计

### 可点击 IP

`networkValue` 继续显示当前标签页的 IP，同时增加可点击语义：

```html
<div class="stateValue copyValue" id="networkValue" onclick="copyNetworkIp()" title="点击复制 IP">--</div>
```

视觉提示：

```css
.copyValue{cursor:pointer;text-decoration:underline;text-decoration-style:dotted;text-underline-offset:4px}
.copyValue:hover{color:#5cc8ff}
```

### 当前复制值

前端维护当前可复制 IP：

```javascript
let networkCopyIp='';
```

`updateNetworkCard()` 在渲染 AP/STA 标签页时同步更新：

- AP 页：`networkCopyIp = ap`
- STA 页：`networkCopyIp = sta`

如果当前页没有有效 IP（例如 STA 未配置显示 `disabled`），点击后显示失败/不可复制提示。

### 复制实现

复制函数按优先级执行：

1. 优先使用 `navigator.clipboard.writeText(ip)`。
2. 如果 Clipboard API 不可用或失败，降级使用隐藏 `textarea` + `document.execCommand('copy')`。
3. 成功后显示 `已复制 IP：<ip>`。
4. 失败后显示 `复制失败，请手动选择 IP`。

### Toast 提示

页面新增一个轻量 toast 容器：

```html
<div id="toast" class="toast"></div>
```

显示位置为右下角，使用 `pointer-events:none`，不拦截用户操作。每次显示会重置计时器，约 1.6 秒后自动隐藏。

样式保持与现有深色 UI 一致：深色背景、细边框、轻微阴影。

## 测试策略

后续实现遵守 TDD，先写失败测试，再实现。建议更新 `tests/test_firmware_feature_flags.py`，覆盖：

- `networkValue` 存在点击复制入口 `onclick="copyNetworkIp()"`。
- 存在 `copyValue` 可点击样式。
- 存在 `toast` 容器和 `.toast.show` 自动提示样式。
- 存在 `copyNetworkIp()` 函数。
- 存在 Clipboard API 路径：`navigator.clipboard.writeText`。
- 存在降级复制路径：`document.execCommand('copy')`。
- `updateNetworkCard()` 中会维护 `networkCopyIp`。
- 复制提示不使用 `alert()`。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -q
.\arduino-cli-wsl.ps1 -Compile
```

编译通过后，按项目偏好通过 HTTP OTA 上传。

## 风险与缓解

- **风险：部分浏览器不允许 Clipboard API。** 提供 `textarea + execCommand('copy')` 降级路径。
- **风险：点击 disabled/无效 STA IP。** 点击时校验 IP 值，无有效 IP 时显示非阻塞失败提示。
- **风险：toast 遮挡操作。** 放在右下角并设置 `pointer-events:none`，自动消失。
- **风险：点击 IP 与 AP/STA 标签点击冲突。** 复制入口只绑定在 `networkValue`，标签按钮与齿轮保持独立。

## 验收标准

- AP 标签页点击 IP 可复制 AP IP。
- STA 标签页点击 IP 可复制 STA IP。
- 复制成功后出现非阻塞提示并自动消失。
- 复制失败时出现非阻塞失败提示并自动消失。
- 页面不使用阻塞式弹窗提示复制结果。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
