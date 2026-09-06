// CloudReporter.cpp — 「找小车」云端心跳上报实现
//
// 依赖：
//   - WiFi（STA 已连接并取得局域网 IP）
//   - HTTPClient + WiFiClientSecure（对 Cloudflare Pages Functions 做同步 HTTPS POST）
//   - AuthService::getHardwareId()（eFuse MAC 派生的 12 位小写十六进制硬件 ID）
//   - BuildInfo.h 的 MUS4_FIRMWARE_VERSION（版本字段）
//
// 协议（POST /report，JSON body，去 token 公开上报）：
//   {"device_id":"<硬件ID>","type":"esp32",
//    "lan_ip":"192.168.3.46","port":"80","hostname":"mus4-esp","version":"v1.8.72"}
//
// 默认关闭：仅当 ENABLE_CLOUD_REPORT、CLOUD_REPORT_URL 二者同时定义时
// 才编译真实逻辑，否则 update() 为空操作。

#include "CloudReporter.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "BuildInfo.h"
#include "AuthService.h"
#include "Mus4Log.h"

// 引入编译期 URL。WirelessSecrets.h 是本机本地文件、不入库，
// 参照 WifiStaConfig.cpp 的 __has_include 模式：提供时宏才生效。
#if defined(ENABLE_CLOUD_REPORT)
#if __has_include("WirelessSecrets.h")
#include "WirelessSecrets.h"
#endif
#endif

#if defined(ENABLE_CLOUD_REPORT) && defined(CLOUD_REPORT_URL)

namespace mus4cloud {

static const unsigned long CLOUD_REPORT_INTERVAL_MS = 300000UL;  // 心跳间隔：5 分钟
static const int CLOUD_REPORT_HTTP_TIMEOUT_MS = 5000;            // 连接 + 总超时：5 秒

static bool reportedOnce = false;            // 是否已完成首次上报
static unsigned long lastReportMs = 0;       // 上次上报时刻（millis()）

/// 拼装上报 JSON。这些字段（硬件 ID / IP / hostname / version）
/// 均不含需转义的字符，直接 String 拼接即可（本工程无 ArduinoJson）。
static String buildReportBody()
{
    String deviceId = getHardwareId();

    String hostname = WiFi.getHostname();
    if (hostname.length() == 0) {
        hostname = "mus4-esp";
    }

    String body;
    body.reserve(200);
    body += "{\"device_id\":\"";
    body += deviceId;
    body += "\",\"type\":\"esp32\",\"lan_ip\":\"";
    body += WiFi.localIP().toString();
    body += "\",\"port\":\"80\",\"hostname\":\"";
    body += hostname;
    body += "\",\"version\":\"";
    body += MUS4_FIRMWARE_VERSION;
    body += "\"}";
    return body;
}

/// 执行一次同步 HTTPS POST。
/// 注意：会在本次调用阻塞约 1-3 秒（低频、仅首报 + 每 5 分钟一次，可接受；
/// 后续可优化为异步/带退避）。
static void reportNow()
{
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);
    http.setTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);

    String body = buildReportBody();

    if (!http.begin(client, CLOUD_REPORT_URL)) {
        mus4LogLine("cloud", "report begin failed");
        return;
    }
    http.addHeader("Content-Type", "application/json");
    // Cloudflare 会拦截默认的 ESP32 HTTPClient UA（403），伪装成浏览器 UA 才能通过。
    http.addHeader("User-Agent", "Mozilla/5.0 (ESP32) DonkeyDrift-FindCar/1.0");

    int code = http.POST(body);
    mus4Logf("cloud", "POST /report -> %d", code);
    http.end();
}

void update()
{
    // 未联网 / 未拿到 IP 时不上报。
    if (WiFi.status() != WL_CONNECTED) return;
    if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;

    unsigned long now = millis();
    // 首次拿到 IP 立即上报一次；之后每 5 分钟心跳一次（每次重读 localIP，
    // 以应对 DHCP 更换 IP）。
    if (!reportedOnce || (now - lastReportMs) >= CLOUD_REPORT_INTERVAL_MS) {
        reportNow();
        reportedOnce = true;
        lastReportMs = now;
    }
}

bool isConfigured()
{
    return true;
}

}  // namespace mus4cloud

#else  // 未启用：开关 / URL 任一缺失时退化为空操作

namespace mus4cloud {

void update() {}

bool isConfigured()
{
    return false;
}

}  // namespace mus4cloud

#endif  // ENABLE_CLOUD_REPORT && CLOUD_REPORT_URL
