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
// 节奏：首次拿到 IP 立即上报；此后每 5 分钟心跳一次；尚未成功上报过时按
// 退避阶梯重试（60s→120s→300s 封顶，低堆跳过计入同一阶梯；失败不写 KV，
// 不额外消耗云端写入额度）；IP 变化时立即补报（同样受低堆门限保护）。
//
// 内存（v1.10.17 回归修复后）：TLS 会话按「纪元」常驻——会话对象任务级构造，
// 心跳经 HTTP keep-alive 复用；任一跳失败立即整体释放（client.stop()），冷态
// 重建走 DRAM 门限 + 退避（详见下方「内存护栏」段与常量处依据注释）。
// v1.10.16 的常驻对象无释放策略、v1.10.17 初版的按次建连均已被实测否定。
// 每次上报实测 free_heap / 历史最低堆 / worker 栈高水位并以 POD 回传主 loop
// 打日志（heap=/min_heap=/stack_hwm=），为栈大小调优与堆基线追溯提供数据。
//
// 内存护栏（v1.10.17 实车回归修复）：上一轮的「按次建连」被实测证伪——
// map 计算本构建 IRAM-only 池稳态 ~34.7KB，INTERNAL 口径稳态 ~85KB 对应
// DRAM（MALLOC_CAP_8BIT，malloc 实际服务的池）仅 ~50.7KB，而一次 TLS 尝试
// 峰值 ~48.8KB：按次建连只有 ~1.9KB 余量，握手内分配屡屡失败（HTTPClient
// 统一报 -1），60s 快速重试曾把该池打到剩 108 字节并两次 PANIC。本 core
// （3.3.10）NetworkClientSecure 无 setBufferSizes / mbedtls 未开
// MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH，缓冲 16KB+16KB 不可缩减，按次建连在该
// core/库组合下结构性不可行。改回常驻会话的「纪元」模型：
//   ① 会话对象任务级常驻，心跳经 HTTP keep-alive 复用（稳态每跳 ~0KB 新分配，
//     重握手仅 ~16KB 瞬时）；任一跳失败立即 client.stop() 整体释放进入冷态；
//   ② 冷态重建走 DRAM 门限（free8/maxAlloc8 双条件），不达标零分配跳过——
//     常驻态仅剩 ~17.7KB，重握手所需 33KB 分配必然失败，算术上杜绝了
//     stale-context 重复 ssl_setup 的 32KB 泄漏路径；
//   ③ 连续失败（含低堆跳过）按 60s→120s→300s 退避封顶，成功一次回 5 分钟
//     节奏；④ 日志补 min_heap8=/max_blk=/next=/fails= 追溯字段。上报内容、
//     云端接口、主 loop 不阻塞的语义均不变。
//
// 默认关闭：仅当 FirmwareConfig.h 定义 ENABLE_CLOUD_REPORT 时才编译真实逻辑。

#include "CloudReporter.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

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

static const unsigned long CLOUD_REPORT_INTERVAL_MS = 300000UL;  // 稳态心跳间隔：5 分钟（也是退避封顶）
static const unsigned long CLOUD_REPORT_RETRY_MS = 60000UL;      // 退避基数：首次失败后 1 分钟起阶梯翻倍
static const int CLOUD_REPORT_HTTP_TIMEOUT_MS = 5000;            // 连接 + 总超时：5 秒

// 冷态（重）建连前 DRAM（MALLOC_CAP_8BIT）池门限——v1.10.17 实车回归
// （.9 车取证 + 链接 map 内存论证）依据：
//   - 单次 TLS（重）建连瞬时实测消耗 ~48.8KB（free_heap 83.5→34.7KB
//     INTERNAL 口径），全部由 8BIT 池供给；稳态该池仅 ~50.7KB（INTERNAL
//     ~85KB − IRAM-only ~34.7KB）。60s 快速重试曾把它打到剩 108 字节并两次
//     PANIC（INTERNAL 口径 min=34712 完全掩盖了这一低水位）。
//   - 48KB 空闲门限（47–50KB 区间取整为 48KB）：冷态重建仅在池内确有整个
//     尝试峰值的空间时进行，配合退避间隔把 1–2s 的低水位窗口限制在安静期；
//     常态稳态 ~50.7KB 可通过，重载期（DC 页面 + 遥测并发）自动让路。
//   - 17KB 最大连续块门限：mbedTLS 的 ssl_setup 需要一块 ~16.4KB 连续
//     in_buf（out_buf 同级），最大块放不下它时 connect() 必失败（HTTPClient
//     报 -1），与其白白分配再回滚，不如在门限处直接跳过。
static const uint32_t CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD = 48000;
static const uint32_t CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD = 17000;

