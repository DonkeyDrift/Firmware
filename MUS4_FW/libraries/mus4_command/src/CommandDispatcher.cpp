#include "CommandDispatcher.h"

#include "CommandParser.h"
#include "FirmwareConfig.h"
#include "Mus4Log.h"
#include "SharedTypes.h"
#include "JoystickCalibration.h"
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
    if (line.equalsIgnoreCase("JOYSTICK_CAL")) {
        startJoystickCalibration(out);
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_SAVE")) {
        if (joystick_cal_state == JoystickCalState::DONE) {
            bool steer_ok = validateJoystickCalibration(joystick_cal.steering);
            bool thr_ok = validateJoystickCalibration(joystick_cal.throttle);
            if (steer_ok && thr_ok) {
                joystick_cal.steering_enabled = true;
                joystick_cal.throttle_enabled = true;
                if (saveJoystickCalibration()) {
                    joystick_cal_state = JoystickCalState::IDLE;
                    out.println("ACK:JOYSTICK_SAVED");
                } else {
                    out.println("NACK:JOYSTICK_SAVE_FAILED");
                }
            } else {
                out.printf("NACK:JOYSTICK_INVALID_RANGE steer_ok=%d thr_ok=%d\n", steer_ok, thr_ok);
            }
        } else {
            out.println("NACK:JOYSTICK_NOT_DONE");
        }
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_RETRY")) {
        if (joystick_cal_state == JoystickCalState::DONE || joystick_cal_state == JoystickCalState::MINMAX) {
            joystick_cal_state = JoystickCalState::CENTERING;
            joystick_cal_stage_start_ms = millis();
            joystick_cal_temp_min[0] = INT16_MAX;
            joystick_cal_temp_min[1] = INT16_MAX;
            joystick_cal_temp_max[0] = INT16_MIN;
            joystick_cal_temp_max[1] = INT16_MIN;
            tui.log("[CAL] Retrying from center capture...");
            out.println("ACK:JOYSTICK_RETRY");
        } else {
            out.println("NACK:JOYSTICK_NOT_DONE");
        }
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_ABORT")) {
        abortJoystickCalibration();
        out.println("ACK:JOYSTICK_ABORTED");
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_RESET")) {
        resetJoystickCalibration();
        joystick_cal_state = JoystickCalState::IDLE;
        out.println("ACK:JOYSTICK_RESET");
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_STATUS")) {
        printJoystickCalStatus(out);
        return true;
    }

    // Legacy aliases
    if (line.equalsIgnoreCase("STEER_CAL")) {
        out.println("ACK:DEPRECATED_USE_JOYSTICK_CAL");
        return startJoystickCalibration(out);
    }
    if (line.equalsIgnoreCase("CAL_STATUS")) {
        printJoystickCalStatus(out);
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
