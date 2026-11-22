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
// 内存与连接（v1.10.17 最终实车语义）：TLS 对象任务级构造，但每一跳无论
// 成败都在响应后 client.stop() 整体释放；不复用旧 keep-alive 会话。实车证据：
// 首个心跳 200 后，旧会话在下一个 5 分钟心跳稳定 read timeout(-11)，而释放后
// 新建会话的快速重试返回 200。冷态建连受 DRAM(8BIT) 双门限与退避保护，
// 防止每次握手的 ~48.8KB 峰值把池打到 OOM；每次上报回传堆/连续块/栈高水位。
//
// 内存护栏（第三轮收紧）：第二轮纪元模型的运行期复测暴露门限失明——5 次
// 冷态重建尝试门限全过（free8≈81KB、max_blk≈38.9KB）仍全部 -1，且每次把
// 8BIT 池真实低水位打到 176–212 字节（OOM 悬崖）：运行期碎片化后「总量够、
// 连续块不够」，17KB 连续块门限只保证单块 16.4KB 缓冲放得下，装不下两块
// 缓冲 + 握手瞬态。v1.10.16 全程成功恰因会话建立在开机早期的干净堆上。对策：
//   ① 开机早期建连窗口：尚未成功过且开机未满 EARLY_WINDOW 时按 15s 积极重试
//     抢住低碎片窗口（建连仍在 worker 任务内，主 loop 语义不变）；
//   ② 冷态门限收紧为 free8≥64000 且 maxAlloc8≥49000（常量处推算注释）——
//     第二轮失败形态（81KB/38.9KB）被零分配跳过，不再把堆打到 KB 级以下；
//   ③ 跳过原因分桶可观测：free8 不足沿用「skip low heap」日志行，free8 足够
//     但连续块不足打「wait low-frag window」（等待安静期自然去碎片后按
//     60s→120s→300s 阶梯复查门限，不无限硬试）。
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
static const int CLOUD_REPORT_HTTP_TIMEOUT_MS = 5000;            // TCP/TLS 连接超时：5 秒
static const int CLOUD_REPORT_HTTP_READ_TIMEOUT_MS = 10000;      // 响应读取超时：10 秒（慢响应不提前切断）
static const unsigned long CLOUD_REPORT_STALE_RETRY_MS = 15000UL; // 稳态旧连接读失败后，15s 换会话快速重试

// 开机早期建连窗口（第三轮）：堆碎片化由 Web 控制台/遥测客户端接入与运行
// 期分配逐渐累积，开机后前几分钟是低碎片窗口——v1.10.16 全程成功正因会话
// 建立在该窗口内。窗口内（尚未成功过时）用 15s 积极重试抢住窗口，一旦成功
// 立即回 5 分钟稳态心跳；窗口结束后回落 60s→120s→300s 退避阶梯。15s 取值：
// 显著低于 60s 退避基数（抢窗口），又不会密到单次 5s 超时叠加把 worker 占满。
static const unsigned long CLOUD_REPORT_EARLY_WINDOW_MS = 300000UL;
static const unsigned long CLOUD_REPORT_EARLY_RETRY_MS = 15000UL;

// 冷态（重）建连前 DRAM（MALLOC_CAP_8BIT）池门限——第三轮按实测需求收紧，
// 推算依据（第二轮实车取证 + 链接 map 内存论证）：
//   - 单次 TLS（重）建连峰值 ~48.8KB（free_heap 83.5→34.7KB INTERNAL 口径
//     实测），全部由 8BIT 池供给；其中两块 ~16.4KB 连续缓冲（mbedTLS
//     in_buf/out_buf，本 core 无 setBufferSizes 缩减 API）最坏情况同取自
//     最大连续块（33.3KB），加上握手瞬态（ECDH/RSA mpi、证书链解析、握手
//     窗口内 lwIP pbuf，合计 ~15.7KB）≈ 49KB，与实测峰值总量双向印证。
//   - 连续块门限 49000：第二轮 5 次 -1 悬崖案例 max_blk=38.9KB（33.3KB 双
//     缓冲装得下、瞬态只剩 5.6KB 装不下）必须被拒绝——低于该值时握手内部
//     分配必失败，且失败过程中的分配/并发消耗曾把池水位打到 176–212 字节。
//   - 空闲门限 64000 = 峰值 48.8KB + ≥15KB 残余地板：算术上保证单次尝试的
//     TLS 自身消耗不可能把 8BIT 池打到 KB 级以下。注意该门限只适用于窗口
//     结束后的运行期——开机无会话时 free8 实测仅 47–48.5KB（见下方 BOOT
//     门限），早期窗口若套用本值会把干净建连窗口关死。
//   - 门限不满足零分配跳过；碎片化（free8 够、连续块不够）期等待安静期
//     （大块消费者释放后自然去碎片）再按退避阶梯复查，不无限硬试。
static const uint32_t CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD = 64000;
static const uint32_t CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD = 49000;

