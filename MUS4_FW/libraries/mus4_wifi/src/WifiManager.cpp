#include "WifiManager.h"

#ifdef ENABLE_WIFI_CONSOLE

#include "WifiConsoleTypes.h"
#include "WifiIdentity.h"
#include "WifiOta.h"
#include "WifiStaConfig.h"
#include "WifiStaHistory.h"
#include "WebLogBuffer.h"
#include "WirelessConsole.h"
#include "Mus4Log.h"
#include "RuntimeState.h"
#include "StringPrint.h"
#include "Buzzer.h"
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <string.h>
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
#include <NetBIOS.h>
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
#include <WiFiUdp.h>
#endif

// Hardware objects defined in MUS4_FW.ino
extern Preferences mus4Prefs;
extern WiFiServer wifiConsoleServer;
extern WiFiClient wifiConsoleClient;
extern WebServer wifiWebServer;
extern DNSServer wifiCaptiveDnsServer;
extern Buzzer buzzer;

// Runtime state defined in MUS4_FW.ino
extern WifiRuntimeState wifiRuntime;
extern OtaRuntimeState otaRuntime;

// Wi-Fi runtime aliases (defined in MUS4_FW.ino)
extern bool& wifiConsoleStarted;
extern bool& wifiConsoleAuthenticated;
extern bool& wifiStaConfigured;
extern bool& wifiStaConnected;
extern bool& wifiStaTimedOut;
extern bool& wifiStaConnecting;
extern char* const wifiStaLastError;
extern char* const wifiStaLastErrorMessage;
extern bool& wifiStaApplyPending;
extern bool& wifiStaApplyFromAp;
extern uint8_t& wifiStaTargetChannel;
extern bool& wifiApRestartPending;
extern bool& wifiMdnsStarted;
extern bool& wifiStaHandoffActive;
extern char* const wifiStaHandoffTargetSsid;
extern char* const wifiStaHandoffStaIp;
extern char* const wifiStaHandoffApSsid;
extern unsigned long& wifiStaHandoffStartedMs;
extern bool& wifiDevModeEnabled;
extern char* const wifiApSsid;
extern char* const wifiStaSsid;
extern char* const wifiStaPassword;
extern bool& wifiStaPasswordSet;
extern unsigned long& lastWifiConsoleStartAttemptMs;
extern unsigned long& wifiStaConnectStartMs;
extern unsigned long& wifiStaApplyDeadlineMs;
extern unsigned long& wifiApRestartDeadlineMs;
extern unsigned long& wifiStaUpGraceDeadlineMs;
extern unsigned long& wifiStaDownGraceDeadlineMs;
extern bool& wifiInApOnlyMode;
extern bool& wifiOtaStarted;
extern bool& wifiOtaWindowOpen;
extern bool& wifiOtaInProgress;
extern bool& wifiOtaParkGuardActive;
extern unsigned long& wifiOtaDeadlineMs;
extern uint8_t& wifiOtaLastProgressPct;

// Shared serial buffer (defined in MUS4_FW.ino)
extern SerialBuf wifiConsoleBuf;

// Shared web data (defined in MUS4_FW.ino)
extern WebDataPoint wifiWebData[];
extern uint16_t wifiWebDataHead;
extern uint16_t wifiWebDataCount;
extern uint32_t wifiWebDataSeq;

// Wi-Fi scan cache (defined in MUS4_FW.ino)
extern WifiScanEntry wifiScanCache[];
extern uint8_t wifiScanCacheCount;

#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
static bool wifiNetbiosStarted = false;
#endif

static unsigned long bootWifiResetPressedAtMs = 0;
static bool bootWifiResetTriggered = false;

// STA 连上后临时覆盖 AP SSID 为 "MUS4-<sta_ip>"；非空时 getActiveWifiApSsid()
// 返回此名字，配合一次 SoftAP 重启把带 IP 名广播出去。关闭/失败恢复 AP 时清空。
static String g_staIpApSsidOverride = "";

// STA 连接历史重试节奏：两次扫描/尝试之间的最小间隔，避免 AP-only 下空转扫频。
static const unsigned long WIFI_STA_HISTORY_RETRY_INTERVAL_MS = 3000;

// 一轮历史候选试完后，等待该冷却时间再重开新一轮扫描，覆盖「小车先开机、
// 历史 Wi-Fi 后出现（或暂时不在覆盖范围）」的场景；15s 是扫描频率与空转功耗的折中。
static const unsigned long WIFI_STA_HISTORY_RESCAN_INTERVAL_MS = 15000;

// --- Drift/Judge 设置 NVS 单次写 blob ---
// 行车中在 Drift/Judge 设置页点保存时，HTTP handler 在唯一主循环里同步写 NVS；
// 旧实现逐键 put（drift 12 键 / judge 10 键），每次 put 都触发一次 nvs_commit 写
// flash，整段数十~数百 ms 内控制输出被推迟、车会瞬时停顿。改为把整组参数打包进
// 定长 blob 一次 putBytes（单次 nvs_commit 写 flash）；load 优先读 blob，读不到
// 回退旧逐键格式——老车已调好的参数无损保留。旧键保留不删，便于回滚旧固件。
static const char* MUS4_PREF_JUDGE_CFG_BLOB_KEY = "judge_cfg";
static const char* MUS4_PREF_DRIFT_CFG_BLOB_KEY = "drift_cfg";
// blob 格式版本：将来扩展字段时递增，load 只接受版本与长度完全匹配的 blob。
static const uint8_t MUS4_CFG_BLOB_VERSION = 1;

struct JudgeConfigBlob {
    uint8_t version;      // 格式版本，当前为 MUS4_CFG_BLOB_VERSION
    uint8_t windowSize;
    uint8_t reserved[2];  // 对齐预留，固定写 0
    float collisionThreshold;
    float bigTurnThreshold;
    float collisionPenalty;
    float turnSmoothnessWeight;
    float rangeMatchWeight;
    float gyroStabilityWeight;
    float bigTurnStabilityWeight;
    float speedStabilityWeight;
    float throttleStabilityWeight;
};
static_assert(sizeof(JudgeConfigBlob) == 40, "JudgeConfigBlob layout changed");

struct DriftConfigBlob {
    uint8_t version;      // 格式版本，当前为 MUS4_CFG_BLOB_VERSION
    int8_t steeringGyroSign;
    uint8_t reserved[2];  // 对齐预留，固定写 0
    float maxYawRate;
    float kp;
    float kd;
    float maxSteeringCorrection;
    float gyroFilterAlpha;
    float spinThreshold;
    float steeringThreshold;
    float continuousThrottle;
    float pulseThrottle;
    float pulseFreqHz;
    float pulseDuty;
};
static_assert(sizeof(DriftConfigBlob) == 48, "DriftConfigBlob layout changed");

#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
static WiFiUDP wifiLlmnrUdp;
static bool wifiLlmnrStarted = false;
static const uint16_t LLMNR_PORT = 5355;
static const IPAddress LLMNR_MULTICAST_IP(224, 0, 0, 252);

static bool isLlmnrQueryForHost(const uint8_t* data, uint16_t len, const String& host)
{
    if (len < host.length() + 5) return false;
    uint8_t labelLen = data[0];
    if (labelLen != host.length()) return false;
    for (uint8_t i = 0; i < labelLen; i++) {
        if (tolower(data[1 + i]) != tolower(host[i])) return false;
    }
    if (data[1 + labelLen] != 0) return false;
    uint16_t qtype = (data[2 + labelLen] << 8) | data[3 + labelLen];
    uint16_t qclass = (data[4 + labelLen] << 8) | data[5 + labelLen];
    return qtype == 0x0001 && qclass == 0x0001;
}

