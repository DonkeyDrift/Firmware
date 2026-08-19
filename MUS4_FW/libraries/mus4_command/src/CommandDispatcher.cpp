#include "CommandDispatcher.h"

#include "CommandParser.h"
#include "FirmwareConfig.h"
#include "Mus4Log.h"
#include "SharedTypes.h"
#include "JoystickCalibration.h"
#include "ControlMixer.h"
#include "TUI.h"
#include "RuntimeState.h"

extern TUI tui;
extern ControlData pilot_data;
extern int lastSeq;
extern int servo_mid_v;
extern int motor_mid_v;

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

static bool handleJoystickSave(Print& out)
{
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

static bool handleJoystickRetry(Print& out)
{
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

static bool handleJoystickAbort(Print& out)
{
    abortJoystickCalibration();
    out.println("ACK:JOYSTICK_ABORTED");
    return true;
}

static bool handleJoystickReset(Print& out)
{
    resetJoystickCalibration();
    joystick_cal_state = JoystickCalState::IDLE;
    out.println("ACK:JOYSTICK_RESET");
    return true;
}

bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb, bool pilotSilent)
{
#ifdef ENABLE_AUTH_SERVICE
    // Auth 命令（CMD:READ_HW_ID / READ_UID / WRITE_UID / CLEAR_UID）
    // 优先于所有其他命令处理，避免被下面的调度逻辑误消费
    if (processAuthCommand(line, out)) {
        return true;
    }
#endif

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
    if (line.startsWith("MODE ") || line.startsWith("MODE:")) {
        String arg = line.substring(5);
        arg.trim();
        int m = -1;
        if (arg.length() == 1 && arg.charAt(0) >= '0' && arg.charAt(0) <= '2') {
            m = arg.charAt(0) - '0';
        }
        if (m >= 0 && setCarModeCommand(m)) {
            out.printf("ACK:MODE %d\n", m);
        } else {
            out.println("NACK:MODE_INVALID");
        }
        return true;
    }
    if (line.equalsIgnoreCase("SERVO_MID")) {
        out.printf("ACK:SERVO_MID %d\n", servo_mid_v);
        return true;
    }
    if (line.startsWith("SERVO_MID ")) {
        int duty = line.substring(strlen("SERVO_MID ")).toInt();
        if (saveServoMid((int16_t)duty)) {
            out.printf("ACK:SERVO_MID %d\n", servo_mid_v);
        } else {
            out.println("NACK:SERVO_MID_RANGE [4915, 9830]");
        }
        return true;
    }
    if (line.equalsIgnoreCase("MOTOR_MID")) {
        out.printf("ACK:MOTOR_MID %d\n", motor_mid_v);
        return true;
    }
    if (line.startsWith("MOTOR_MID ")) {
        int duty = line.substring(strlen("MOTOR_MID ")).toInt();
        if (saveMotorMid((int16_t)duty)) {
            out.printf("ACK:MOTOR_MID %d\n", motor_mid_v);
        } else {
            out.println("NACK:MOTOR_MID_RANGE [4915, 9830]");
        }
        return true;
    }
    if (line.equalsIgnoreCase("THROTTLE_MIN")) {
        out.printf("ACK:THROTTLE_MIN %d\n", joystick_cal.throttle_min_duty);
        return true;
    }
    if (line.startsWith("THROTTLE_MIN ")) {
        int val = line.substring(strlen("THROTTLE_MIN ")).toInt();
        if (saveThrottleMinDuty((int16_t)val)) {
            out.printf("ACK:THROTTLE_MIN %d\n", joystick_cal.throttle_min_duty);
        } else {
            out.printf("NACK:THROTTLE_RANGE [4915, %d]\n", motor_mid_v);
        }
        return true;
    }
    if (line.equalsIgnoreCase("THROTTLE_MAX")) {
        out.printf("ACK:THROTTLE_MAX %d\n", joystick_cal.throttle_max_duty);
        return true;
    }
    if (line.startsWith("THROTTLE_MAX ")) {
        int val = line.substring(strlen("THROTTLE_MAX ")).toInt();
        if (saveThrottleMaxDuty((int16_t)val)) {
            out.printf("ACK:THROTTLE_MAX %d\n", joystick_cal.throttle_max_duty);
        } else {
            out.printf("NACK:THROTTLE_RANGE [%d, 9830]\n", motor_mid_v);
        }
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_CAL")) {
        startJoystickCalibration(out);
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_SAVE")) {
        return handleJoystickSave(out);
    }
    if (line.equalsIgnoreCase("JOYSTICK_RETRY")) {
        return handleJoystickRetry(out);
    }
    if (line.equalsIgnoreCase("JOYSTICK_ABORT")) {
        return handleJoystickAbort(out);
    }
    if (line.equalsIgnoreCase("JOYSTICK_RESET")) {
        return handleJoystickReset(out);
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
    if (line.equalsIgnoreCase("CAL_SAVE")) {
        out.println("ACK:DEPRECATED_USE_JOYSTICK_SAVE");
        return handleJoystickSave(out);
    }
    if (line.equalsIgnoreCase("CAL_RETRY")) {
        out.println("ACK:DEPRECATED_USE_JOYSTICK_RETRY");
        return handleJoystickRetry(out);
    }
    if (line.equalsIgnoreCase("CAL_ABORT")) {
        out.println("ACK:DEPRECATED_USE_JOYSTICK_ABORT");
        return handleJoystickAbort(out);
    }
    if (line.equalsIgnoreCase("CAL_RESET")) {
        out.println("ACK:DEPRECATED_USE_JOYSTICK_RESET");
        return handleJoystickReset(out);
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
