#pragma once

#define WIFI_STA_SSID "你的路由器SSID"
#define WIFI_STA_PASSWORD "你的路由器密码"

// 「找小车」云端心跳上报配置（配合 FirmwareConfig.h 的 ENABLE_CLOUD_REPORT 使用）。
// 需要时取消注释并填入真实值；token 为共享口令，URL 为 Cloudflare Pages Functions 的 /report 端点。
// 注意：本文件会被复制为 WirelessSecrets.h 后才参与编译，且 WirelessSecrets.h 不入库。
// #define CLOUD_REPORT_URL "https://your-find-car.pages.dev/report"
// #define CLOUD_REPORT_TOKEN "your-shared-token"
