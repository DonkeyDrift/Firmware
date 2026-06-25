# AP 配网后 AP/STA 共存 60 秒补充设计

## 背景

上一版已加入 `wifiStaApplyFromAp` 的 60 秒 grace 机制，并尝试通过 `isWifiWebRequestFromAp()` 判断 `/api/wifi-sta` 请求是否来自 SoftAP。但实机反馈：通过 AP 给 STA 配网后，AP 仍会提前断开，前端无法稳定显示 STA IP。

这说明当前请求来源判定仍不够稳。可能原因包括 ESP32 `WebServer` 在 `WIFI_AP_STA` 下无法稳定通过 `WiFiClient::localIP()` 区分请求入口，或 captive portal/浏览器访问路径导致 Host/localIP 与预期不一致。

用户已明确新的验收要求：**通过 AP 给 STA 配网后，AP 与 STA 必须共存 60 秒。**

## 目标

1. 通过 AP 页面保存 STA 配置并成功连接后，SoftAP 不得在 60 秒内关闭。
2. 前端必须有足够时间通过 AP 页面轮询 `/api/wifi-sta` 并显示 STA IP。
3. 尽量保留 STA 页面重配 STA 的短 grace 行为，不把所有 STA 连接都无条件改成 60 秒。
4. 使用最小改动修复状态机，不重构 AP/STA 生命周期。

## 非目标

- 不重新引入长期 AP+STA 共存。
- 不修改 STA 连接失败恢复 AP 的逻辑。
- 不修改 BOOT 长按清除 STA 的功能。
- 不修改 Web UI 布局或文案。

## 方案 A：后端 AP 配网会话保守判定

新增或重写后端判定函数，例如 `shouldKeepApForStaConfig(sourceArg)`，由 `handleWifiWebStaSet()` 调用。判定不再只依赖 `client.localIP()`。

推荐规则：

1. `sourceArg == "ap"`：直接判定为 AP 配网。
2. 当前 SoftAP IP 有效，且 STA 尚未 connected：判定为 AP 配网。
3. 当前 SoftAP 有客户端连接：判定为 AP 配网。
4. 其它情况：保持普通 STA grace。

伪代码：

```cpp
static bool shouldKeepApForStaConfig(const String& sourceArg)
{
    IPAddress apIp = WiFi.softAPIP();
    bool apOnline = apIp != IPAddress(0, 0, 0, 0);
    if (!apOnline) return false;
    if (sourceArg == "ap") return true;
    if (!ws.staConnected) return true;
    return WiFi.softAPgetStationNum() > 0;
}
```

在 `handleWifiWebStaSet()` 中：

```cpp
bool keepApForStaConfig = shouldKeepApForStaConfig(sourceArg);
...
wifiStaApplyFromAp = keepApForStaConfig;
```

这样通过 AP 发起配网时，即使 `client.localIP()` / Host 头不可靠，只要 SoftAP 在线且 STA 尚未连接完成，就会选择 `WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS = 60000`。

## 数据流

1. 用户连接设备 AP，打开 Web Console。
2. 前端提交 `/api/wifi-sta`。
3. 后端保存 STA 配置前后计算 `keepApForStaConfig = true`。
4. `wifiStaApplyFromAp = true`。
5. `scheduleWifiStaApply()` 延迟执行 STA 连接。
6. STA 拿到有效 IP 后，`updateWifiSta()` 使用：
   ```cpp
   WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS = 60000
   ```
7. AP 保持在线 60 秒，前端继续轮询并展示 STA IP。
8. 60 秒到期后才调用 `stopWifiApForStaOnly()` 关闭 SoftAP。

## 边界条件

- 如果 STA 已经 connected，且请求明显来自 STA 页面，仍可使用短 grace。
- 如果 SoftAP 仍在线且有客户端连接，即使 STA 已 connected，也应保守保留 AP 60 秒，避免 AP 页面被提前踢掉。
- 如果 SoftAP 不在线，不应为了 STA 页面重配而重新开启 AP。
- STA 连接失败路径保持不变，仍恢复 AP。

## 测试计划

先补源码断言测试，再实现：

1. 修改 `tests/test_firmware_feature_flags.py::test_ap_sta_configuration_keeps_ap_open_long_enough_to_show_ip`：
   - 断言存在 `shouldKeepApForStaConfig` 或等价函数。
   - 断言函数使用 `WiFi.softAPIP()` 判断 AP 是否在线。
   - 断言函数使用 `!ws.staConnected` 或等价条件确保 AP 配网初始路径走 60 秒 grace。
   - 断言函数使用 `WiFi.softAPgetStationNum()` 作为 AP 客户端兜底。
   - 断言 `wifiStaApplyFromAp = keepApForStaConfig`。
2. 保留已有断言：
   - `WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS = 60000`。
   - `updateWifiSta()` 按 `wifiStaApplyFromAp ? WIFI_STA_AP_CONFIG_SUCCESS_GRACE_MS : WIFI_STA_GRACE_UP_MS` 选择 grace。
3. 运行：
   ```powershell
   pytest tests/test_firmware_feature_flags.py -k "ap_sta_configuration_keeps_ap_open_long_enough_to_show_ip or web_console_closes_ap_after_sta_grace"
   pytest tests/test_firmware_feature_flags.py
   ```
4. 编译并上传后实机验证：
   - 通过 AP 页面保存 STA。
   - STA 连接成功后，AP 继续广播并可访问约 60 秒。
   - Web 页面显示 STA IP。

## 验收标准

1. 通过 AP 配 STA 成功后，AP 与 STA 共存约 60 秒。
2. 60 秒内 AP 页面可以看到 STA IP。
3. 60 秒后 AP 才关闭，设备继续通过 STA IP 访问。
4. STA 失败或密码错误时仍恢复 AP，不会失联。
