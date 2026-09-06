#pragma once
// CloudReporter.h — 「找小车」云端心跳上报
//
// ESP32 连上家里 Wi-Fi 拿到 IP 后立即向 Cloudflare Pages Functions 上报一次，
// 之后每 5 分钟心跳上报一次自己的局域网 IP，供网页「找小车」查询。
//
// 默认关闭：需在 FirmwareConfig.h 打开 ENABLE_CLOUD_REPORT，并在本机的
// WirelessSecrets.h 里提供 CLOUD_REPORT_TOKEN / CLOUD_REPORT_URL。二者缺一
// 时 update() 为空操作。

#include <Arduino.h>

namespace mus4cloud {

/// 每循环调用一次；未启用（开关 / token / URL 未齐）时为空操作。
void update();

/// 是否已完成配置（开关已开且 token / URL 均已定义）。
bool isConfigured();

}  // namespace mus4cloud
