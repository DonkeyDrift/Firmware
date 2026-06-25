# STA 连上后把 IP 编码进 AP 名显示 10 秒设计

## 背景

前面尝试过 60 秒 AP/STA 共存 + 信道预对齐 + 前端跨网 mDNS 探测，让用户在 `192.168.4.1` 页面看 STA IP。但实测在“笔记本带网线不自动重连 AP + Windows 对 `.local` 解析不通 + 断网后页面变成 `chrome-error://` 错误页”的环境下全部失效：任何依赖“页面 JS 持续存活 / 浏览器跨网 / mDNS”的方案都不可靠。

用户确认改用**带外**方式：STA 连上拿到 IP 后，把 SoftAP 名字临时改成包含 IP（如 `MUS4-192.168.1.50`），用户在电脑/手机 Wi-Fi 列表里直接看到 IP，不依赖页面、不依赖重连、不依赖 mDNS。这个带 IP 的 AP 只显示 10 秒，然后自动关闭进入 STA-only。

## 目标

1. 任何 STA 首次连上并拿到有效 IP（首次配网 **或** 重启后自动重连），都把 SoftAP 名改成 `MUS4-<sta_ip>`。
2. 该带 IP 的 AP 在 STA 所在信道广播 **10 秒**，然后自动关闭 AP。
3. 移除前端 mDNS 跨网探测与对应后端 CORS 头。
4. 保留信道预对齐（让 STA 连接更稳）。
5. STA 失败/断开/清除路径仍恢复正常基础 AP 名用于重配。

## 非目标

- 不再依赖 `192.168.4.1` 页面实时显示 STA IP。
- 不保证带 IP 的 AP 期间原页面客户端不掉线（改 SSID 必然让旧 AP 客户端掉，符合“去 Wi-Fi 列表看名字”的用法）。
- 不改 BOOT 长按恢复。

## 设计

### 常量（WifiConsoleTypes.h）

- 新增 `WIFI_STA_IP_DISPLAY_MS = 10000`：STA 连上后带 IP AP 的显示时长。
- 新增前缀 `WIFI_STA_IP_AP_PREFIX = "MUS4-"`。
- 移除不再使用的 `WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS`（原 60 秒）。`WIFI_STA_GRACE_UP_MS` 保留定义但 connected 分支不再用于选择窗口。

### SoftAP 启动支持 SSID 覆盖（WifiManager.cpp）

- `startWifiApServices(const char* logPrefix, uint8_t channel, const char* ssidOverride)` 增加可选 `ssidOverride`：
  - `ssidOverride` 为空时用 `getActiveWifiApSsid()`（基础名）；
  - 非空时用该 SSID 启动 SoftAP。
- `getActiveWifiApSsid()` 仍返回基础名；带 IP 名只在显示窗口通过 `ssidOverride` 传入，确保失败/关闭后恢复基础名。

### 显示 STA IP 到 AP 名

新增 `static void showStaIpInApName()`：

```cpp
String ip = WiFi.localIP().toString();
String ssid = String(WIFI_STA_IP_AP_PREFIX) + ip;   // MUS4-192.168.1.50
uint8_t ch = WiFi.channel();
if (ch < 1 || ch > 14) ch = WIFI_CONSOLE_CHANNEL;
wifiCaptiveDnsServer.stop();
WiFi.softAPdisconnect(false);
delay(100);
startWifiApServices("AP shows STA IP", ch, ssid.c_str());
```

SSID 超 32 字节时截断（`MUS4-` + 最长 15 字符 IP = 20 字节，正常不会超）。

### updateWifiSta 连接成功分支

STA 首次进入 `WL_CONNECTED` 且 `WiFi.localIP()` 有效时：

- 保留现有 connected 标记、mDNS/发现服务启动、提示音。
- 调用 `showStaIpInApName()` 把 AP 名改成 `MUS4-<ip>`。
- 武装 `wifiStaUpGraceDeadlineMs = millis() + WIFI_STA_IP_DISPLAY_MS`（10 秒），不再区分 `wifiStaApplyFromAp`。
- 10 秒到期仍由现有 `stopWifiApForStaOnly()` 关闭 AP（进入 STA-only）。

