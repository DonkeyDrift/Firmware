// CloudReporter.cpp — 「找小车」云端心跳上报实现
//
// 依赖：
//   - WiFi（STA 已连接并取得局域网 IP）
//   - HTTPClient + WiFiClientSecure（由独立 FreeRTOS worker 执行同步 HTTPS POST）
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
//    "lan_ip":"192.168.3.x","port":"80","hostname":"mus4-esp","version":"v1.8.79"}
//
// 节奏：首次拿到 IP 立即上报；此后每 5 分钟心跳一次；开机尚未成功上报过时按
// 1 分钟快速重试（失败不写 KV，不额外消耗云端写入额度）；IP 变化时立即补报。
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

static const unsigned long CLOUD_REPORT_INTERVAL_MS = 300000UL;  // 稳态心跳间隔：5 分钟
static const unsigned long CLOUD_REPORT_RETRY_MS = 60000UL;      // 首报成功前的快速重试：1 分钟
static const int CLOUD_REPORT_HTTP_TIMEOUT_MS = 5000;            // 连接 + 总超时：5 秒
static const uint32_t CLOUD_REPORT_TASK_STACK_SIZE = 16384;      // ESP32 Arduino 的 stack 单位为字节
static const uint32_t CLOUD_REPORT_WORKER_POLL_MS = 100;
static const size_t CLOUD_REPORT_HOSTNAME_CAPACITY = 33;

static char reportTaskHostname[CLOUD_REPORT_HOSTNAME_CAPACITY];  // 创建成功后只读
static bool reportTaskStarted = false;        // 仅主循环访问，保证惰性单任务
static portMUX_TYPE reportResultMux = portMUX_INITIALIZER_UNLOCKED;
static bool reportResultPending = false;      // 以下结果字段均为 POD，不跨任务共享 String
static bool reportResultBeginFailed = false;
static int reportResultHttpCode = 0;

struct ReportAttemptResult {
    bool beginFailed;
    int httpCode;
};

/// 拼装上报 JSON。这些字段（硬件 ID / IP / hostname / version）
/// 均不含需转义的字符，直接 String 拼接即可（本工程无 ArduinoJson）。
static String buildReportBody(const String& hostname)
{
    String deviceId = getHardwareId();

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

/// 在 cloud worker 上下文执行一次同步 HTTPS POST，结果只以 POD 返回。
static ReportAttemptResult reportNow(WiFiClientSecure& client, HTTPClient& http, const String& hostname)
{
    ReportAttemptResult result = {false, 0};
    String body = buildReportBody(hostname);

    if (!http.begin(client, CLOUD_REPORT_URL)) {
        http.end();
        result.beginFailed = true;
        return result;
    }
    http.addHeader("Content-Type", "application/json");
    // Cloudflare 会拦截默认的 ESP32 HTTPClient UA（403），伪装成浏览器 UA 才能通过。
    http.addHeader("User-Agent", "Mozilla/5.0 (ESP32) DonkeyDrift-FindCar/1.0");

    result.httpCode = http.POST(body);
    http.end();
    return result;
}

/// 唯一拥有网络对象和上报/重试状态的任务；主循环不参与同步网络操作。
static void cloudReportTask(void* parameter)
{
    String hostname(static_cast<const char*>(parameter));

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);
    http.setTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);

    bool reportedOnce = false;         // 是否已尝试过首次上报
    bool everSucceeded = false;        // 本次开机是否成功上报过（决定是否仍走快速重试）
    bool ipKnown = false;              // 是否已尝试上报过某个 IP
    IPAddress lastReportedIp;          // 上次尝试上报时的局域网 IP
    unsigned long lastReportMs = 0;    // 上次上报时刻（millis()）

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(CLOUD_REPORT_WORKER_POLL_MS));
            continue;
        }
        IPAddress ip = WiFi.localIP();
        if (ip == IPAddress(0, 0, 0, 0)) {
            vTaskDelay(pdMS_TO_TICKS(CLOUD_REPORT_WORKER_POLL_MS));
            continue;
        }

        unsigned long now = millis();
        // 每次重读 localIP，避免网页上留着旧地址；快速重试只在尚未成功时启用。
        bool ipChanged = ipKnown && ip != lastReportedIp;
        unsigned long interval = everSucceeded ? CLOUD_REPORT_INTERVAL_MS : CLOUD_REPORT_RETRY_MS;

        if (!reportedOnce || ipChanged || (now - lastReportMs) >= interval) {
            ReportAttemptResult result = reportNow(client, http, hostname);
            reportedOnce = true;
            lastReportMs = now;
            // 无论成败都记下这次的 IP，避免换 IP 后退化成不停重试。
            lastReportedIp = ip;
            ipKnown = true;
            if (!result.beginFailed && result.httpCode >= 200 && result.httpCode < 300) {
                everSucceeded = true;
            }

            portENTER_CRITICAL(&reportResultMux);
            reportResultBeginFailed = result.beginFailed;
            reportResultHttpCode = result.httpCode;
            reportResultPending = true;
            portEXIT_CRITICAL(&reportResultMux);
        }

        vTaskDelay(pdMS_TO_TICKS(CLOUD_REPORT_WORKER_POLL_MS));
    }
}

/// 仅在主循环消费 worker 的 POD 结果，日志与 Web sink 不进入网络任务。
static void emitPendingReportLog()
{
    bool beginFailed = false;
    int httpCode = 0;

    portENTER_CRITICAL(&reportResultMux);
    if (!reportResultPending) {
        portEXIT_CRITICAL(&reportResultMux);
        return;
    }
    beginFailed = reportResultBeginFailed;
    httpCode = reportResultHttpCode;
    reportResultPending = false;
    portEXIT_CRITICAL(&reportResultMux);

    if (beginFailed) {
        mus4LogLine("cloud", "report begin failed");
    } else {
        mus4Logf("cloud", "POST /report -> %d", httpCode);
    }
}

void update()
{
    emitPendingReportLog();
    if (reportTaskStarted) return;

    // 首次拿到 IP 后才惰性创建唯一 worker；后续节奏和 IP 检测均由 worker 负责。
    if (WiFi.status() != WL_CONNECTED) return;
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0)) return;

    String hostname = WiFi.getHostname();
    if (hostname.length() == 0) {
        hostname = "mus4-esp";
    }
    hostname.toCharArray(reportTaskHostname, CLOUD_REPORT_HOSTNAME_CAPACITY);

    BaseType_t created = xTaskCreatePinnedToCore(
        cloudReportTask,
        "cloud-report",
        CLOUD_REPORT_TASK_STACK_SIZE,
        reportTaskHostname,
        1,
        nullptr,
        0
    );
    if (created == pdPASS) {
        reportTaskStarted = true;
        mus4Logf("cloud", "report armed: %s", CLOUD_REPORT_URL);
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