static void processLlmnrPacket()
{
    if (!wifiLlmnrStarted) return;
    int packetSize = wifiLlmnrUdp.parsePacket();
    if (packetSize < 9) return;

    uint8_t buffer[256];
    int len = wifiLlmnrUdp.read(buffer, sizeof(buffer));
    if (len < 9) return;

    // Must be a query (QR bit clear)
    if (len >= 3 && (buffer[2] & 0x80)) return;

    String host = wifiMdnsHostText();
    if (len < 12 + (int)host.length() + 5) return;
    if (!isLlmnrQueryForHost(buffer + 12, len - 12, host)) return;

    IPAddress remoteIp = wifiLlmnrUdp.remoteIP();
    uint16_t remotePort = wifiLlmnrUdp.remotePort();
    uint16_t queryId = (buffer[0] << 8) | buffer[1];

    uint8_t response[128];
    uint16_t idx = 0;

    // Header
    response[idx++] = (queryId >> 8) & 0xFF;
    response[idx++] = queryId & 0xFF;
    response[idx++] = 0x80; // QR=1 (response)
    response[idx++] = 0x00;
    response[idx++] = 0x00; // QDCOUNT = 1
    response[idx++] = 0x01;
    response[idx++] = 0x00; // ANCOUNT = 1
    response[idx++] = 0x01;
    response[idx++] = 0x00; // NSCOUNT = 0
    response[idx++] = 0x00;
    response[idx++] = 0x00; // ARCOUNT = 0
    response[idx++] = 0x00;

    // Question section (embedded name at offset 12 for C00C pointer)
    uint8_t nameLen = host.length();
    response[idx++] = nameLen;
    for (uint8_t i = 0; i < nameLen; i++) {
        response[idx++] = tolower(host[i]);
    }
    response[idx++] = 0x00;
    response[idx++] = 0x00; // Type A
    response[idx++] = 0x01;
    response[idx++] = 0x00; // Class IN
    response[idx++] = 0x01;

    // Answer section (pointer C00C to question name)
    response[idx++] = 0xC0;
    response[idx++] = 0x0C;
    response[idx++] = 0x00; // Type A
    response[idx++] = 0x01;
    response[idx++] = 0x00; // Class IN
    response[idx++] = 0x01;
    response[idx++] = 0x00; // TTL = 300
    response[idx++] = 0x00;
    response[idx++] = 0x01;
    response[idx++] = 0x2C;
    response[idx++] = 0x00; // RDLENGTH = 4
    response[idx++] = 0x04;

    IPAddress localIp = WiFi.localIP();
    response[idx++] = localIp[0];
    response[idx++] = localIp[1];
    response[idx++] = localIp[2];
    response[idx++] = localIp[3];

    wifiLlmnrUdp.beginPacket(remoteIp, remotePort);
    wifiLlmnrUdp.write(response, idx);
    wifiLlmnrUdp.endPacket();
}
#endif // ENABLE_WIFI_LLMNR_DISCOVERY

// Web console layer (defined in other modules)
extern void setupWebConsoleServer();
extern void updateWebConsoleServer();
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
extern void setupWifiWebSocket();
extern void updateWifiWebSocket();
#endif

void loadDevModePreference()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        wifiDevModeEnabled = false;
        mus4LogLine("wifi", "dev_mode load failed");
        return;
    }
    wifiDevModeEnabled = mus4Prefs.getBool(MUS4_PREF_DEV_MODE_KEY, false);
    mus4Prefs.end();
    mus4Logf("wifi", "dev_mode=%d", wifiDevModeEnabled ? 1 : 0);
}

bool saveDevModePreference(bool enabled)
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t written = mus4Prefs.putBool(MUS4_PREF_DEV_MODE_KEY, enabled);
    mus4Prefs.end();
    if (written == 0) return false;
    wifiDevModeEnabled = enabled;
    if (wifiDevModeEnabled) {
        keepDevModeOtaWindowActive(otaRuntime, wifiRuntime);
    } else if (wifiOtaWindowOpen && !wifiOtaInProgress) {
        closeWifiOtaWindow("DEV_MODE_OFF", otaRuntime);
    }
    mus4Logf("wifi", "dev_mode saved=%d", wifiDevModeEnabled ? 1 : 0);
    // v1.7.8 起：切换 DEV 不再触发 AP 重启——AP SSID 派生只看 STA 连接状态，
    // 与 DEV 状态无关，避免切换 DEV 时丢一次 AP/Web Console 连接。
    return true;
}

void loadJudgeConfigPreference()
{
    JudgeConfig config = defaultJudgeConfig();
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        wifiRuntime.judgeConfig = config;
        mus4LogLine("wifi", "judge config load failed");
        return;
    }
    JudgeConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    bool loadedFromBlob =
        mus4Prefs.getBytesLength(MUS4_PREF_JUDGE_CFG_BLOB_KEY) == sizeof(blob) &&
        mus4Prefs.getBytes(MUS4_PREF_JUDGE_CFG_BLOB_KEY, &blob, sizeof(blob)) == sizeof(blob) &&
        blob.version == MUS4_CFG_BLOB_VERSION;
    if (loadedFromBlob) {
        config.collisionThreshold = blob.collisionThreshold;
        config.bigTurnThreshold = blob.bigTurnThreshold;
        config.windowSize = blob.windowSize;
        config.collisionPenalty = blob.collisionPenalty;
        config.turnSmoothnessWeight = blob.turnSmoothnessWeight;
        config.rangeMatchWeight = blob.rangeMatchWeight;
        config.gyroStabilityWeight = blob.gyroStabilityWeight;
        config.bigTurnStabilityWeight = blob.bigTurnStabilityWeight;
        config.speedStabilityWeight = blob.speedStabilityWeight;
        config.throttleStabilityWeight = blob.throttleStabilityWeight;
    } else {
        // 旧固件逐键格式回退：老车已调好的参数无损保留
        config.collisionThreshold = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_COLLISION_THRESHOLD_KEY,
            WIFI_JUDGE_COLLISION_THRESHOLD_DEFAULT);
        config.bigTurnThreshold = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_BIG_TURN_THRESHOLD_KEY,
            WIFI_JUDGE_BIG_TURN_THRESHOLD_DEFAULT);
        config.windowSize = mus4Prefs.getUChar(
            MUS4_PREF_JUDGE_WINDOW_SIZE_KEY,
            WIFI_JUDGE_WINDOW_SIZE_DEFAULT);
        config.collisionPenalty = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_COLLISION_PENALTY_KEY,
            WIFI_JUDGE_COLLISION_PENALTY_DEFAULT);
        config.turnSmoothnessWeight = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_TURN_SMOOTHNESS_WEIGHT_KEY,
            WIFI_JUDGE_TURN_SMOOTHNESS_WEIGHT_DEFAULT);
        config.rangeMatchWeight = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_RANGE_MATCH_WEIGHT_KEY,
            WIFI_JUDGE_RANGE_MATCH_WEIGHT_DEFAULT);
        config.gyroStabilityWeight = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_GYRO_STABILITY_WEIGHT_KEY,
            WIFI_JUDGE_GYRO_STABILITY_WEIGHT_DEFAULT);
        config.bigTurnStabilityWeight = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_BIG_TURN_STABILITY_WEIGHT_KEY,
            WIFI_JUDGE_BIG_TURN_STABILITY_WEIGHT_DEFAULT);
        config.speedStabilityWeight = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_SPEED_STABILITY_WEIGHT_KEY,
            WIFI_JUDGE_SPEED_STABILITY_WEIGHT_DEFAULT);
        config.throttleStabilityWeight = mus4Prefs.getFloat(
            MUS4_PREF_JUDGE_THROTTLE_STABILITY_WEIGHT_KEY,
            WIFI_JUDGE_THROTTLE_STABILITY_WEIGHT_DEFAULT);
    }
    // 只有值真的来自旧键才顺手迁移写成 blob；全新设备（无旧键）不在 load 里多写 flash
    bool migrateLegacyKeys = !loadedFromBlob &&
        mus4Prefs.isKey(MUS4_PREF_JUDGE_COLLISION_THRESHOLD_KEY);
    mus4Prefs.end();
    if (!isValidJudgeConfig(config)) {
        config = defaultJudgeConfig();
        migrateLegacyKeys = false;
        mus4LogLine("wifi", "judge config invalid, using defaults");
    }
    wifiRuntime.judgeConfig = config;
    mus4Logf(
        "wifi",
        "judge cfg col=%.2f turn=%.2f win=%u pen=%.2f",
        (double)wifiRuntime.judgeConfig.collisionThreshold,
        (double)wifiRuntime.judgeConfig.bigTurnThreshold,
        wifiRuntime.judgeConfig.windowSize,
        (double)wifiRuntime.judgeConfig.collisionPenalty);
    if (migrateLegacyKeys) {
        // 一次性迁移：写成 blob 后，后续 load 直读 blob、save 也只单次写 blob
        saveJudgeConfigPreference(config);
    }
}

