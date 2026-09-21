#pragma once
// CloudReporter.h — 「找小车」云端心跳上报
//
// ESP32 连上家里 Wi-Fi 拿到 IP 后立即向 Cloudflare Pages Functions 上报一次，
// 之后每 5 分钟心跳上报一次自己的局域网 IP，供网页「找 Donkey Car」查询；
// 上报失败时每 1 分钟快速重试，DHCP 换 IP 时立即补报。
//
// 默认关闭：需在 FirmwareConfig.h 打开 ENABLE_CLOUD_REPORT。上报地址默认取
// FirmwareConfig.h 的 CLOUD_REPORT_URL_DEFAULT（公开端点），可在本机
// WirelessSecrets.h 里用 CLOUD_REPORT_URL 覆盖。

#include <Arduino.h>

namespace mus4cloud {

/// 每循环调用一次；未启用（ENABLE_CLOUD_REPORT 未打开）时为空操作。
void update();

/// 是否已启用云端上报（编译期开关）。
bool isConfigured();

}  // namespace mus4cloud
