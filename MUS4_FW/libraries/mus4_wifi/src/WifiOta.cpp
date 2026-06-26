#include "WifiOta.h"

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_partition.h>

#include "Mus4Log.h"
#include "SharedTypes.h"
#include "WirelessConsole.h"

// IDF OTA helpers are linked from the Arduino core but the header is not
// exposed; declare the small subset we need for boot-time cleanup.
extern "C" {
const esp_partition_t* esp_ota_get_running_partition(void);
const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t* start_from);
esp_err_t esp_ota_get_state_partition(const esp_partition_t* partition, int* out_state);
}

#ifdef ENABLE_WIFI_CONSOLE
// WIFI_CONSOLE_AP_PASSWORD, WIFI_OTA_PORT and WIFI_OTA_WINDOW_MS are defined
// in WifiConsoleTypes.h (included via RuntimeState.h -> WifiConsoleTypes.h).

extern SerialBuf wifiConsoleBuf;
extern ControlData rc_data;
extern ControlData car_output;

extern void ensureWifiOtaStarted();

void forceWifiOtaParkLocked()
{
    rc_data.park = PARK_LOCKED;
    car_output.park = PARK_LOCKED;
    car_output.throttle = 0;
}

void keepDevModeOtaWindowActive(OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (!ws.devModeEnabled) return;
    ensureWifiOtaStarted();
    os.windowOpen = true;
    os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS;
}

bool shouldEmitSerial1Telemetry(OtaRuntimeState& os)
{
    // v1.7.8 起：仅在 OTA 真正传输期间暂停 Serial1，避免 DEV ON 时 windowOpen
    // 长期为 true 阻塞与上位机通信。Park Guard 仍由 forceWifiOtaParkLocked()
    // 在传输期内托底。详见 docs/Plan/DEV模式影响面与运行逻辑映射.md §2.3。
    return !os.inProgress;
}

void openWifiOtaWindow(Print& out, WirelessCommandOrigin origin, OtaRuntimeState& os, WifiRuntimeState& ws)
{
    bool webDevMode = ws.devModeEnabled && origin == WIRELESS_ORIGIN_WEB;
    if (!webDevMode && !ws.consoleAuthenticated) {
        out.println("NACK:AUTH_REQUIRED");
        wifiConsoleBuf.errors++;
        return;
    }
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        wifiConsoleBuf.errors++;
        return;
    }
    os.parkGuardActive = true;
    forceWifiOtaParkLocked();
    ensureWifiOtaStarted();
    os.windowOpen = true;
    os.closeWsPending = true;
    os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    os.lastProgressPct = 0;
    out.printf("OTA_READY ip=%s port=%u ttl_ms=%lu\n", WiFi.localIP().toString().c_str(), WIFI_OTA_PORT, WIFI_OTA_WINDOW_MS);
    mus4LogLine("ota", webDevMode ? "ready: web_dev" : "ready");
}

void openLocalWifiOtaWindow(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os)
{
    if (!line.substring(11).equals(WIFI_CONSOLE_AP_PASSWORD)) {
        out.println("NACK:AUTH_REQUIRED");
        sb.errors++;
        return;
    }
    os.parkGuardActive = true;
    forceWifiOtaParkLocked();
    ensureWifiOtaStarted();
    os.windowOpen = true;
    os.closeWsPending = true;
    os.deadlineMs = millis() + WIFI_OTA_WINDOW_MS;
    os.lastProgressPct = 0;
    out.printf("OTA_READY ip=%s port=%u ttl_ms=%lu\n", WiFi.localIP().toString().c_str(), WIFI_OTA_PORT, WIFI_OTA_WINDOW_MS);
    mus4LogLine("ota", "ready: local");
}

bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (isLocalOtaOpenCommand(line)) {
        openLocalWifiOtaWindow(line, out, sb, os);
        return true;
    }
    if (isWirelessOtaStatusCommand(line)) {
        printWifiOtaStatus(out, os, ws);
        return true;
    }
    if (isWirelessOtaCloseCommand(line)) {
        closeWifiOtaWindow("LOCAL", os);
        out.println("OTA_CLOSED");
        return true;
    }
    return false;
}

void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (ws.devModeEnabled) keepDevModeOtaWindowActive(os, ws);
    if (!os.windowOpen) return;
    if (os.inProgress || os.parkGuardActive) {
        forceWifiOtaParkLocked();
    }
    unsigned long now = millis();
    if (!ws.devModeEnabled && !os.inProgress && (long)(now - os.deadlineMs) >= 0) {
        closeWifiOtaWindow("TIMEOUT", os);
        return;
    }
    ArduinoOTA.handle();
}

void closeWifiOtaWindow(const char* reason, OtaRuntimeState& os)
{
    os.windowOpen = false;
    os.closeWsPending = false;
    os.deadlineMs = 0;
    os.inProgress = false;
    os.parkGuardActive = false;
    os.lastProgressPct = 0;
    if (os.started) {
        ArduinoOTA.end();
        os.started = false;
    }
    mus4LogLine("ota", String("closed: ") + reason);
}

unsigned long wifiOtaTtlMs(OtaRuntimeState& os, WifiRuntimeState& ws)
{
    if (!os.windowOpen) return 0;
    if (ws.devModeEnabled) return WIFI_OTA_WINDOW_MS;
    unsigned long now = millis();
    if ((long)(os.deadlineMs - now) <= 0) return 0;
    return os.deadlineMs - now;
}

void printWifiOtaStatus(Print& out, OtaRuntimeState& os, WifiRuntimeState& ws)
{
    out.printf("OTA_STATUS started=%d window=%d in_progress=%d ttl_ms=%lu progress=%u park=%d dev_mode=%d park_guard=%d\n",
        os.started ? 1 : 0,
        os.windowOpen ? 1 : 0,
        os.inProgress ? 1 : 0,
        wifiOtaTtlMs(os, ws),
        os.lastProgressPct,
        car_output.park ? 1 : 0,
        ws.devModeEnabled ? 1 : 0,
        os.parkGuardActive ? 1 : 0);
}

void cleanupInvalidOtaPartition()
{
    const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
    if (!next) {
        mus4LogLine("ota", "no next update partition found");
        return;
    }

    int state = 0;
    esp_err_t err = esp_ota_get_state_partition(next, &state);
    if (err != ESP_OK) {
        mus4Logf("ota", "get_state %s failed: 0x%x", next->label, err);
        return;
    }

    // ESP_OTA_IMG_INVALID == 1, ESP_OTA_IMG_ABORTED == 3
    if (state == 1 || state == 3) {
        mus4Logf("ota", "erasing %s (state=%d)", next->label, state);
        err = esp_partition_erase_range(next, 0, next->size);
        if (err != ESP_OK) {
            mus4Logf("ota", "erase %s failed: 0x%x", next->label, err);
        } else {
            mus4LogLine("ota", "invalid/aborted ota partition erased");
        }
    } else {
        mus4Logf("ota", "%s state=%d, no cleanup needed", next->label, state);
    }
}
#endif