bool saveJudgeConfigPreference(const JudgeConfig& config)
{
    if (!isValidJudgeConfig(config)) return false;
    // 整组参数打包成 blob 一次 putBytes（单次 nvs_commit 写 flash），
    // 避免行车中逐键 put 多次写 flash、阻塞主循环造成控制输出瞬时停顿。
    JudgeConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    blob.version = MUS4_CFG_BLOB_VERSION;
    blob.collisionThreshold = config.collisionThreshold;
    blob.bigTurnThreshold = config.bigTurnThreshold;
    blob.windowSize = config.windowSize;
    blob.collisionPenalty = config.collisionPenalty;
    blob.turnSmoothnessWeight = config.turnSmoothnessWeight;
    blob.rangeMatchWeight = config.rangeMatchWeight;
    blob.gyroStabilityWeight = config.gyroStabilityWeight;
    blob.bigTurnStabilityWeight = config.bigTurnStabilityWeight;
    blob.speedStabilityWeight = config.speedStabilityWeight;
    blob.throttleStabilityWeight = config.throttleStabilityWeight;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t written = mus4Prefs.putBytes(
        MUS4_PREF_JUDGE_CFG_BLOB_KEY, &blob, sizeof(blob));
    mus4Prefs.end();
    if (written != sizeof(blob)) return false;
    wifiRuntime.judgeConfig = config;
    mus4Logf(
        "wifi",
        "judge cfg saved col=%.2f turn=%.2f win=%u pen=%.2f",
        (double)wifiRuntime.judgeConfig.collisionThreshold,
        (double)wifiRuntime.judgeConfig.bigTurnThreshold,
        wifiRuntime.judgeConfig.windowSize,
        (double)wifiRuntime.judgeConfig.collisionPenalty);
    return true;
}

bool resetJudgeConfigPreference()
{
    return saveJudgeConfigPreference(defaultJudgeConfig());
}

void loadDriftConfigPreference()
{
    DriftConfig config = defaultDriftConfig();
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        wifiRuntime.driftConfig = config;
        mus4LogLine("wifi", "drift config load failed");
        return;
    }
    DriftConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    bool loadedFromBlob =
        mus4Prefs.getBytesLength(MUS4_PREF_DRIFT_CFG_BLOB_KEY) == sizeof(blob) &&
        mus4Prefs.getBytes(MUS4_PREF_DRIFT_CFG_BLOB_KEY, &blob, sizeof(blob)) == sizeof(blob) &&
        blob.version == MUS4_CFG_BLOB_VERSION;
    if (loadedFromBlob) {
        config.steeringGyroSign = blob.steeringGyroSign;
        config.maxYawRate = blob.maxYawRate;
        config.kp = blob.kp;
        config.kd = blob.kd;
        config.maxSteeringCorrection = blob.maxSteeringCorrection;
        config.gyroFilterAlpha = blob.gyroFilterAlpha;
        config.spinThreshold = blob.spinThreshold;
        config.steeringThreshold = blob.steeringThreshold;
        config.continuousThrottle = blob.continuousThrottle;
        config.pulseThrottle = blob.pulseThrottle;
        config.pulseFreqHz = blob.pulseFreqHz;
        config.pulseDuty = blob.pulseDuty;
    } else {
        // 旧固件逐键格式回退：老车已调好的参数无损保留
        config.steeringGyroSign = (int8_t)mus4Prefs.getInt(
            MUS4_PREF_DRIFT_STEERING_GYRO_SIGN_KEY,
            WIFI_DRIFT_STEERING_GYRO_SIGN_DEFAULT);
        config.maxYawRate = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_MAX_YAW_RATE_KEY,
            WIFI_DRIFT_MAX_YAW_RATE_DEFAULT);
        config.kp = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_KP_KEY,
            WIFI_DRIFT_KP_DEFAULT);
        config.kd = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_KD_KEY,
            WIFI_DRIFT_KD_DEFAULT);
        config.maxSteeringCorrection = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_MAX_STEERING_CORRECTION_KEY,
            WIFI_DRIFT_MAX_STEERING_CORRECTION_DEFAULT);
        config.gyroFilterAlpha = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_GYRO_FILTER_ALPHA_KEY,
            WIFI_DRIFT_GYRO_FILTER_ALPHA_DEFAULT);
        config.spinThreshold = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_SPIN_THRESHOLD_KEY,
            WIFI_DRIFT_SPIN_THRESHOLD_DEFAULT);
        config.steeringThreshold = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_STEERING_THRESHOLD_KEY,
            WIFI_DRIFT_STEERING_THRESHOLD_DEFAULT);
        config.continuousThrottle = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_CONTINUOUS_THROTTLE_KEY,
            WIFI_DRIFT_CONTINUOUS_THROTTLE_DEFAULT);
        config.pulseThrottle = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_PULSE_THROTTLE_KEY,
            WIFI_DRIFT_PULSE_THROTTLE_DEFAULT);
        config.pulseFreqHz = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_PULSE_FREQ_HZ_KEY,
            WIFI_DRIFT_PULSE_FREQ_HZ_DEFAULT);
        config.pulseDuty = mus4Prefs.getFloat(
            MUS4_PREF_DRIFT_PULSE_DUTY_KEY,
            WIFI_DRIFT_PULSE_DUTY_DEFAULT);
    }
    // 只有值真的来自旧键才顺手迁移写成 blob；全新设备（无旧键）不在 load 里多写 flash
    bool migrateLegacyKeys = !loadedFromBlob &&
        mus4Prefs.isKey(MUS4_PREF_DRIFT_STEERING_GYRO_SIGN_KEY);
    mus4Prefs.end();
    if (!isValidDriftConfig(config)) {
        config = defaultDriftConfig();
        migrateLegacyKeys = false;
        mus4LogLine("wifi", "drift config invalid, using defaults");
    }
    wifiRuntime.driftConfig = config;
    mus4Logf(
        "wifi",
        "drift cfg sign=%d yaw=%.2f kp=%.3f kd=%.3f maxCorr=%.2f alpha=%.2f spin=%.2f strTh=%.2f cont=%.2f pulse=%.2f freq=%.2f duty=%.2f",
        (int)wifiRuntime.driftConfig.steeringGyroSign,
        (double)wifiRuntime.driftConfig.maxYawRate,
        (double)wifiRuntime.driftConfig.kp,
        (double)wifiRuntime.driftConfig.kd,
        (double)wifiRuntime.driftConfig.maxSteeringCorrection,
        (double)wifiRuntime.driftConfig.gyroFilterAlpha,
        (double)wifiRuntime.driftConfig.spinThreshold,
        (double)wifiRuntime.driftConfig.steeringThreshold,
        (double)wifiRuntime.driftConfig.continuousThrottle,
        (double)wifiRuntime.driftConfig.pulseThrottle,
        (double)wifiRuntime.driftConfig.pulseFreqHz,
        (double)wifiRuntime.driftConfig.pulseDuty);
    if (migrateLegacyKeys) {
        // 一次性迁移：写成 blob 后，后续 load 直读 blob、save 也只单次写 blob
        saveDriftConfigPreference(config);
    }
}

bool saveDriftConfigPreference(const DriftConfig& config)
{
    if (!isValidDriftConfig(config)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) {
        mus4LogLine("wifi", "drift config save failed: prefs begin");
        return false;
    }
    // 整组参数打包成 blob 一次 putBytes（单次 nvs_commit 写 flash），
    // 避免行车中逐键 put 多次写 flash、阻塞主循环造成控制输出瞬时停顿。
    DriftConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    blob.version = MUS4_CFG_BLOB_VERSION;
    blob.steeringGyroSign = config.steeringGyroSign;
    blob.maxYawRate = config.maxYawRate;
    blob.kp = config.kp;
    blob.kd = config.kd;
    blob.maxSteeringCorrection = config.maxSteeringCorrection;
    blob.gyroFilterAlpha = config.gyroFilterAlpha;
    blob.spinThreshold = config.spinThreshold;
    blob.steeringThreshold = config.steeringThreshold;
    blob.continuousThrottle = config.continuousThrottle;
    blob.pulseThrottle = config.pulseThrottle;
    blob.pulseFreqHz = config.pulseFreqHz;
    blob.pulseDuty = config.pulseDuty;
    size_t written = mus4Prefs.putBytes(
        MUS4_PREF_DRIFT_CFG_BLOB_KEY, &blob, sizeof(blob));
    mus4Prefs.end();
    if (written != sizeof(blob)) {
        mus4Logf("wifi", "drift config save failed: blob written=%u", (unsigned)written);
        return false;
    }
    wifiRuntime.driftConfig = config;
    mus4Logf(
        "wifi",
        "drift cfg saved sign=%d yaw=%.2f kp=%.3f kd=%.3f maxCorr=%.2f alpha=%.2f spin=%.2f strTh=%.2f cont=%.2f pulse=%.2f freq=%.2f duty=%.2f",
        (int)wifiRuntime.driftConfig.steeringGyroSign,
        (double)wifiRuntime.driftConfig.maxYawRate,
        (double)wifiRuntime.driftConfig.kp,
        (double)wifiRuntime.driftConfig.kd,
        (double)wifiRuntime.driftConfig.maxSteeringCorrection,
        (double)wifiRuntime.driftConfig.gyroFilterAlpha,
        (double)wifiRuntime.driftConfig.spinThreshold,
        (double)wifiRuntime.driftConfig.steeringThreshold,
        (double)wifiRuntime.driftConfig.continuousThrottle,
        (double)wifiRuntime.driftConfig.pulseThrottle,
        (double)wifiRuntime.driftConfig.pulseFreqHz,
        (double)wifiRuntime.driftConfig.pulseDuty);
    return true;
}

