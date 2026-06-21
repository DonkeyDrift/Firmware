#include "CommandDispatcher.h"

#include "CommandParser.h"
#include "FirmwareConfig.h"
#include "Mus4Log.h"
#include "SharedTypes.h"
#include "SteeringCalibration.h"
#include "TUI.h"
#include "RuntimeState.h"

extern TUI tui;
extern ControlData pilot_data;
extern int lastSeq;

extern bool processLine(const String& line, int* throttle, int* steering, int* seq);
#ifdef ENABLE_WIFI_CONSOLE
extern bool processLocalOtaMaintenanceCommand(const String& line, Print& out, SerialBuf& sb, OtaRuntimeState& os, WifiRuntimeState& ws);
extern bool processWifiStaConfigCommand(const String& line, Print& out, WifiRuntimeState& ws);

static OtaRuntimeState* g_otaState = nullptr;
static WifiRuntimeState* g_wifiState = nullptr;

void setCommandDispatcherRuntimeStates(OtaRuntimeState& os, WifiRuntimeState& ws)
{
    g_otaState = &os;
    g_wifiState = &ws;
}
#else
extern bool processWifiStaConfigCommand(const String& line, Print& out);
#endif

bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb, bool pilotSilent)
{
#ifdef ENABLE_WIFI_CONSOLE
    if (g_otaState && g_wifiState && processLocalOtaMaintenanceCommand(line, out, sb, *g_otaState, *g_wifiState)) {
        return true;
    }
    if (g_wifiState && processWifiStaConfigCommand(line, out, *g_wifiState)) {
        return true;
    }
#else
    if (processWifiStaConfigCommand(line, out)) {
        return true;
    }
#endif
    if (line.equalsIgnoreCase("LOG_WEB")) {
        setMus4LogTargetWeb();
        mus4LogLine("log", mus4LogTarget == MUS4_LOG_TARGET_WEB ? "target=web" : "target=serial wifi_disabled");
        out.println("ACK:LOG_WEB");
        return true;
    }
    if (line.equalsIgnoreCase("LOG_SERIAL")) {
        mus4LogTarget = MUS4_LOG_TARGET_SERIAL;
        mus4LogLine("log", "target=serial");
        out.println("ACK:LOG_SERIAL");
        return true;
    }
    if (line.equalsIgnoreCase("STEER_CAL")) {
        startSteerCalibration(out);
        return true;
    }
    if (line.equalsIgnoreCase("CAL_SAVE")) {
        if (steer_cal_state == STEER_CAL_DONE) {
            if (steer_cal.min_pwm < steer_cal.mid_pwm && steer_cal.mid_pwm < steer_cal.max_pwm
                && (steer_cal.mid_pwm - steer_cal.min_pwm) > 100 && (steer_cal.max_pwm - steer_cal.mid_pwm) > 100) {
                if (saveSteeringCalibration()) {
                    steer_cal_state = STEER_CAL_IDLE;
                    out.println("ACK:CAL_SAVED");
                } else {
                    out.println("NACK:CAL_SAVE_FAILED");
                }
            } else {
                out.println("NACK:CAL_INVALID_RANGE");
            }
        } else {
            out.println("NACK:CAL_NOT_DONE");
        }
        return true;
    }
    if (line.equalsIgnoreCase("CAL_RETRY")) {
        if (steer_cal_state == STEER_CAL_DONE) {
            steer_cal_state = STEER_CAL_CENTER;
            steer_cal_stage_start_ms = millis();
            steer_cal_temp_min = 32767;
            steer_cal_temp_max = -32768;
            tui.log("[CAL] Retrying center capture...");
            out.println("ACK:CAL_RETRY");
        } else {
            out.println("NACK:CAL_NOT_DONE");
        }
        return true;
    }
    if (line.equalsIgnoreCase("CAL_ABORT")) {
        steer_cal_state = STEER_CAL_IDLE;
        loadSteeringCalibration();
        out.println("ACK:CAL_ABORTED");
        return true;
    }
    if (line.equalsIgnoreCase("CAL_RESET")) {
        resetSteeringCalibration();
        steer_cal_state = STEER_CAL_IDLE;
        out.println("ACK:CAL_RESET");
        return true;
    }
    if (line.equalsIgnoreCase("CAL_STATUS")) {
        printCalStatus(out);
        return true;
    }

    int t, s, seq;
    bool ok = processLine(line, &t, &s, &seq);
    if (ok) {
        pilot_data.throttle = t;
        pilot_data.steering = s;
        lastSeq = seq;
        if (!pilotSilent) {
            if (seq >= 0) out.printf("ACK:%d\n", seq);
            else out.println("ACK");
        }
        sb.frames++;
    } else {
        if (!pilotSilent) {
            if (seq >= 0) out.printf("NACK:%d\n", seq);
            else out.println("NACK");
        }
        sb.errors++;
    }
    return ok;
}
