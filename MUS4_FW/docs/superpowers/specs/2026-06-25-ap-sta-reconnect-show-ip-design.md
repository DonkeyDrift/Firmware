# AP 配网保存后接受短暂断连并自动恢复显示 STA IP 设计

## 背景

信道预对齐已让 STA 连接成功时 AP 不再掉线，但带来一个副作用：保存 STA 后，固件会把 SoftAP 从 channel 6 重启切换到目标路由器信道。ESP32 单射频改 AP 信道时，当前连在 `192.168.4.1` 的浏览器客户端会被短暂踢断一次（SSID 仍在，关联被重置）。

这导致前端 `waitWifiStaConnectionResult()` 的一次性 22 秒等待窗口在断连期间所有 `fetch` 失败，无法在原页面直接显示 STA IP。

用户已确认方向：**接受保存时一次短暂断连，但页面应自动恢复，并最终在原页面直接显示 STA IP，无需手动重连。**

由于 ESP32 单射频 + 跨信道场景下改信道必然断一次，这里不追求“零断连”，只追求“断连后自动恢复并显示 IP”。

## 目标

1. 保存 STA 后，明确提示用户页面会短暂断开、请勿手动断开 AP。
2. 前端等待逻辑跨越断连自动恢复，不因断连误判为失败。
3. STA 连上并拿到有效 IP 后，在原页面固定区域直接显示 STA IP 与访问地址。
4. 真正的失败（错误密码、找不到 SSID、超时）仍要明确提示。
5. 后端不改动；复用现有 `/api/wifi-sta` 与每 5 秒的状态轮询。

## 非目标

- 不追求保存时零断连（硬件约束）。
- 不改 Wi-Fi 状态机、信道预对齐、BOOT 长按。
- 不改后端 JSON 结构。

## 设计

仅修改 `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`。

### 1. 保存后提示文案

`saveWifiSta()` 在提交成功、进入等待前，把提示文案改为明确说明：

- 设备正在切换到目标 Wi-Fi 信道；
- 本页面会短暂断开；
- 请保持连接 MUS4-ESP，不要手动断开；
- 连上后会自动显示 STA IP。

### 2. 等待逻辑健壮化

改造 `waitWifiStaConnectionResult()`：

- 总等待时长从 22 秒延长到约 60 秒，覆盖：信道切换断连恢复 + STA 连接（最长 `WIFI_STA_CONNECT_TIMEOUT_MS = 15s`）+ 获取 IP。
- 轮询 `refreshWifiSta(false)` 返回 `null`（断连期间 fetch 失败）时，**不判失败**，继续重试。
- 仅当后端明确返回 `last_error` 或 `timed_out` 时才判失败并弹失败弹窗。
- 一旦 `connected` 且 `sta_ip` 有效且非 `0.0.0.0`：
  - 更新 `staNotice` 文本显示 STA IP 与 `http://<sta_ip>/`；
  - 调用 `refreshStatus()` 让状态卡 STA 标签显示 IP；
  - 弹 handoff modal 展示访问地址。

### 3. 显示位置

复用现有：

- `updateNetworkCard()`：STA 标签显示 `sta_ip`。
- `staNotice`：文本提示。
- `showWifiStaHandoffModal()`：弹窗显示访问地址。

加上已有 `setInterval(refreshStatus,5000)` 和 `setInterval(refreshWifiSta,5000)`，页面恢复后即使等待窗口已结束，也会兜底刷新出 STA IP。

## 数据流

1. 用户在 `192.168.4.1` 保存 STA（携带 scan channel）。
2. 后端保存配置、记录目标信道、返回 200。
3. 前端显示“正在切换信道，页面会短暂断开”提示，进入健壮等待。
4. 固件延迟 apply：预对齐重启 SoftAP 到目标信道。
5. 浏览器到 `192.168.4.1` 的连接短暂中断；等待逻辑持续重试不判失败。
6. 手机/电脑自动跟随 AP 到新信道，恢复访问 `192.168.4.1`。
7. STA 连上路由器、拿到 IP。
8. 等待逻辑或 5 秒轮询拿到 `connected + sta_ip`，在原页面显示 STA IP。

## 边界条件

- 断连时间较长导致 60 秒窗口结束：5 秒后台轮询仍会在恢复后显示 IP；不应在断连期间提前弹失败。
- 错误密码/找不到 SSID：后端返回 `last_error`/`timed_out`，前端弹失败弹窗。
- 手动输入 SSID（无 channel，无预对齐）：不发生信道切换断连，等待逻辑同样适用。
- 用户中途手动断开 AP：无法保证恢复，提示已告知不要手动断开。

## 测试计划

源码断言（`tests/test_firmware_feature_flags.py`）：

1. 保存提示文案包含“切换”“短暂断开”“不要手动断开”等关键词（按最终文案断言）。
2. `waitWifiStaConnectionResult()` 总时长延长：断言不再是 `Date.now()+22000`，而是更长（如 `Date.now()+60000`）。
3. 断连不提前失败：保留 `j=null` 时 `continue` 重试逻辑。
4. 显示 STA IP 的文案与 `refreshStatus()` 调用保留。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -k "web_console_sta_failure_uses_page_modal_and_waits_for_result or web_console_sta_settings_support_scan_and_password_visibility" -q
pytest tests/test_firmware_feature_flags.py -q
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -u
```

## 验收标准

1. 在 `192.168.4.1` 保存扫描选中的 STA 后，页面提示会短暂断开。
2. 断连后页面自动恢复，无需手动重连即可在原页面看到 STA IP。
3. STA IP 与访问地址在页面固定区域显示。
4. 错误密码/找不到 SSID 仍有明确失败提示。