bool resetDriftConfigPreference()
{
    return saveDriftConfigPreference(defaultDriftConfig());
}

void startWifiMdnsIfNeeded()
{
#ifdef DISABLE_WIFI_NAME_DISCOVERY
    return;
#endif
    if (wifiMdnsStarted) return;
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;
    if (!MDNS.begin(wifiMdnsHostText().c_str())) {
        mus4LogLine("wifi", "mDNS start failed");
        return;
    }
    MDNS.addService("http", "tcp", WIFI_WEB_CONSOLE_PORT);
    wifiMdnsStarted = true;
    mus4Logf("wifi", "mDNS started: %s.local", wifiMdnsHostText().c_str());
}

bool ensureWifiApAvailable();
bool restartWifiAp();
bool startWifiApServices(const char* logPrefix, uint8_t channel = WIFI_CONSOLE_CHANNEL);

void stopWifiMdnsIfNeeded()
{
    if (!wifiMdnsStarted) return;
    MDNS.end();
    wifiMdnsStarted = false;
    mus4LogLine("wifi", "mDNS stopped");
}

#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
void startWifiNetbiosIfNeeded()
{
    if (wifiNetbiosStarted) return;
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;
    if (!NBNS.begin(wifiMdnsHostText().c_str())) {
        mus4LogLine("wifi", "NetBIOS start failed");
        return;
    }
    wifiNetbiosStarted = true;
    mus4Logf("wifi", "NetBIOS started: %s", wifiMdnsHostText().c_str());
}

void stopWifiNetbiosIfNeeded()
{
    if (!wifiNetbiosStarted) return;
    NBNS.end();
    wifiNetbiosStarted = false;
    mus4LogLine("wifi", "NetBIOS stopped");
}
#endif

#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
void startWifiLlmnrIfNeeded()
{
    if (wifiLlmnrStarted) return;
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return;
    if (!wifiLlmnrUdp.begin(LLMNR_PORT)) {
        mus4LogLine("wifi", "LLMNR start failed");
        return;
    }
    wifiLlmnrUdp.beginMulticast(LLMNR_MULTICAST_IP, LLMNR_PORT);
    wifiLlmnrStarted = true;
    mus4Logf("wifi", "LLMNR started: %s", wifiMdnsHostText().c_str());
}

void stopWifiLlmnrIfNeeded()
{
    if (!wifiLlmnrStarted) return;
    wifiLlmnrUdp.stop();
    wifiLlmnrStarted = false;
    mus4LogLine("wifi", "LLMNR stopped");
}
#endif

// v1.7.18 起 AP/STA 互斥切换：STA→STA 切换由「保存新配置 → applyWifiStaCredentials()
// 直接接管」处理，handoff 三态共存逻辑退役。保留下面三个接口和 staHandoff*
// 字段是为了让 WebConsoleServer.cpp 与 wifiStaJson() 的调用面继续编译通过；
// handoff_active 永远停留在 false，前端 wifiStaHandoffModal 不会再被触发。
void clearWifiStaHandoff()
{
    wifiStaHandoffActive = false;
    wifiStaHandoffTargetSsid[0] = 0;
    wifiStaHandoffStaIp[0] = 0;
    wifiStaHandoffApSsid[0] = 0;
    wifiStaHandoffStartedMs = 0;
}

void finishWifiStaHandoff()
{
    clearWifiStaHandoff();
}

void startWifiStaHandoff(const String& /*targetSsid*/)
{
    clearWifiStaHandoff();
}

void disconnectWifiStaOnly()
{
    esp_wifi_disconnect();
}

static bool isValidWifiChannel(uint8_t channel)
{
    return channel >= 1 && channel <= 14;
}

// 配网前把 SoftAP 预先重启到目标 STA 信道：ESP32 单射频 AP+STA 必须同信道，
// 提前对齐可避免 STA 连接成功瞬间 SoftAP 被动切信道踢掉 AP 客户端。
static bool restartWifiApOnChannel(uint8_t channel)
{
    if (!isValidWifiChannel(channel)) return false;
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) return false;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(false);
    delay(100);
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
    return startWifiApServices("AP channel prealigned for STA", channel);
}

static void prealignWifiApChannelForStaApply()
{
    if (!wifiStaApplyFromAp) return;
    uint8_t channel = wifiStaTargetChannel;
    if (!isValidWifiChannel(channel)) return;
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) return;
    if (restartWifiApOnChannel(channel)) {
        mus4Logf("wifi", "STA apply: AP prealigned to channel %u", channel);
    } else {
        mus4LogLine("wifi", "STA apply: AP channel prealign skipped");
    }
}

void applyWifiStaCredentials()
{
    if (!wifiStaConfigured) return;
    stopWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
    stopWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
    stopWifiLlmnrIfNeeded();
#endif
    wifiStaApplyPending = false;
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = true;
    // v1.7.18 起 AP/STA 互斥切换：发起新一轮 STA 连接前清空两个 grace 锚点，避免上一轮残留触发误关 AP。
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaDownGraceDeadlineMs = 0;
    clearWifiStaLastError();
    wifiStaConnectStartMs = millis();
    // v1.7.21 起：开机模式即为 WIFI_AP_STA，常态下不会触发 mode 切换。
    // 仅当当前在 STA-only（落地后用户在 web 端发起重新连接）时才需要切回 AP_STA；
    // 同时确保 AP 服务在线，给 STA 失败提供兜底入口。
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        mus4LogLine("wifi", "STA apply: switching to AP_STA");
        // ESP-IDF 切到 AP_STA 后 STA netif 需要一小段时间初始化；不加这个延时
        // 紧接着的 STA begin 可能拿不到信道，会导致 timeout。
        delay(50);
    }
    if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
        startWifiApServices("AP restored for STA apply");
    }
    // 开机自动连接时，信道对齐已在 setupWifiConsole() 中完成（AP 启动前扫描
    // 并将 AP 置于目标信道），此处无需再扫描。Web 端 AP 发起时使用前端传入的信道。
    prealignWifiApChannelForStaApply();
    wifiInApOnlyMode = false;
    // 不再显式调用 disconnectWifiStaOnly()——WiFi.begin 内部已调用
    // esp_wifi_disconnect()，重复调用可能导致竞态使 STA 连接静默失败。
    // 延时确保 Wi-Fi 栈处理完 prealignWifiApChannelForStaApply 可能的 AP 重启事件。
    delay(100);
    WiFi.setHostname(wifiMdnsHostText().c_str());
    WiFi.begin(wifiStaSsid, wifiStaPassword);
    mus4Logf("wifi", "STA connecting: ssid=\"%s\" pass_len=%d", wifiStaSsid, (int)strlen(wifiStaPassword));
}

void scheduleWifiApRestart()
{
    wifiApRestartPending = true;
    wifiApRestartDeadlineMs = millis() + WIFI_STA_APPLY_DELAY_MS;
}

String getActiveWifiApSsid()
{
    // STA 连上后的 10 秒展示窗口里返回 "MUS4-<sta_ip>"，让用户在 Wi-Fi 列表读到 IP；
    // 其余时间返回基础 AP SSID。窗口结束/失败恢复时 g_staIpApSsidOverride 被清空。
    if (g_staIpApSsidOverride.length() > 0) {
        return g_staIpApSsidOverride;
    }
    return String(wifiApSsid);
}