这样首次配网和重启自动重连共用同一路径。

### 信道预对齐保留

`prealignWifiApChannelForStaApply()` 与 `wifiStaTargetChannel` 保留：STA begin 前把 AP 对齐到目标信道，连接更稳；连上后 `showStaIpInApName()` 用 `WiFi.channel()` 与 STA 同信道广播带 IP 名。

### 移除前端 mDNS 探测与后端 CORS

- `WebConsoleAssets.h`：删除 `staProbeMdnsUrl` 及 `waitWifiStaConnectionResult` 中的 mDNS 跨网 `fetch`，恢复为只轮询相对路径 `/api/wifi-sta`（页面仍开着时可用，但不再是主渠道）。
- 保存提示改为引导带外查看：“设备正在连接 Wi-Fi。连上后请在电脑/手机的 Wi-Fi 列表中查看名为 `MUS4-<设备IP>` 的网络，即为设备的局域网 IP（约 10 秒后该 AP 自动关闭）。”
- `WebConsoleServer.cpp`：移除 `handleWifiWebSta()` 的 `Access-Control-Allow-Origin` / `Access-Control-Allow-Private-Network` 头。

## 数据流

1. 用户在 AP 页面保存 STA（携带 scan channel）。
2. 固件保存配置、预对齐切信道、发起 STA 连接。
3. STA 连上路由器、拿到 IP（如 192.168.1.50）。
4. 固件把 SoftAP 改名为 `MUS4-192.168.1.50`，在 STA 信道广播。
5. 用户在 Wi-Fi 列表看到该名字，得到设备 IP。
6. 10 秒后 AP 自动关闭，设备进入 STA-only，可经 IP 访问。
7. 重启后若 STA 自动重连成功，重复 4–6。

## 边界条件

- STA 连上但 `localIP()` 暂为 0：等待下一轮再改名（沿用现有“等有效 IP”判断）。
- 改 AP 名会断开当前 AP 客户端：预期行为，用户改看 Wi-Fi 列表。
- STA 失败/断开/清除：`restoreApAfterStaLost()` 恢复基础 AP 名，清 `wifiStaTargetChannel`。
- 10 秒窗口期间 STA 抖动断开：沿用现有 down grace；恢复基础 AP 名。

## 测试计划

源码断言（`tests/test_firmware_feature_flags.py`）：

1. `WIFI_STA_IP_DISPLAY_MS = 10000` 与 `WIFI_STA_IP_AP_PREFIX = "MUS4-"` 存在；`WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS` 不再存在。
2. `startWifiApServices` 支持 `ssidOverride` 参数。
3. `showStaIpInApName()` 构造 `String(WIFI_STA_IP_AP_PREFIX) + ...` 并调用 `startWifiApServices(..., ssid...)`。
4. `updateWifiSta` connected 分支调用 `showStaIpInApName()` 并武装 `WIFI_STA_IP_DISPLAY_MS`。
5. 前端不再含 `staProbeMdnsUrl` / `mode:'cors'`；保存提示含 `MUS4-` 与 Wi-Fi 列表引导。
6. 后端 `handleWifiWebSta()` 不再含 CORS 头。
7. 更新原 60 秒 grace 相关断言。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -q
pytest tests/test_wireless_console_policy.py -q
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -u
```

## 验收标准

1. STA 连上后，电脑/手机 Wi-Fi 列表出现 `MUS4-<设备IP>`，可直接读出 IP。
2. 该带 IP 的 AP 约 10 秒后自动关闭。
3. 重启后 STA 自动重连成功，同样广播带 IP 名 10 秒后关闭。
4. STA 失败/找不到 SSID 时恢复正常基础 AP 名用于重配。
5. 不再依赖 mDNS / 页面跨网探测。
