# CHANGELOG.md

## 2026-06-07 v1.5.24

- 固件次版本号从 `v1.5.23` 更新到 `v1.5.24`。
- 新增 Web Console AP tab 下的 AP SSID 配置弹窗，保存后持久化到 NVS。
- 保存 AP SSID 后自动重启 SoftAP，使新 SSID 无需整机重启即可生效。
- Network 齿轮按钮按当前 AP/STA tab 分流，STA tab 继续打开原 STA Wi-Fi 配置。

## 2026-06-05 v1.5.23

- 固件次版本号从 `v1.5.22` 更新到 `v1.5.23`。
- 将 Web Console 的 `RC Channels` 初始状态改为折叠，减少顶部状态区默认占用高度。
- 优化 `STATUS Details` 展开布局，宽屏显示 3 列，中等宽度 2 列，窄屏 1 列。

## 2026-06-05 v1.5.22

- 固件次版本号从 `v1.5.21` 更新到 `v1.5.22`。
- 为 Web Console 的 RC 通道与 STATUS 详情新增 `▸` / `▾` 折叠展示，STATUS 折叠时只保留标题。
- 将 Web Console 的 STATUS 文本展开视图改为 key/value 列表，并保留 `build` 等带空格引号值的完整内容。

## 2026-06-05 v1.5.21

- 固件次版本号从 `v1.5.20` 更新到 `v1.5.21`。
- 将 Web Console 顶部开发模式开关文案从 `DEBUG MODE` 调整为 `DEV MODE`，并同步认证失败提示。
- 收窄 Web Console 的 `MODE` 与 `PARK` 状态卡片到 `flex:0.30`，同时保留原字体大小与内边距。
- 调整 Web Console 标题行底边对齐，使版本号文字与 `MUS4 Web Console` 标题底边对齐。

## 2026-06-01 v1.5.20

- 固件次版本号从 `v1.5.19` 更新到 `v1.5.20`。
- 修复 STA 断开或重连时运行时断开操作扰动 SoftAP，导致 AP 需要多次重试才能连接的问题。
- 为 Windows 连通性探测提供本地 DNS 捕获和 `/connecttest.txt`、`/ncsi.txt` 响应，降低系统因“无 Internet”自动断开 MUS4-DEBUG AP 的概率。

## 2026-06-01 v1.5.19

- 固件次版本号从 `v1.5.18` 更新到 `v1.5.19`。
- Web Console 保存 STA Wi-Fi 后会等待连接结果；连接失败时在页面内悬浮窗显示原因和处理建议。
- 扩展 STA 状态输出，新增连接中状态、失败原因码和失败原因说明，便于 Web Console 与命令行排障。

## 2026-06-01 v1.5.18

- 固件次版本号从 `v1.5.17` 更新到 `v1.5.18`。
- 调整 DEBUG MODE 权限策略：Web Console 操作免 AUTH，但仍保留 Park Locked 安全限制，并在非 Park 或未授权时给出明确弹窗引导。
- 同步更新无线权限策略镜像测试，覆盖 STA 修改、OTA 和控制命令的开发模式免认证行为。

## 2026-06-01 v1.5.17

- 固件次版本号从 `v1.5.16` 更新到 `v1.5.17`。
- 将 Web Console 右上角开发模式开关标签从 `Auto OTA` 改为 `DEBUG MODE`，使其更准确表达开关含义。

## 2026-06-01 v1.5.16

- 固件次版本号从 `v1.5.15` 更新到 `v1.5.16`。
- 修复 Web Console 保存 STA Wi-Fi 配置时立即重连可能中断当前 HTTP 请求，导致浏览器提示 `Failed to fetch` 的问题。

## 2026-06-01 v1.5.15

- 固件次版本号从 `v1.5.14` 更新到 `v1.5.15`。
- 修复 Web Console 的 STA Wi-Fi 配置弹窗在用户输入 SSID 后离焦时，周期刷新可能把输入值覆盖为当前保存值或编译默认值的问题。

## 2026-05-30 v1.5.14

- 固件次版本号从 `v1.5.13` 更新到 `v1.5.14`。
- 新增 Web Console Tub JSON 连续记录与浏览器下载功能，便于采集 CH1-CH6 等遥测样本交给模型分析。
- 压缩 Tub 控件文案，降低 Web Console HTML 体积，规避 HTTP OTA 接近分区末尾写入失败。

## 2026-05-30 v1.5.13

- 固件次版本号从 `v1.5.12` 更新到 `v1.5.13`。
- 保留当前已验证的 WebSocket 曲线实时显示能力，便于通过 HTTP OTA 发布当前稳定固件。