bool configureWifiSoftApNetwork()
{
    IPAddress apIp(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    return WiFi.softAPConfig(apIp, apIp, subnet);
}

bool startWifiConsoleServices(const char* logPrefix)
{
    // 先关闭再重新打开监听套接字，确保接口变化（如 STA 断开、AP 重启）后
    // 服务端能绑定到当前活动的网络接口，避免 AP 恢复后 HTTP/TCP 无法访问。
    mus4LogLine("wifi", "console services: close old sockets");
    wifiWebServer.close();
    wifiConsoleServer.stop();
    mus4LogLine("wifi", "console services: DNS start");
    wifiCaptiveDnsServer.start(53, "*", WiFi.softAPIP());
    mus4LogLine("wifi", "console services: TCP begin");
    wifiConsoleServer.begin();
    wifiConsoleServer.setNoDelay(true);
    mus4LogLine("wifi", "console services: HTTP begin");
    wifiWebServer.begin();
    wifiConsoleStarted = true;
    mus4Logf("wifi", "%s ssid=%s IP: %s", logPrefix, getActiveWifiApSsid().c_str(), WiFi.softAPIP().toString().c_str());
    return true;
}

bool startWifiApServices(const char* logPrefix, uint8_t channel)
{
    mus4LogLine("wifi", "AP services: config network");
    configureWifiSoftApNetwork();
    mus4LogLine("wifi", "AP services: softAP begin");
    String activeSsid = getActiveWifiApSsid();
    uint8_t apChannel = (channel >= 1 && channel <= 14) ? channel : WIFI_CONSOLE_CHANNEL;
    bool started = WiFi.softAP(
        activeSsid.c_str(),
        WIFI_CONSOLE_AP_PASSWORD,
        apChannel,
        false,
        WIFI_CONSOLE_MAX_CLIENTS
    );
    mus4LogLine("wifi", "AP services: softAP done");
    if (!started) {
        wifiConsoleStarted = false;
        mus4Logf("wifi", "%s failed", logPrefix);
        return false;
    }
    bool ok = startWifiConsoleServices(logPrefix);
    if (ok) {
        buzzer.playWifiApStartSound();
    }
    return ok;
}

bool ensureWifiApAvailable()
{
    wifiApRestartPending = false;
    // AP 接口可能仍在（WiFi.softAPIP() != 0），但 STA 扫描/连接阶段会把 SoftAP
    // 信道带跑；直接复用旧接口只重启 console 服务无法让 AP 回到正确信道，
    // 客户端重连后会出现「ping 通但 HTTP 无响应」。
    // 这里用 softAPdisconnect(false) 清掉当前 AP 配置并重启一次 AP 接口
    //（不切换底层 WIFI_AP_STA mode），再由 startWifiApServices() 重新下发
    // 正确配置、启动完整服务。
    if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
        mus4LogLine("wifi", "AP interface up, soft reset before reconfigure");
        wifiCaptiveDnsServer.stop();
        WiFi.softAPdisconnect(false);
        delay(100);
    }
    return startWifiApServices("AP ensured");
}

bool restartWifiAp()
{
    wifiApRestartPending = false;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(true);
    delay(100);
    // 保持 WIFI_AP_STA 不变，避免 AP-only ↔ AP_STA 反复切换重置 SoftAP、踢掉客户端。
    // STA 是否在线由 wifiStaConnected / wifiInApOnlyMode 语义管理，不依赖 mode。
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
    wifiInApOnlyMode = !wifiStaConnected;
    return startWifiApServices("AP restarted");
}

// STA 稳定 grace 通过后，主动把 SoftAP 关掉并切到纯 STA 模式。
// 实测在 ESP32 Arduino core 上，若仅调用 softAPdisconnect(true) 而保持底层
// WIFI_AP_STA，AP 接口常被底层以默认 SSID（如 ESP_48F54D）重新拉起，
// 192.168.4.1 继续可用。切换到 WIFI_STA 才能彻底关闭 SoftAP。
// Captive DNS 与 wifiConsoleServer 都依赖 SoftAP，AP 关闭时一并停掉，避免
// 出现「socket 还监听但 AP 已下线」的悬空状态。
static void stopWifiApForStaOnly()
{
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaApplyFromAp = false;
    wifiStaTargetChannel = 0;
    g_staIpApSsidOverride = "";
    wifiApRestartPending = false;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(true);
    delay(100);
    // 必须切到 WIFI_STA；仅保持 AP_STA 会导致默认 AP 残留。
    if (WiFi.getMode() != WIFI_STA) {
        WiFi.mode(WIFI_STA);
        delay(50);
    }
    // 切到 STA-only 后重新绑定 WebServer，让监听套接字只绑定 STA 接口。
    // 同时避免在 AP 仍在线的 grace 窗口内 close/begin，导致配置页面中断。
    wifiWebServer.close();
    wifiWebServer.begin();
    mus4LogLine("wifi", "WebServer re-bound for STA");
    // 关 AP 后 SoftAP 端的 TCP 控制台监听句柄已失效；这里把 wifiConsoleStarted
    // 置 false 让 updateWifiConsole() 不再尝试 accept，但 wifiWebServer 仍可在
    // STA 接口上响应。
    wifiConsoleStarted = false;
    wifiInApOnlyMode = false;
    mus4LogLine("wifi", "AP stopped after STA connected");
    buzzer.playWifiApStopSound();
}

// STA 持续断开 / STA 失败 / WIFI_STA_CLEAR 路径都收敛到这里：停止 STA 并
// 确保 AP 兜底可用。保持底层 mode 为 WIFI_AP_STA，不再切到 WIFI_AP，避免
// 下一次从 AP-only 保存 STA 时做破坏性的 AP↔AP_STA 切换、踢掉配置客户端。
// v1.7.20 起本函数不再写 lastError，由调用方按场景决定错误码；这样四条失败路径
// （down grace 后的 sta_lost / timeout / auth_failed / no_ssid）能各自保留语义。
static void restoreApAfterStaLost()
{
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaDownGraceDeadlineMs = 0;
    wifiStaApplyFromAp = false;
    wifiStaTargetChannel = 0;
    g_staIpApSsidOverride = "";
    buzzer.playWifiStaDisconnectedSound();
    wifiStaConnected = false;
    wifiStaConnecting = false;
    stopWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
    stopWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
    stopWifiLlmnrIfNeeded();
#endif
    // esp_wifi_disconnect() 把 ESP32 自带的 STA 自动重连机制停掉，避免设备在
    // AP-only 下后台不断重试错误密码、干扰 RF 调度。
    esp_wifi_disconnect();
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
    wifiInApOnlyMode = true;
    ensureWifiApAvailable();
}

bool clearWifiStaAndRestoreAp()
{
    if (!clearWifiStaPreference()) {
        mus4LogLine("wifi", "BOOT long press: STA clear failed");
        return false;
    }
    restoreApAfterStaLost();
    mus4LogLine("wifi", "STA cleared by BOOT long press");
    return true;
}

void updateWifiBootResetButton()
{
    bool pressed = digitalRead(WIFI_BOOT_RESET_PIN) == LOW;
    unsigned long now = millis();
    if (!pressed) {
        bootWifiResetPressedAtMs = 0;
        bootWifiResetTriggered = false;
        return;
    }
    if (bootWifiResetPressedAtMs == 0) {
        bootWifiResetPressedAtMs = now;
        return;
    }
    if (wifiOtaInProgress) return;
    if (!bootWifiResetTriggered && now - bootWifiResetPressedAtMs >= WIFI_BOOT_RESET_HOLD_MS) {
        bootWifiResetTriggered = true;
        clearWifiStaAndRestoreAp();
    }
}

void loadWifiApPreference()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) {
        copyWifiApSsid(String(WIFI_CONSOLE_AP_DEFAULT_SSID));
        mus4LogLine("wifi", "AP SSID load failed, using default");
        return;
    }
    String ssid = mus4Prefs.getString(MUS4_PREF_AP_SSID_KEY, WIFI_CONSOLE_AP_DEFAULT_SSID);
    mus4Prefs.end();
    ssid.trim();
    if (!copyWifiApSsid(ssid)) {
        copyWifiApSsid(String(WIFI_CONSOLE_AP_DEFAULT_SSID));
        mus4LogLine("wifi", "AP SSID invalid, using default");
    }
}

bool saveWifiApPreference(const String& ssid)
{
    String trimmed = ssid;
    trimmed.trim();
    if (!copyWifiApSsid(trimmed)) return false;
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    size_t ssidWritten = mus4Prefs.putString(MUS4_PREF_AP_SSID_KEY, wifiApSsid);
    mus4Prefs.end();
    return ssidWritten > 0;
}

void setupWifiWebConsole()
{
    setupWebConsoleServer();
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
    setupWifiWebSocket();
#endif
}

void updateWifiWebConsole()
{
    updateWebConsoleServer();
    if (wifiApRestartPending && (long)(millis() - wifiApRestartDeadlineMs) >= 0) {
        restartWifiAp();
    }
#ifdef ENABLE_WIFI_WEBSOCKET_TELEMETRY
    updateWifiWebSocket();
#endif
}

