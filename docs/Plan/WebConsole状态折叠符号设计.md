# Web Console 状态折叠符号设计

日期：2026-06-05

## 背景

Web Console 当前会同时展示 CH1-CH6 遥控通道值和一整行 `STATUS key=value ...` 文本。随着 Wi-Fi、OTA、WebSocket、HTTP 统计和版本信息增加，STATUS 行已经过长，影响顶部状态区的可读性。

本设计只调整 Web Console 前端展示层，不改变固件后端 API、串口协议、TCP Console 行为或现有 `/api/status` 文本格式。

## 目标

- 使用清晰、紧凑的折叠符号打开和收起 RC 通道与 STATUS 详情。
- 折叠后只显示标题，不显示右侧摘要内容。
- CH1-CH6 默认展开，便于观察高频遥控输入。
- STATUS 默认折叠，避免长文本挤占页面空间。
- STATUS 展开后解析为 key/value 列表，而不是继续显示一整行原始文本。

## 非目标

- 不修改 `/api/status` 输出格式。
- 不修改 `/api/data` 输出格式。
- 不新增后端字段或命令。
- 不把 STATUS 字段拆成后端结构化 JSON。
- 不改变无线权限策略。

## 折叠符号

采用经典 Chevron 符号：

- `▸`：当前折叠，点击后展开。
- `▾`：当前展开，点击后折叠。

选择原因：

- 符合调试工具和树形列表的常见认知。
- 占用空间小，适合 Web Console 的紧凑状态区。
- 比 `⊞ / ⊟` 视觉重量更低。
- 比“展开 / 收起”文本按钮更适合高密度遥测界面。

## 分组

采用两组折叠：

1. `RC Channels`
   - 包含 CH1 Steering、CH2 Throttle、CH3 Park、CH4 Mode、CH5 Drift、CH6 Scale。
   - 默认展开。
2. `STATUS Details`
   - 包含 `/api/status` 返回的所有 key/value 字段。
   - 默认折叠。

暂不为每个 CH 单独折叠，因为每个通道当前只有一个数值，单独折叠会制造视觉噪声。暂不把 STATUS 继续拆成 Control、OTA、Network、Web Metrics 等子组，避免首次实现过度复杂；后续字段继续增长时可在前端解析层扩展。

## 默认显示

```text
▾ RC Channels
  CH1 Steering   0
  CH2 Throttle   0
  CH3 Park       1500
  CH4 Mode       0
  CH5 Drift      1000
  CH6 Scale      1500

▸ STATUS Details
```

折叠状态只保留标题行：

```text
▸ RC Channels
▸ STATUS Details
```

## 展开 STATUS 后显示

STATUS 展开后，将原始文本解析为 key/value 列表：

```text
▾ STATUS Details
  mode                0
  park                1
  throttle            0
  steering           -1
  wifi_frames         0
  wifi_errors         0
  ota_window          1
  ota_progress        0
  ota_ttl_ms          120000
  dev_mode            1
  park_guard          0
  version             v1.5.21
  build               Jun  5 2026 10:11:09
  web_port            80
  free_heap           123712
  ...
```

`build="Jun  5 2026 10:11:09"` 这类带引号且内部有空格的值需要保持完整，不应被简单空格拆碎。

## 交互规则

- 点击标题整行即可切换展开状态，不要求精确点击箭头。
- 鼠标悬停标题行时应有轻微高亮，提示可点击。
- 标题行应保留键盘可访问性：至少支持 `button` 语义或可聚焦元素，并支持 Enter / Space 切换。
- 展开状态只影响前端显示，不影响后台轮询、WebSocket 接收或日志采集。
- 页面刷新后可以回到默认状态；本次设计不要求持久化折叠状态。

## 数据流

1. `/api/data` 继续按现有逻辑更新 CH1-CH6 数值。
2. 前端把 CH1-CH6 放入 `RC Channels` 内容区。
3. `/api/status` 继续返回原始 `STATUS key=value ...` 文本。
4. 前端解析 STATUS 文本为 key/value 对。
5. `STATUS Details` 展开时渲染 key/value 表；折叠时隐藏内容区。

## 错误处理

- `/api/status` 请求失败时，`STATUS Details` 展开内容显示错误文本，例如 `status error: ...`。
- STATUS 文本解析失败时，保留原始文本作为兜底内容，避免信息丢失。
- 单个字段解析异常不应阻断其他字段显示。
- CH 通道值缺失时继续沿用现有占位策略，例如 `----`。

## 验证口径

- 页面初始打开时，`RC Channels` 为 `▾` 且显示 CH1-CH6；`STATUS Details` 为 `▸` 且不显示右侧摘要。
- 点击 `RC Channels` 标题后，符号变为 `▸`，CH1-CH6 内容隐藏。
- 再次点击 `RC Channels` 标题后，符号变为 `▾`，CH1-CH6 内容恢复。
- 点击 `STATUS Details` 标题后，符号变为 `▾`，STATUS 字段以 key/value 列表显示。
- `build="Jun  5 2026 10:11:09"` 能显示为完整 build 值。
- STATUS 折叠后只显示 `▸ STATUS Details`，不显示任何摘要文本。
- 现有 `/api/status` 与 `/api/data` 后端输出不变。

## 后续扩展

如果 STATUS 字段继续增长，可在不修改后端 API 的前提下，把前端列表进一步分为：

- Control：`mode`、`park`、`throttle`、`steering`。
- OTA：`ota_window`、`ota_progress`、`ota_ttl_ms`、`dev_mode`、`park_guard`。
- Network：`ap_ip`、`ap_clients`、`sta_configured`、`sta_connected`、`sta_ip`。
- Web Metrics：`web_*`、`http_*`。
- WebSocket Metrics：`ws_*`。

本次实现不包含上述子分组。
