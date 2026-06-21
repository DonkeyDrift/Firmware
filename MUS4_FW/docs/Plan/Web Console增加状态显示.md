# 实施计划：Web Console 增加 Mode、Park、Drift 动效状态显示

## Context

当前 Web Console 已通过 `/api/data` 获取控制曲线数据，并且数据点里已有 `mode` 与 `park` 字段；但页面只用它们绘制曲线，没有给驾驶/调试人员提供醒目的状态卡片。Drift Assist 的运行状态目前仅存在于固件变量中，尚未进入 Web 数据接口。

本次改动目标是在 Web Console 顶部增加 Mode、Park、Drift 三个动态状态卡片：Mode 显示当前控制模式，Park 显示锁定/解锁状态，Drift 显示关闭/待命/介入状态及补偿强度。实现应复用现有 `/api/data` 轮询链路，不新增 HTTP 接口，不影响控制协议、认证策略、串口/Serial1 输出和 OTA 流程。

## 修改文件

- `mus4.ino` — 扩展 Web 数据点、JSON 输出与内嵌 Web Console HTML/CSS/JS。
- `wireless_console_policy.py` — 同步 Python 镜像的数据点格式。
- `tests/test_wireless_console_policy.py` — 更新 Web 数据点格式测试。

## 实施步骤

### 1. 先补 Python 数据格式测试

在 `tests/test_wireless_console_policy.py` 中更新 `test_formats_web_data_point_with_short_keys`，让期望结果包含 Drift 相关短字段：

- `de`：Drift Assist enabled，0/1。
- `da`：Drift Assist active，0/1。
- `dc`：当前 drift compensation。
- `gzf`：过滤后的 gyro Z。

同时增加一个默认场景测试：未传 Drift 数据时，`de=0`、`da=0`、`dc=0.0`、`gzf=0.0`，确保旧调用路径有明确默认值。

### 2. 扩展 Python 镜像函数

在 `wireless_console_policy.py` 中把 `format_web_data_point(...)` 扩展为可选 `drift=None` 参数，并在返回字典中追加：

```python
"de": 1 if drift.get("enabled", False) else 0,
"da": 1 if drift.get("active", False) else 0,
"dc": drift.get("compensation", 0.0),
"gzf": drift.get("gyroZFiltered", 0.0),
```

这样 Python 测试可覆盖固件 `/api/data` 的字段契约。

### 3. 扩展固件 Web 数据点

在 `mus4.ino` 的 `WebDataPoint` 中追加 Drift 状态字段：

- `bool driftEnabled`
- `bool driftActive`
- `float driftCompensation`
- `float gyroZFiltered`

在 `sampleWifiWebData()` 中从现有固件变量填充：

- `drift_assist_enabled`
- `drift_assist_active`
- `drift_compensation`
- `gyro_z_filtered`

在 `handleWifiWebData()` 的 JSON 输出中追加短字段 `de`、`da`、`dc`、`gzf`。保持现有 `seq/t/thr/str/mode/park/rct/rcs/pt/ps/cur/vol/gz` 字段不变，避免破坏已有前端逻辑。

### 4. 增加 Web Console 状态卡片结构

在 `WIFI_WEB_CONSOLE_HTML` 的现有 wide status panel 中保留 `<div id="status">loading...</div>`，并在它上方新增三张卡片：

- `modeCard`：显示 RC / ASSIST / AUTO。
- `parkCard`：显示 LOCKED / UNLOCKED。
- `driftCard`：显示 OFF / ARMED / ACTIVE，并带补偿强度条。

新增 DOM id：

- `modeValue`、`modeSub`
- `parkValue`、`parkSub`
- `driftValue`、`driftSub`、`driftNeedle`

### 5. 增加 CSS 动效样式

在内嵌 CSS 中新增状态卡片样式，控制重点如下：

- `.stateGrid` 使用响应式网格布局，移动端自动单列或窄列排列。
- `.stateCard` 使用边框、阴影和渐变背景承载状态。
- Mode 按 `mode0/mode1/mode2` 分别显示不同颜色。
- Park 按 `parkLocked/parkUnlocked` 区分锁定和解锁；锁定态使用脉冲强调。
- Drift 按 `driftOff/driftArmed/driftActive` 区分关闭、待命、介入；介入态使用更明显的 pulse/scan 动效。
- `.driftBar` 与 `#driftNeedle` 展示 `dc` 对应的补偿方向与强度。

### 6. 复用 `/api/data` 轮询更新状态

在现有 Web Console JS 中增加 `updateState(p)`：

- Mode 映射：`0 -> RC`、`1 -> ASSIST`、`2 -> AUTO`，未知值显示 `MODE <n>`。
- Park 映射：`p.park ? LOCKED : UNLOCKED`。
- Drift 映射：
  - `!p.de` -> `OFF`
  - `p.de && !p.da` -> `ARMED`
  - `p.da` -> `ACTIVE`
- Drift 副标题显示 `comp=<dc> gzf=<gzf>`。
- Drift 指针把 `dc` 限幅映射到 `[-70, 70]` 范围内的百分比位置。

调整 `pollData()`：每次成功拿到 `/api/data` 后，都用最新数据点调用 `updateState(latest)`。如果用户暂停曲线，只暂停 `points.push(...)` 与 `draw()`，不要暂停状态卡片刷新，也不要阻断 `lastDataSeq` 前进。

### 7. 保持不变的边界

- 不新增 HTTP API，继续使用 `/api/data`。
- 不改无线认证、Park 权限、OTA 命令和 `wireless_console_policy.py` 的命令授权逻辑。
- 不改 `Serial1.printf("T%d:S%d\n", ...)` 协议回传。
- 不改 `PROCESS_COMMAND_LINE` 的 ACK/NACK 协议响应。
- 不改控制融合、Park、Emergency Stop 或 Drift Assist 算法本身，只读取状态用于展示。

## 验证

1. 运行 Python 测试：

```powershell
pytest tests/test_wireless_console_policy.py
```

2. 使用 WSL 加速编译固件：

```powershell
& "C:\Dev\FFE\Baoshan\mus4\arduino-cli-wsl.ps1" -c
```

3. 如连接实机，上传后打开 Web Console 验证：
   - 顶部出现 Mode、Park、Drift 三张状态卡片。
   - 切换控制模式时 Mode 卡片文字、颜色、动效同步变化。
   - Park 锁定/解锁时 Park 卡片同步变化。
   - Drift Assist 启用/介入时 Drift 卡片从 OFF/ARMED/ACTIVE 切换，补偿指针随 `dc` 改变。
   - 点击“暂停曲线”后，曲线停止追加，但三张状态卡片仍继续刷新。
   - `/api/log`、命令输入、OTA、Serial1 `Txx:Sxx` 回传仍正常。