void setupWifiConsole()
{
    mus4LogLine("wifi", "setup: entered");
    webLogBufferInit();
    mus4SetWebLogSink(appendWebLog);
    lastWifiConsoleStartAttemptMs = millis();
    wifiStaConnected = false;
    wifiStaTimedOut = false;
    wifiStaConnecting = false;
    wifiApRestartPending = false;
    // 底层 mode 始终维持 WIFI_AP_STA；AP/STA 的「互斥」通过启用/停用 SoftAP
    // 以及 STA 连接状态来管理，避免 AP-only ↔ AP_STA 反复切换重置接口、踢掉客户端。
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaDownGraceDeadlineMs = 0;
    // wifiInApOnlyMode 表示「当前是否按 AP-only 行为运行」（STA 不活跃），
    // 不再与底层 WiFi mode 严格一一对应。
    wifiInApOnlyMode = false;
    clearWifiStaLastError();
    // 不再调用带 eraseap 的 WiFi.disconnect ——其 eraseap 参数会擦除 Wi-Fi 驱动
    // 层 NVS 配置，但 WiFi.begin() / WiFi.softAP() 都显式传入凭据，此擦除多余
    // 且可能引起 Wi-Fi 栈内部状态异常。
    mus4LogLine("wifi", "setup: mode OFF");
    WiFi.mode(WIFI_OFF);
    delay(100);
    // 全程保持 WIFI_AP_STA。SoftAP 负责 AP 兜底入口，STA 部分在配置存在时才
    // begin；AP 关闭（stopWifiApForStaOnly）或 STA 失败（restoreApAfterStaLost）
    // 时不再切 mode，只启停对应接口。这样彻底避免 AP↔AP_STA 反复切换导致的
    // SoftAP 重置、配置页面断连和 STA netif race。
    // 历史 v1.7.17 全程 WIFI_AP_STA 已验证 newhome_iot 等路由器可正常连接。
    mus4LogLine("wifi", "setup: mode AP_STA");
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    {
        String host = wifiMdnsHostText();
        if (host.length() > 0) {
            WiFi.setHostname(host.c_str());
        }
    }
    mus4LogLine("wifi", "setup: web console");
    setupWifiWebConsole();

    // 开机自动连接时，在启动 SoftAP 之前先扫描目标 SSID 所在信道。
    // 此时 WIFI_AP_STA 已初始化但 SoftAP 尚未运行，扫描不会干扰 AP Beacon，
    // 且扫描后到 WiFi.begin() 之间有 AP 启动时间作为充足的 STA 接口恢复延时。
    uint8_t apChannel = WIFI_CONSOLE_CHANNEL;
    if (wifiStaConfigured && strlen(wifiStaSsid) > 0) {
        int n = WiFi.scanNetworks(false, false);  // 阻塞同步扫描
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                if (WiFi.SSID(i) == String(wifiStaSsid)) {
                    apChannel = (uint8_t)WiFi.channel(i);
                    mus4Logf("wifi", "STA boot: found %s on channel %u",
                             wifiStaSsid, apChannel);
                    break;
                }
            }
            WiFi.scanDelete();
        }
        if (apChannel == WIFI_CONSOLE_CHANNEL) {
            mus4LogLine("wifi", "STA boot: SSID not found in scan, using default channel");
        }
    }

    // STA 连接历史开机回退：已配置 SSID 不可见（或 STA 未配置）而历史条目可见时，
    // 按历史优先级（槽 0 最近）挑最佳可见条目，把凭据写入运行时状态接管本次开机
    // 连接；仅改运行时，不触碰 NVS 中的 sta_ssid/sta_pass 配置。首试槽位记入
    // staHistTriedMask，供 updateWifiStaHistoryRetry() 在断线重试时跳过。
    if (wifiStaHistoryCount() > 0) {
        // 上面的开机扫描已命中已配置 SSID（信道被记录且非默认值）时无需回退；
        // 其余情况（未配置 / 未扫到 / 恰好在默认信道）补一次阻塞扫描判定可见性。
        bool staBootConfiguredVisible = wifiStaConfigured && strlen(wifiStaSsid) > 0 &&
            apChannel != WIFI_CONSOLE_CHANNEL;
        if (!staBootConfiguredVisible) {
            int n = WiFi.scanNetworks(false, false);  // 阻塞同步扫描（历史回退判定）
            if (n > 0) {
                if (wifiStaConfigured && strlen(wifiStaSsid) > 0) {
                    for (int i = 0; i < n; i++) {
                        if (WiFi.SSID(i) == String(wifiStaSsid)) {
                            staBootConfiguredVisible = true;
                            apChannel = (uint8_t)WiFi.channel(i);
                            mus4Logf("wifi", "STA boot: found %s on channel %u (history scan)",
                                     wifiStaSsid, apChannel);
                            break;
                        }
                    }
                }
                if (!staBootConfiguredVisible) {
                    bool staHistBootPicked = false;
                    for (uint8_t rank = 0; rank < wifiStaHistoryCount() && !staHistBootPicked; rank++) {
                        if ((wifiRuntime.staHistTriedMask & (uint8_t)(1u << rank)) != 0) continue;
                        String histSsid;
                        if (!copyWifiStaHistorySsid(rank, histSsid)) continue;
                        for (int i = 0; i < n; i++) {
                            if (WiFi.SSID(i) != histSsid) continue;
                            String histPassword;
                            if (findWifiStaHistoryEntry(histSsid, histPassword) &&
                                copyWifiStaSsid(histSsid) && copyWifiStaPassword(histPassword)) {
                                wifiStaConfigured = true;
                                apChannel = (uint8_t)WiFi.channel(i);
                                wifiRuntime.staHistTriedMask |= (uint8_t)(1u << rank);
                                staHistBootPicked = true;
                                mus4Logf("wifi", "STA boot: history slot %u ssid=\"%s\" on channel %u",
                                         rank, histSsid.c_str(), apChannel);
                            }
                            break;
                        }
                    }
                }
                WiFi.scanDelete();
            }
        }
        if (staBootConfiguredVisible) {
            // 已配置 SSID 可见：保持原有连接目标，仅把它的历史槽位标记为已首试。
            int8_t rank = wifiStaHistoryRankOf(String(wifiStaSsid));
            if (rank >= 0) {
                wifiRuntime.staHistTriedMask |= (uint8_t)(1u << rank);
            }
        }
    }

    mus4LogLine("wifi", "setup: start AP services");
    if (!startWifiApServices("AP started", apChannel)) {
        return;
    }
    mus4Logf("wifi", "AP %s IP: %s Port: %u Web: %u ch=%u", getActiveWifiApSsid().c_str(), WiFi.softAPIP().toString().c_str(), WIFI_CONSOLE_PORT, WIFI_WEB_CONSOLE_PORT, apChannel);
    if (wifiStaConfigured) {
        mus4LogLine("wifi", "setup: apply STA credentials");
        applyWifiStaCredentials();
    }
}

// STA 连上拿到有效 IP 后，把 SoftAP 名临时改成 "MUS4-<sta_ip>"，在 STA 信道
// 广播；用户在 Wi-Fi 列表直接读出设备 IP。随后由 10 秒窗口触发关闭 AP。
static void showStaIpInApName()
{
    String ip = WiFi.localIP().toString();
    g_staIpApSsidOverride = String(WIFI_STA_IP_AP_PREFIX) + ip;
    if (g_staIpApSsidOverride.length() > WIFI_AP_SSID_MAX_LEN) {
        g_staIpApSsidOverride = g_staIpApSsidOverride.substring(0, WIFI_AP_SSID_MAX_LEN);
    }
    uint8_t ch = (uint8_t)WiFi.channel();
    if (ch < 1 || ch > 14) ch = WIFI_CONSOLE_CHANNEL;
    wifiCaptiveDnsServer.stop();
    WiFi.softAPdisconnect(false);
    delay(100);
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
    startWifiApServices("AP shows STA IP", ch);
    mus4Logf("wifi", "AP name shows STA IP: %s", g_staIpApSsidOverride.c_str());
}

