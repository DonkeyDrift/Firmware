// CloudReporter.cpp — 「找小车」云端心跳上报实现
//
// 依赖：
//   - WiFi（STA 已连接并取得局域网 IP）
//   - HTTPClient + WiFiClientSecure（对 Cloudflare Pages Functions 做同步 HTTPS POST）
//   - AuthService::getHardwareId()（eFuse MAC 派生的 12 位小写十六进制硬件 ID）
//   - BuildInfo.h 的 MUS4_FIRMWARE_VERSION（版本字段）
//
// 上报地址：优先用本地 WirelessSecrets.h 的 CLOUD_REPORT_URL，没有则用
// FirmwareConfig.h 的 CLOUD_REPORT_URL_DEFAULT（公开端点、非机密）。**不得**再让
// 上报能力依赖 gitignore 的本地密钥文件——在干净 worktree 里编译时它不存在，
// 曾导致整块上报代码被静默编译掉（v1.8.77 车上固件就是这种状态）。
//
// 协议（POST /report，JSON body，去 token 公开上报）：
//   {"device_id":"<硬件ID>","type":"esp32",
//    "lan_ip":"192.168.3.46","port":"80","hostname":"mus4-esp","version":"v1.8.78"}
//
// 节奏：首次拿到 IP 立即上报；成功每 5 分钟心跳一次，失败每 1 分钟快速重试
// （失败不写 KV，快速重试不会额外消耗云端写入额度），IP 变化时立即补报。
//
// 默认关闭：仅当 FirmwareConfig.h 定义 ENABLE_CLOUD_REPORT 时才编译真实逻辑。

#include "CloudReporter.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "BuildInfo.h"
#include "AuthService.h"
#include "FirmwareConfig.h"
#include "Mus4Log.h"

// 引入编译期 URL。WirelessSecrets.h 是本机本地文件、不入库，
// 参照 WifiStaConfig.cpp 的 __has_include 模式：提供时优先用它。
#if defined(ENABLE_CLOUD_REPORT)
#if __has_include("WirelessSecrets.h")
#include "WirelessSecrets.h"
#endif
#endif

#if defined(ENABLE_CLOUD_REPORT)

// 没在本地密钥文件里覆盖时，用配置头里的公开默认地址兜底。
#ifndef CLOUD_REPORT_URL
#define CLOUD_REPORT_URL CLOUD_REPORT_URL_DEFAULT
#endif

namespace mus4cloud {

static const unsigned long CLOUD_REPORT_INTERVAL_MS = 300000UL;  // 成功后的心跳间隔：5 分钟
static const unsigned long CLOUD_REPORT_RETRY_MS = 60000UL;      // 失败后的重试间隔：1 分钟
static const int CLOUD_REPORT_HTTP_TIMEOUT_MS = 5000;            // 连接 + 总超时：5 秒

static bool reportedOnce = false;         // 是否已尝试过首次上报
static bool lastReportOk = false;         // 上次上报是否成功（决定下一跳间隔）
static bool ipKnown = false;              // 是否已成功上报过某个 IP
static IPAddress lastReportedIp;          // 上次成功上报时的局域网 IP
static unsigned long lastReportMs = 0;    // 上次上报时刻（millis()）

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

/// 执行一次同步 HTTPS POST；成功（2xx）返回 true。
/// 注意：会在本次调用阻塞约 1-3 秒（低频、仅首报 + 每 5 分钟一次，可接受；
/// 后续可优化为异步/带退避）。
static bool reportNow()
{
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);
    http.setTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);

    String body = buildReportBody();

    if (!http.begin(client, CLOUD_REPORT_URL)) {
        mus4LogLine("cloud", "report begin failed");
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    // Cloudflare 会拦截默认的 ESP32 HTTPClient UA（403），伪装成浏览器 UA 才能通过。
    http.addHeader("User-Agent", "Mozilla/5.0 (ESP32) DonkeyDrift-FindCar/1.0");

    int code = http.POST(body);
    mus4Logf("cloud", "POST /report -> %d", code);
    http.end();
    return code >= 200 && code < 300;
}

void update()
{
    // 未联网 / 未拿到 IP 时不上报。
    if (WiFi.status() != WL_CONNECTED) return;
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0)) return;

    unsigned long now = millis();
    // 首次拿到 IP 立即上报；成功每 5 分钟一跳，失败 1 分钟后重试；
    // DHCP 换 IP 时立即补报（每次重读 localIP，避免网页上留着旧地址）。
    bool ipChanged = ipKnown && ip != lastReportedIp;
    unsigned long interval = lastReportOk ? CLOUD_REPORT_INTERVAL_MS : CLOUD_REPORT_RETRY_MS;

    if (!reportedOnce || ipChanged || (now - lastReportMs) >= interval) {
        if (!reportedOnce) {
            mus4Logf("cloud", "report armed: %s", CLOUD_REPORT_URL);
        }
        bool ok = reportNow();
        reportedOnce = true;
        lastReportOk = ok;
        lastReportMs = now;
        if (ok) {
            lastReportedIp = ip;
            ipKnown = true;
        }
    }
}

bool isConfigured()
{
    return true;
}

}  // namespace mus4cloud

#else  // 未启用：FirmwareConfig.h 未打开 ENABLE_CLOUD_REPORT 时退化为空操作

namespace mus4cloud {

void update() {}

bool isConfigured()
{
    return false;
}

}  // namespace mus4cloud

#endif  // ENABLE_CLOUD_REPORT