// 栈大小保持 16384 字节:同步 HTTPS 里 WiFiClientSecure 握手的调用深度需要
// 余量(mbedTLS 握手栈占用常见 6–10KB,叠加 HTTPClient 调用深度)。v1.10.17
// 起每次上报实测栈高水位并通过 "cloud" 日志的 stack_hwm 字段回传,若实车
// 数据长期显示富余,再据此收敛——先测量后调整,不盲砍。
static const uint32_t CLOUD_REPORT_TASK_STACK_SIZE = 16384;      // ESP32 Arduino 的 stack 单位为字节
static const uint32_t CLOUD_REPORT_WORKER_POLL_MS = 100;
static const size_t CLOUD_REPORT_HOSTNAME_CAPACITY = 33;

static char reportTaskHostname[CLOUD_REPORT_HOSTNAME_CAPACITY];  // 创建成功后只读
static bool reportTaskStarted = false;        // 仅主循环访问，保证惰性单任务
static portMUX_TYPE reportResultMux = portMUX_INITIALIZER_UNLOCKED;
static bool reportResultPending = false;      // 以下结果字段均为 POD，不跨任务共享 String
static bool reportResultBeginFailed = false;
static bool reportResultSkippedLowHeap = false;
static int reportResultHttpCode = 0;
static uint32_t reportResultFreeHeapAfter = 0;
static uint32_t reportResultMinFreeHeap = 0;
static uint32_t reportResultMinFreeHeap8 = 0;
static uint32_t reportResultFreeHeap8 = 0;
static uint32_t reportResultMaxAlloc8 = 0;
static uint32_t reportResultStackHwmBytes = 0;
static unsigned long reportResultNextIntervalMs = 0;
static unsigned int reportResultConsecutiveFailures = 0;

struct ReportAttemptResult {
    bool beginFailed;
    bool skippedLowHeap;     // 冷态重建门限不达标，本次零分配跳过
    int httpCode;
    uint32_t freeHeapAfter;   // 本次上报结束后的 free_heap 实测（INTERNAL 口径）
    uint32_t minFreeHeap;     // 开机以来最低 free_heap 实测（INTERNAL 口径，沿用旧字段）
    uint32_t minFreeHeap8;    // 开机以来 DRAM(8BIT) 池最低水位——与 STATUS min_free_heap 同源
    uint32_t freeHeap8;       // 尝试前 DRAM(8BIT) 池空闲（跳过时即判定依据）
    uint32_t maxAlloc8;       // 尝试前 DRAM(8BIT) 池最大连续块
    uint32_t stackHwmBytes;   // worker 任务栈高水位（历史最少剩余字节）
    unsigned long nextIntervalMs;   // 本次结果推导出的下次重试间隔
    unsigned int consecutiveFailures;  // 连续失败次数（含低堆跳过），成功清零
};

/// 冷态（重）建连前 DRAM 池门限判据（纯函数，便于测试锁定）。
static bool reportHeapGateOk(uint32_t free8, uint32_t maxAlloc8)
{
    return free8 >= CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD
        && maxAlloc8 >= CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD;
}