void updateWifiSta()
{
    if (wifiStaApplyPending && (long)(millis() - wifiStaApplyDeadlineMs) >= 0) {
        applyWifiStaCredentials();
    }
    // v1.7.18 起 AP/STA 互斥切换：未配置 STA 时停留在 AP-only；但若刚被
    // WIFI_STA_CLEAR 清空，而设备此时还在 STA_ONLY，需要立即恢复 AP。
    if (!wifiStaConfigured) {
        if (!wifiInApOnlyMode) {
            clearWifiStaLastError();
            restoreApAfterStaLost();
            mus4LogLine("wifi", "AP restored after STA cleared");
        }
        return;
    }

    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
        if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
            // ESP-IDF 偶尔会出现 Wi-Fi 事件先报告 WL_CONNECTED、netif IP 尚未写入的
            // 短暂窗口；此时若标记 connected，/api/wifi-sta 会把 sta_ip 填成 0.0.0.0，
            // Web Console 就没办法在 AP 关闭前把局域网访问地址展示给用户。等待下一轮。
            return;
        }
        if (!wifiStaConnected) {
            // 首次进入 WL_CONNECTED：标记 STA 在线，启动 mDNS，并武装 up grace。
            // 关 AP 的动作要等 WIFI_STA_GRACE_UP_MS 后由本函数下一轮触发，
            // 避免连接刚握手就被 softAP 抖动干扰。
            wifiStaConnected = true;
            wifiStaTimedOut = false;
            wifiStaConnecting = false;
            clearWifiStaLastError();
            buzzer.playWifiStaConnectedSound();
            startWifiMdnsIfNeeded();
#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
            startWifiNetbiosIfNeeded();
#endif
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
            startWifiLlmnrIfNeeded();
#endif
            mus4Logf("wifi", "STA connected IP: %s", WiFi.localIP().toString().c_str());
            // 把 STA IP 编码进 AP 名广播；不区分首次配网还是重启自动重连，
            // 统一展示 WIFI_STA_IP_DISPLAY_MS（60 秒）后由本函数下一轮关闭 AP。
            showStaIpInApName();
            wifiStaUpGraceDeadlineMs = millis() + WIFI_STA_IP_DISPLAY_MS;
            wifiStaDownGraceDeadlineMs = 0;
            // handoff 在新方案里已经退役；保留 finish 调用是为了把残留字段清空。
            finishWifiStaHandoff();
            // 连接成功：把当前凭据记入 STA 连接历史（去重/置顶在函数内部处理）。
            recordWifiStaHistory(wifiStaSsid, wifiStaPassword);
        } else if (wifiStaDownGraceDeadlineMs != 0) {
            // STA 抖动后又恢复（grace 窗口内），取消 down grace，保持 STA_ONLY。
            wifiStaDownGraceDeadlineMs = 0;
            mus4LogLine("wifi", "STA recovered within grace window");
        }
        if (wifiStaUpGraceDeadlineMs != 0 && (long)(millis() - wifiStaUpGraceDeadlineMs) >= 0) {
            stopWifiApForStaOnly();
        }
        return;
    }

    if (wifiStaConnected) {
        // 进入 down grace：先不关闭 mDNS、也不切模式；只武装截止时间，等下一轮判定。
        if (wifiStaDownGraceDeadlineMs == 0) {
            wifiStaDownGraceDeadlineMs = millis() + WIFI_STA_GRACE_DOWN_MS;
            mus4LogLine("wifi", "STA link lost, arming down grace");
        }
        if ((long)(millis() - wifiStaDownGraceDeadlineMs) >= 0) {
            setWifiStaLastError("sta_lost", "STA 连接已断开，已切回 AP 模式。", true);
            restoreApAfterStaLost();
            mus4LogLine("wifi", "AP restored after STA lost");
        }
        return;
    }

    if (!wifiStaConnecting) return;
    // v1.7.20 起：三种失败路径（no_ssid / auth_failed / timeout）都先写 lastError，
    // 再走 restoreApAfterStaLost() 切回 AP-only。否则设备会卡在 AP_STA + ESP32
    // 后台 auto-reconnect 错密码的死循环里，AP 也不会重启。
    if (status == WL_NO_SSID_AVAIL) {
        setWifiStaLastError("no_ssid", "未找到目标 SSID，请检查网络名称或距离。", false);
        restoreApAfterStaLost();
        mus4LogLine("wifi", "AP restored after STA no_ssid");
        return;
    }
    if (status == WL_CONNECT_FAILED) {
        setWifiStaLastError("auth_failed", "STA 认证失败，请检查 Wi-Fi 密码。", false);
        restoreApAfterStaLost();
        mus4LogLine("wifi", "AP restored after STA auth_failed");
        return;
    }
    if (!wifiStaTimedOut && millis() - wifiStaConnectStartMs >= WIFI_STA_CONNECT_TIMEOUT_MS) {
        setWifiStaLastError("timeout", "STA 连接超时，请检查 SSID、密码与路由器信号。", true);
        restoreApAfterStaLost();
        mus4LogLine("wifi", "AP restored after STA timeout");
    }
}

// STA 连接历史运行期重试状态机：
// - 上升沿（staConnected false→true）：记录/置顶历史，清空已试掩码与周期标志；
// - 连接失败自愈：lastError 非空（auth_failed/timeout 等）且历史中同 SSID 存有
//   不同密码时，解锁该槽位的已试标记，让重试状态机用历史（最近成功）密码再试
//   一次，有界不死循环；
// - 重试窗口（!connected && !connecting && !applyPending && (configured || lastError 非空)，
//   即开机首试失败或 sta_lost 断线落地之后的状态）：还有未试槽位时启动异步扫描，
//   按历史优先级挑未试过的可见条目，copy 凭据、标记已试后发起新一轮连接；
// - 可见候选全部试完（或扫描失败）：本轮周期结束，停在 AP-only（与现状一致）；
//   下次 connected 边沿清掩码后，新断线自然重新武装。

// NVS 凭据自愈：连上的网络与 NVS sta_ssid 相同、但 NVS sta_pass 与本次成功密码
// 不一致时（keep_password 只写 sta_ssid、路由器改密码后只更新了历史等场景），
// 把本次验证成功的密码同步回 sta_pass，修复 NVS 凭据对；否则下次开机仍用旧
// （错误）密码先试失败一轮。连上的 SSID 与 NVS 配置不同（回退到其它网络）时不
// 触碰 NVS，沿用 v1.7.35「回退仅改运行时」的设计边界。
static void healWifiStaPreferenceAfterConnect()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) return;
    bool staEnabled = mus4Prefs.getBool(MUS4_PREF_STA_ENABLED_KEY, false);
    String nvsSsid = mus4Prefs.getString(MUS4_PREF_STA_SSID_KEY, "");
    String nvsPass = mus4Prefs.getString(MUS4_PREF_STA_PASSWORD_KEY, "");
    mus4Prefs.end();
    if (!staEnabled || nvsSsid.length() == 0) return;
    if (nvsSsid != String(wifiStaSsid)) return;
    if (nvsPass == String(wifiStaPassword)) return;
    if (saveWifiStaPreference(String(wifiStaSsid), String(wifiStaPassword))) {
        mus4LogLine("wifi", "STA pref healed: sta_pass synced with working password");
    }
}