// 开机早期窗口专用门限（第四轮实车修正）：严格门限只适用于窗口结束后的
// 碎片化期，若一刀切到早期窗口，会把唯一能成功建连的干净窗口关死——
// 第三轮实车（.9 车）早期窗口三次尝试全部
//   `skip low heap free8≈48548 max_blk≈47092`（开机无会话画像），
// 第二轮 free8=47100/47300 也恰好低于当时 48000 门限而错过窗口，之后才在
// 碎片化期硬握手打到 OOM 悬崖。v1.10.16 正是靠在开机窗口建立常驻会话
// 才长期成功，因此早期窗口必须按实测开机画像放行：
//   - free8 ≥ 40000：开机实测 47–48.5KB，留 ~7–8.5KB 余量即可放行；
//   - maxAlloc8 ≥ 34000：两块 16.4KB TLS 连续缓冲（33.3KB）刚好容纳，
//     开机实测 max_blk≈47KB 余量充足；窗口期堆尚未碎片化，握手瞬态可
//     由其余空闲块供给，不套用窗口后的 49KB 单块要求。
// 窗口结束后一律回到上面的严格门限（峰值 48.8KB + 15KB 地板、单块 49KB），
// 宁可跳过等待也不让运行期碎片化握手把 8BIT 池打到 KB 级以下。
static const uint32_t CLOUD_REPORT_MIN_FREE_HEAP8_FOR_BOOT = 40000;
static const uint32_t CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_BOOT = 34000;

// 任务栈 16384 → 8192（第四轮实车修正）：实车日志 stack_hwm=12540 字节，
// 即 16KB 栈在含 TLS 握手的完整上报路径上峰值只用 ~3.85KB——余量长期
// 虚占 12.5KB 的 8BIT 池。而这台车 8BIT 池开机可用只有 ~48.5KB，mbedTLS
// 握手峰值 ~48.8KB，第四轮实车证明「干净窗口内握手也 -1、min_heap8 打到
// 108 字节」正是差这几 KB。砍半后仍留 ~4.3KB（>2 倍实测峰值）栈余量，
// 同时永久归还 8KB 给握手分配；stack_hwm 每次上报继续回传，若实车余量
// 收窄可即时发现。
static const uint32_t CLOUD_REPORT_TASK_STACK_SIZE = 8192;      // ESP32 Arduino 的 stack 单位为字节
static const uint32_t CLOUD_REPORT_WORKER_POLL_MS = 100;
static const size_t CLOUD_REPORT_HOSTNAME_CAPACITY = 33;

static char reportTaskHostname[CLOUD_REPORT_HOSTNAME_CAPACITY];  // 创建成功后只读
static bool reportTaskStarted = false;        // 仅主循环访问，保证惰性单任务
static portMUX_TYPE reportResultMux = portMUX_INITIALIZER_UNLOCKED;
static bool reportResultPending = false;      // 以下结果字段均为 POD，不跨任务共享 String
static bool reportResultBeginFailed = false;
static bool reportResultSkippedLowHeap = false;
static bool reportResultSkippedFrag = false;
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
    bool skippedLowHeap;     // 冷态重建 free8 地板不达标，本次零分配跳过
    bool skippedFrag;        // 冷态重建 free8 足够但 maxAlloc8 连续块不足，零分配跳过（等待安静期去碎片）
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

/// 冷态（重）建连前 DRAM 池空闲地板判据（纯函数，便于测试锁定）。
/// earlyWindow=true 用开机窗口门限（实测开机画像放行），否则用运行期严格门限。
static bool reportHeapFreeFloorOk(uint32_t free8, bool earlyWindow)
{
    return free8 >= (earlyWindow ? CLOUD_REPORT_MIN_FREE_HEAP8_FOR_BOOT
                                 : CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD);
}

/// 冷态（重）建连前 DRAM 池最大连续块判据（纯函数，便于测试锁定）。
static bool reportHeapContiguousOk(uint32_t maxAlloc8, bool earlyWindow)
{
    return maxAlloc8 >= (earlyWindow ? CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_BOOT
                                     : CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD);
}

/// 冷态（重）建连前 DRAM 池组合门限（纯函数，便于测试锁定）。
static bool reportHeapGateOk(uint32_t free8, uint32_t maxAlloc8, bool earlyWindow)
{
    return reportHeapFreeFloorOk(free8, earlyWindow)
        && reportHeapContiguousOk(maxAlloc8, earlyWindow);
}

/// 连续失败退避：60s → 120s → 300s 封顶（= 稳态心跳间隔）。
/// 低堆/碎片跳过与网络失败共用同一阶梯——跳过期间不会每 60s 刷一条日志，
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

