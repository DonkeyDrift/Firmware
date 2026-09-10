#pragma once

#define WIFI_STA_SSID "你的路由器SSID"
#define WIFI_STA_PASSWORD "你的路由器密码"

// 「找 Donkey Car」云端上报地址**可选覆盖**（配合 FirmwareConfig.h 的 ENABLE_CLOUD_REPORT 使用）。
// 不定义时自动用 FirmwareConfig.h 的 CLOUD_REPORT_URL_DEFAULT（官方 find-dkc 站点），
// 因此本文件缺省也不影响「一键找车」；只有要指向自建端点时才取消注释并填入真实 URL。
// 不再需要 token（网页公开查询）。
// 注意：本文件会被复制为 WirelessSecrets.h 后才参与编译，且 WirelessSecrets.h 不入库。
// #define CLOUD_REPORT_URL "https://find-dkc.pages.dev/report"