void updateWifiStaHistoryRetry()
{
    static bool lastStaConnected = false;
    static bool staHistScanPending = false;
    bool connected = wifiStaConnected;
    if (connected && !lastStaConnected) {
        recordWifiStaHistory(wifiStaSsid, wifiStaPassword);
        healWifiStaPreferenceAfterConnect();
        wifiRuntime.staHistTriedMask = 0;
        wifiRuntime.staHistRetryActive = false;
        wifiRuntime.staHistRetryDeadlineMs = 0;
        wifiRuntime.staHistRescanDeadlineMs = 0;
    }
    lastStaConnected = connected;

    // 连接失败自愈：当前凭据连接失败（auth_failed / timeout 等任何 lastError 非空），
    // 而历史中同一 SSID 存有不同的密码时，解锁该槽位，让下面的重试状态机用历史
    // 凭据再试一次。历史保存的是最近一次成功连接的密码，可覆盖 NVS sta_pass 过期/
    // 写错（如 keep_password 只更新了 sta_ssid）导致开机反复失败、历史回退却因
    // 「已配置 SSID 可见」被锁死的场景。WPA2 密码错误在 ESP32 上多表现为 timeout
    // 而非 auth_failed，因此按 lastError 非空判定而非单一错误码。历史密码与当前
    // 一致时不解锁，避免同一个错误密码无限重试；用历史凭据重试后运行时密码与
    // 历史一致，本条件自然失效，重试次数有界。
    if (!connected && !wifiStaConnecting && wifiStaLastError[0] != 0) {
        int8_t rank = wifiStaHistoryRankOf(String(wifiStaSsid));
        String histPassword;
        if (rank >= 0 && findWifiStaHistoryEntry(String(wifiStaSsid), histPassword) &&
            histPassword != String(wifiStaPassword)) {
            wifiRuntime.staHistTriedMask &= (uint8_t)~(1u << rank);
        }
    }

    // 历史非空即进入重试窗口——覆盖 STA 从未配置（NVS sta_en=false 或从未配网）
    // 但历史记录非空的场景（Issue #88）：否则只能靠开机那一刻的扫描，运行中
    // 永不进入重试。历史为空时下方 wifiStaHistoryCount()==0 分支兜底，不会空转。
    bool inRetryWindow = !wifiStaConnected && !wifiStaConnecting && !wifiStaApplyPending &&
        (wifiStaConfigured || wifiStaLastError[0] != 0 || wifiStaHistoryCount() > 0);

    if (staHistScanPending) {
        int16_t scanResult = WiFi.scanComplete();
        if (scanResult == WIFI_SCAN_RUNNING) return;
        staHistScanPending = false;
        if (!inRetryWindow || wifiStaHistoryCount() == 0) {
            // 窗口已关闭（用户发起新的 apply 等）：丢弃扫描结果并结束本轮周期。
            WiFi.scanDelete();
            wifiRuntime.staHistRetryActive = false;
            return;
        }
        if (scanResult > 0) {
            int8_t bestRank = -1;
            uint8_t bestChannel = 0;
            for (uint8_t rank = 0; rank < wifiStaHistoryCount() && bestRank < 0; rank++) {
                if ((wifiRuntime.staHistTriedMask & (uint8_t)(1u << rank)) != 0) continue;
                String histSsid;
                if (!copyWifiStaHistorySsid(rank, histSsid)) continue;
                for (int16_t i = 0; i < scanResult; i++) {
                    if (WiFi.SSID(i) == histSsid) {
                        bestRank = (int8_t)rank;
                        bestChannel = (uint8_t)WiFi.channel(i);
                        break;
                    }
                }
            }
            if (bestRank >= 0) {
                String histSsid;
                String histPassword;
                copyWifiStaHistorySsid((uint8_t)bestRank, histSsid);
                findWifiStaHistoryEntry(histSsid, histPassword);
                WiFi.scanDelete();
                if (copyWifiStaSsid(histSsid) && copyWifiStaPassword(histPassword)) {
                    wifiStaConfigured = true;
                    wifiRuntime.staHistTriedMask |= (uint8_t)(1u << bestRank);
                    wifiRuntime.staHistRetryDeadlineMs = millis() + WIFI_STA_HISTORY_RETRY_INTERVAL_MS;
                    mus4Logf("wifi", "STA history retry: slot %d ssid=\"%s\" ch=%u",
                             bestRank, histSsid.c_str(), bestChannel);
                    applyWifiStaCredentials();
                }
                return;
            }
        }
        // 扫描失败或无可见的未试候选：把当前槽位全部标记为已试，本轮结束；
        // 记录重扫描冷却截止，冷却期满后由下方分支清掩码重开新一轮，
        // 覆盖「小车先开机、历史 Wi-Fi 后出现」的场景（Issue #88）。
        WiFi.scanDelete();
        wifiRuntime.staHistTriedMask = 0;
        for (uint8_t rank = 0; rank < wifiStaHistoryCount(); rank++) {
            wifiRuntime.staHistTriedMask |= (uint8_t)(1u << rank);
        }
        wifiRuntime.staHistRetryActive = false;
        wifiRuntime.staHistRescanDeadlineMs = millis() + WIFI_STA_HISTORY_RESCAN_INTERVAL_MS;
        mus4Logf("wifi", "STA history retry: candidates exhausted, rescan in %lus",
                 WIFI_STA_HISTORY_RESCAN_INTERVAL_MS / 1000);
        return;
    }

    if (!inRetryWindow) return;
    if (wifiStaHistoryCount() == 0) {
        wifiRuntime.staHistRetryActive = false;
        return;
    }
    bool anyUntried = false;
    for (uint8_t rank = 0; rank < wifiStaHistoryCount(); rank++) {
        if ((wifiRuntime.staHistTriedMask & (uint8_t)(1u << rank)) == 0) {
            anyUntried = true;
            break;
        }
    }
    if (!anyUntried) {
        wifiRuntime.staHistRetryActive = false;
        // 候选已全部试过：未到重扫描冷却期则等待，期满清掩码重开新一轮。
        if ((long)(millis() - wifiRuntime.staHistRescanDeadlineMs) < 0) return;
        wifiRuntime.staHistTriedMask = 0;
        wifiRuntime.staHistRetryActive = true;
        mus4LogLine("wifi", "STA history retry: starting new round");
    }
    wifiRuntime.staHistRetryActive = true;
    if ((long)(millis() - wifiRuntime.staHistRetryDeadlineMs) < 0) return;
    // 异步扫描（含隐藏 SSID）；即使启动失败也置 pending，下一轮按扫描失败
    // 收敛为「候选试完」，避免在 AP-only 下空转扫频。
    wifiRuntime.staHistRetryDeadlineMs = millis() + WIFI_STA_HISTORY_RETRY_INTERVAL_MS;
    WiFi.scanNetworks(true, true);
    staHistScanPending = true;
}

void updateWifiConsole()
{
    if (wifiConsoleStarted) {
        wifiCaptiveDnsServer.processNextRequest();
    }
#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
    processLlmnrPacket();
#endif
    // Keep mDNS alive: IGMP-snooping routers may drop multicast forwarding
    // after the group membership times out. Periodically restart mDNS to
    // force a fresh IGMP join and service announcement.
    if (wifiMdnsStarted) {
        static unsigned long lastMdnsRestartMs = 0;
        if (millis() - lastMdnsRestartMs >= 60000) {
            lastMdnsRestartMs = millis();
            MDNS.end();
            wifiMdnsStarted = false;
            startWifiMdnsIfNeeded();
            if (wifiMdnsStarted) {
                mus4LogLine("wifi", "mDNS restarted for refresh");
            } else {
                mus4LogLine("wifi", "mDNS refresh failed");
            }
        }
    }
    if (!wifiConsoleStarted) {
        // v1.7.18 起 AP/STA 互斥切换：STA_ONLY 状态下 AP 已主动关闭，wifiConsoleStarted
        // 也跟着置 false，但这并非异常，不能用 WIFI_CONSOLE_RETRY_INTERVAL_MS 周期
        // 重新拉起整个 console（那会把 AP 又开回来，破坏互斥语义）。
        // v1.7.21 调整：STA 在线即认为不需要重试 console；其它情况（包括开机 AP 起不来
        // 在 AP_STA 模式下、AP-only 下 AP 异常）都按 WIFI_CONSOLE_RETRY_INTERVAL_MS 重试。
        if (!wifiStaConnected && millis() - lastWifiConsoleStartAttemptMs >= WIFI_CONSOLE_RETRY_INTERVAL_MS) {
            setupWifiConsole();
        }
        return;
    }
    if (!wifiConsoleClient || !wifiConsoleClient.connected()) {
        WiFiClient nextClient = wifiConsoleServer.available();
        if (nextClient) {
            if (wifiConsoleClient) wifiConsoleClient.stop();
            wifiConsoleClient = nextClient;
            wifiConsoleClient.setNoDelay(true);
            wifiConsoleAuthenticated = false;
            wifiConsoleClient.println("MUS4 WiFi Console Ready");
            wifiConsoleClient.println("Use AUTH:<password> to unlock control commands");
            appendWebLog("tcp", "client connected");
        }
        return;
    }
    while (wifiConsoleClient.available()) {
        int c = wifiConsoleClient.read();
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n') {
            wifiConsoleBuf.buf[wifiConsoleBuf.len] = 0;
            String line = String(wifiConsoleBuf.buf);
            line.trim();
            String response;
            StringPrint out(response);
            appendWebLog("tcp", String("> ") + redactWirelessConsoleLine(line));
            processWirelessConsoleLine(line, out, WIRELESS_ORIGIN_TCP);
            wifiConsoleClient.print(response);
            appendWebLogLines("cmd", response);
            wifiConsoleBuf.len = 0;
            wifiConsoleBuf.overflow = false;
        } else {
            if (wifiConsoleBuf.len < sizeof(wifiConsoleBuf.buf) - 1) {
                wifiConsoleBuf.buf[wifiConsoleBuf.len++] = (char)c;
            } else {
                wifiConsoleBuf.len = 0;
                wifiConsoleBuf.overflow = true;
                wifiConsoleBuf.errors++;
                wifiConsoleClient.println("NACK:OVERFLOW");
            }
        }
    }
}

#endif // ENABLE_WIFI_CONSOLE