/// 连续失败退避：60s → 120s → 300s 封顶（= 稳态心跳间隔）。
/// 低堆跳过与网络失败共用同一阶梯——低堆期间不会每 60s 刷一条跳过日志，
/// 成功一次后由调用侧回落到 5 分钟节奏。
static unsigned long retryBackoffMs(unsigned int consecutiveFailures)
{
    if (consecutiveFailures == 0) {
        return CLOUD_REPORT_RETRY_MS;
    }
    if (consecutiveFailures == 1) {
        return CLOUD_REPORT_RETRY_MS * 2;
    }
    return CLOUD_REPORT_INTERVAL_MS;
}

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
    ReportAttemptResult result = {};
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

    bool reportedOnce = false;         // 是否已尝试过首次上报
    bool everSucceeded = false;        // 本次开机是否成功上报过（成功后退回 5 分钟稳态节奏）
    bool ipKnown = false;              // 是否已尝试上报过某个 IP
    IPAddress lastReportedIp;          // 上次尝试上报时的局域网 IP
    unsigned long lastReportMs = 0;    // 上次上报时刻（millis()）
    unsigned int consecutiveFailures = 0;  // 连续失败次数（含低堆跳过），成功清零
    bool epochHealthy = false;         // 常驻会话纪元：上一跳是否成功（会话可 keep-alive 复用）

    // v1.10.17 回归修复后的「纪元」常驻模型（替换上一轮被实测证伪的按次建连，
    // 内存论证见文件头）：会话对象任务级构造并跨心跳常驻——稳态心跳经 HTTP
    // keep-alive 复用，每跳 ~0KB 新分配（重握手也仅 ~16KB 瞬时，无需重新
    // malloc 两块 16.4KB TLS 缓冲）。释放/重建策略显式化：任一跳失败立即
    // client.stop() 整体释放进入冷态；冷态重建须过 reportHeapGateOk 门限
    // （DRAM 池 free8/maxAlloc8 双条件），不达标零分配跳过。防泄漏论证：
    // 常驻态（会话健康）下 8BIT 池仅剩 ~17.7KB，若 TCP 中途死亡导致
    // connect() 在未 stop 的 ssl_ctx 上重复 ssl_setup，33KB 的新缓冲分配
    // 必然失败——分配失败即无指针覆盖，旧缓冲不丢；该跳报错后下面的
    // client.stop() 把整个上下文干净释放。泄漏路径在算术上不可达。
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);
    http.setTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);

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
        // 每次重读 localIP，避免网页上留着旧地址；快速重试走退避阶梯
        // （60s→120s→300s 封顶），首次成功后回到 5 分钟稳态节奏。
        bool ipChanged = ipKnown && ip != lastReportedIp;
        unsigned long interval = everSucceeded ? CLOUD_REPORT_INTERVAL_MS : retryBackoffMs(consecutiveFailures);

        if (!reportedOnce || ipChanged || (now - lastReportMs) >= interval) {
            ReportAttemptResult result = {};
            // 8BIT 池实测先于任何网络动作：冷态（纪元已释放/尚未建立）时
            // 门限不达标 → 本次零分配跳过，不给 DRAM 池加 48KB 冲击。
            result.freeHeap8 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
            result.maxAlloc8 = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
            if (!epochHealthy && !reportHeapGateOk(result.freeHeap8, result.maxAlloc8)) {
                result.skippedLowHeap = true;
            } else {
                ReportAttemptResult attempt = reportNow(client, http, hostname);
                result.beginFailed = attempt.beginFailed;
                result.httpCode = attempt.httpCode;
                if (!attempt.beginFailed && attempt.httpCode >= 200 && attempt.httpCode < 300) {
                    epochHealthy = true;   // keep-alive 会话保住，下一跳零大分配
                } else {
                    epochHealthy = false;  // 纪元终结
                    client.stop();         // 整体释放 TLS 上下文，进入冷态
                }
            }
            // 栈/堆实测(v1.10.16 的诊断盲区):高水位只在本任务上下文读才有
            // 意义;结果仅以 POD 回传,主 loop 消费时才写日志(见 emitPendingReportLog)。
            result.freeHeapAfter = ESP.getFreeHeap();
            result.minFreeHeap = ESP.getMinFreeHeap();
            result.minFreeHeap8 = (uint32_t)esp_get_minimum_free_heap_size();
            result.stackHwmBytes = (uint32_t)(uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t));
            reportedOnce = true;
            lastReportMs = now;
            // 无论成败都记下这次的 IP，避免换 IP 后退化成不停重试。
            lastReportedIp = ip;
            ipKnown = true;
            if (!result.skippedLowHeap && !result.beginFailed
                && result.httpCode >= 200 && result.httpCode < 300) {
                everSucceeded = true;
                consecutiveFailures = 0;
            } else {
                consecutiveFailures += 1;
            }
            result.consecutiveFailures = consecutiveFailures;
            result.nextIntervalMs = everSucceeded ? CLOUD_REPORT_INTERVAL_MS : retryBackoffMs(consecutiveFailures);

            portENTER_CRITICAL(&reportResultMux);
            reportResultBeginFailed = result.beginFailed;
            reportResultSkippedLowHeap = result.skippedLowHeap;
            reportResultHttpCode = result.httpCode;
            reportResultFreeHeapAfter = result.freeHeapAfter;
            reportResultMinFreeHeap = result.minFreeHeap;
            reportResultMinFreeHeap8 = result.minFreeHeap8;
            reportResultFreeHeap8 = result.freeHeap8;
            reportResultMaxAlloc8 = result.maxAlloc8;
            reportResultStackHwmBytes = result.stackHwmBytes;
            reportResultNextIntervalMs = result.nextIntervalMs;
            reportResultConsecutiveFailures = result.consecutiveFailures;
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
    bool skippedLowHeap = false;
    int httpCode = 0;
    uint32_t freeHeapAfter = 0;
    uint32_t minFreeHeap = 0;
    uint32_t minFreeHeap8 = 0;
    uint32_t freeHeap8 = 0;
    uint32_t maxAlloc8 = 0;
    uint32_t stackHwmBytes = 0;
    unsigned long nextIntervalMs = 0;
    unsigned int consecutiveFailures = 0;

    portENTER_CRITICAL(&reportResultMux);
    if (!reportResultPending) {
        portEXIT_CRITICAL(&reportResultMux);
        return;
    }
    beginFailed = reportResultBeginFailed;
    skippedLowHeap = reportResultSkippedLowHeap;
    httpCode = reportResultHttpCode;
    freeHeapAfter = reportResultFreeHeapAfter;
    minFreeHeap = reportResultMinFreeHeap;
    minFreeHeap8 = reportResultMinFreeHeap8;
    freeHeap8 = reportResultFreeHeap8;
    maxAlloc8 = reportResultMaxAlloc8;
    stackHwmBytes = reportResultStackHwmBytes;
    nextIntervalMs = reportResultNextIntervalMs;
    consecutiveFailures = reportResultConsecutiveFailures;
    reportResultPending = false;
    portEXIT_CRITICAL(&reportResultMux);

    // 追溯字段说明：heap/min_heap 沿用 INTERNAL 口径（与历史日志可比）；
    // min_heap8 与 STATUS min_free_heap 同源（DRAM 池真实低水位，
    // v1.10.17 取证时 INTERNAL 口径的 34712 掩盖了 8BIT 池的 108 字节）；
    // max_blk 为尝试前最大连续块；next/fails 给出退避节奏，下次崩溃可对表。
    unsigned long nextSec = nextIntervalMs / 1000UL;
    if (skippedLowHeap) {
        mus4Logf("cloud",
                 "POST /report skip low heap free8=%u max_blk=%u next=%lus fails=%u",
                 (unsigned)freeHeap8, (unsigned)maxAlloc8,
                 nextSec, consecutiveFailures);
    } else if (beginFailed) {
        mus4Logf("cloud",
                 "POST /report begin failed heap=%u min_heap=%u min_heap8=%u max_blk=%u stack_hwm=%u next=%lus fails=%u",
                 (unsigned)freeHeapAfter, (unsigned)minFreeHeap, (unsigned)minFreeHeap8,
                 (unsigned)maxAlloc8, (unsigned)stackHwmBytes,
                 nextSec, consecutiveFailures);
    } else {
        mus4Logf("cloud",
                 "POST /report -> %d heap=%u min_heap=%u min_heap8=%u max_blk=%u stack_hwm=%u next=%lus fails=%u",
                 httpCode,
                 (unsigned)freeHeapAfter, (unsigned)minFreeHeap, (unsigned)minFreeHeap8,
                 (unsigned)maxAlloc8, (unsigned)stackHwmBytes,
                 nextSec, consecutiveFailures);
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