/// 下次尝试间隔（纯函数，便于测试锁定）：
///   成功且无失败 → 5 分钟稳态心跳（会话已建立，窗口节奏立即退出）；
///   成功过但首次稳态失败（如旧 keep-alive 连接读超时 -11）→ 15s 后重试，
///     失败分支已 client.stop()，下一次用新会话而不是死等 5 分钟；
///   未成功且仍在开机早期窗口（millis() < EARLY_WINDOW）→ 15s 积极重试，
///     抢住堆碎片化前的低碎片建连窗口；
///   其余连续失败 → 60s→120s→300s 退避阶梯（等待安静期去碎片后复查门限）。
static unsigned long retryIntervalMs(
    unsigned long nowMs, bool everSucceeded, unsigned int consecutiveFailures)
{
    if (everSucceeded && consecutiveFailures == 0) {
        return CLOUD_REPORT_INTERVAL_MS;
    }
    if (everSucceeded && consecutiveFailures == 1) {
        return CLOUD_REPORT_STALE_RETRY_MS;
    }
    if (nowMs < CLOUD_REPORT_EARLY_WINDOW_MS) {
        return CLOUD_REPORT_EARLY_RETRY_MS;
    }
    return retryBackoffMs(consecutiveFailures);
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
    bool epochHealthy = false;         // 每跳结束均清零并释放；保留标志用于统一冷态门限路径

    // TLS 对象只在 worker 上下文构造，避免跨任务共享；但连接不跨心跳复用。
    // 每跳由 reportHeapGateOk 先判断 8BIT 池总量/连续块，达标才 reportNow，
    // 响应后无论成败都 client.stop() 归还 TLS 缓冲。这样既不把旧 keep-alive
    // 带到下一跳（实测会 -11），也不会在内存不足时反复发起 48.8KB 握手。
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(CLOUD_REPORT_HTTP_TIMEOUT_MS);
    http.setTimeout(CLOUD_REPORT_HTTP_READ_TIMEOUT_MS);

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
        // 每次重读 localIP，避免网页上留着旧地址；重试节奏统一由
        // retryIntervalMs 推导：早期窗口 15s 积极重试 → 成功后 5 分钟稳态 →
        // 窗口结束后 60s→120s→300s 退避阶梯（等待安静期去碎片后复查门限）。
        bool ipChanged = ipKnown && ip != lastReportedIp;
        unsigned long interval = retryIntervalMs(now, everSucceeded, consecutiveFailures);

        if (!reportedOnce || ipChanged || (now - lastReportMs) >= interval) {
            ReportAttemptResult result = {};
            // 8BIT 池实测先于任何网络动作：冷态（纪元已释放/尚未建立）时
            // 门限不达标 → 本次零分配跳过，不给 DRAM 池加 48KB 冲击；
            // 跳过原因分桶（free8 地板 vs 连续块不足）供日志区分。
            result.freeHeap8 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
            result.maxAlloc8 = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
            const bool earlyWindow = now < CLOUD_REPORT_EARLY_WINDOW_MS;
            // 已成功过的会话在失败后已整体释放，重连按早期门限放行（实测
            // free8≈54KB/max_blk≈34.8KB 被严格门限误跳过）；严格门限只保护
            // 从未成功过的后期冷启动，避免运行期碎片化反复硬握手。
            const bool gateEarlyWindow = earlyWindow || everSucceeded;
            if (!epochHealthy && !reportHeapGateOk(result.freeHeap8, result.maxAlloc8, gateEarlyWindow)) {
                if (!reportHeapFreeFloorOk(result.freeHeap8, gateEarlyWindow)) {
                    result.skippedLowHeap = true;
                } else {
                    result.skippedFrag = true;
                }
            }
            if (!result.skippedLowHeap && !result.skippedFrag) {
                ReportAttemptResult attempt = reportNow(client, http, hostname);
                result.beginFailed = attempt.beginFailed;
                result.httpCode = attempt.httpCode;
                // 每一跳结束都释放会话：实车证明复用旧 keep-alive 连接到
                // 下一个 5 分钟心跳会稳定 read timeout(-11)，而释放后新建
                // 会话的快速重试能返回 200。宁可让冷态门限控制新建节奏，
                // 也不复用陈旧连接。
                epochHealthy = false;
                client.stop();
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
            if (!result.skippedLowHeap && !result.skippedFrag && !result.beginFailed
                && result.httpCode >= 200 && result.httpCode < 300) {
                everSucceeded = true;
                consecutiveFailures = 0;
            } else {
                consecutiveFailures += 1;
            }
            result.consecutiveFailures = consecutiveFailures;
            result.nextIntervalMs = retryIntervalMs(now, everSucceeded, consecutiveFailures);

            portENTER_CRITICAL(&reportResultMux);
            reportResultBeginFailed = result.beginFailed;
            reportResultSkippedLowHeap = result.skippedLowHeap;
            reportResultSkippedFrag = result.skippedFrag;
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
    bool skippedFrag = false;
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
    skippedFrag = reportResultSkippedFrag;
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
    } else if (skippedFrag) {
        // free8 地板够、连续块不足：运行期碎片化形态，等待安静期自然去碎片
        // 后按退避阶梯复查门限（零分配跳过，不硬试握手）。
        mus4Logf("cloud",
                 "POST /report wait low-frag window free8=%u max_blk=%u next=%lus fails=%u",
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
