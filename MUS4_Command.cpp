#include "MUS4.h"

// This file groups related MUS4 firmware implementation sections so the
// Arduino project stays easy to browse without changing runtime behavior.

// ============================================================================
// Section: CommandParser.cpp
// ============================================================================
uint8_t parseHex2(const char* s)
{
    auto hv = [](char c)->uint8_t{ if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return 10+(c-'a'); if(c>='A'&&c<='F')return 10+(c-'A'); return 0; };
    return (hv(s[0])<<4)|hv(s[1]);
}

uint8_t calcChecksum(const char* s, int n)
{
    uint32_t sum = 0;
    for (int i=0;i<n;i++) sum += (uint8_t)s[i];
    return (uint8_t)(sum & 0xFF);
}

bool parsePilotCommandLine(const String& line, int* throttle, int* steering, int* seq)
{
    *seq = -1;
    int star = line.lastIndexOf('*');
    if (star > 0)
    {
        String payload = line.substring(0, star);
        String cs = line.substring(star+1);
        if (cs.length()>=2)
        {
            char cs0 = cs.charAt(0);
            char cs1 = cs.charAt(1);
            char tmp[3]; tmp[0]=cs0; tmp[1]=cs1; tmp[2]=0;
            uint8_t want = parseHex2(tmp);
            int plen = payload.length();
            char buf[260]; int blen = plen; if (blen>259) blen=259;
            payload.toCharArray(buf, blen+1);
            uint8_t got = calcChecksum(buf, blen);
            if (want != got) return false;

            // Try to parse SEQ: T:S:SEQ
            int col2 = payload.lastIndexOf(':');
            int col1 = payload.indexOf(':');
            if (col2 > col1 && col1 > 0) {
                 String seqStr = payload.substring(col2+1);
                 *seq = seqStr.toInt();
                 return parseAndValidateCommand(payload.substring(0, col2), throttle, steering);
            }
            return parseAndValidateCommand(payload, throttle, steering);
        }
    }

    // No checksum; try to parse T:S:SEQ
    int col2 = line.lastIndexOf(':');
    int col1 = line.indexOf(':');
    if (col2 > col1 && col1 > 0) {
            String seqStr = line.substring(col2+1);
            *seq = seqStr.toInt();
            return parseAndValidateCommand(line.substring(0, col2), throttle, steering);
    }

    return parseAndValidateCommand(line, throttle, steering);
}

bool parseAndValidateCommand(String cmd, int* throttle, int* steering)
{
    int colonIndex = cmd.indexOf(':');
    if (colonIndex <= 0)
    {
        return false;
    }

    String throttleStr = cmd.substring(0, colonIndex);
    String steeringStr = cmd.substring(colonIndex + 1);

    int t = throttleStr.toInt();
    int s = steeringStr.toInt();

    // 校验控制范围，避免不可信输入直接影响执行器输出。
    if (t < -100 || t > 100 || s < -100 || s > 100)
    {
        // Print errors only when this is not a test command, to avoid polluting output
        // Serial.print("[CMD ERROR] Out of range: T=");
        // Serial.print(t);
        // Serial.print(" S=");
        // Serial.println(s);
        return false;
    }

    *throttle = t;
    *steering = s;
    return true;
}

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runUnitTests()
{
    int testsTotal = 0;
    int testsPassed = 0;
    int t, s, seq;

    // Basic format
    testsTotal++; if (parsePilotCommandLine(String("0:0"), &t, &s, &seq) && t == 0 && s == 0 && seq == -1) testsPassed++;
    testsTotal++; if (!parsePilotCommandLine(String("200:0"), &t, &s, &seq)) testsPassed++;
    // Checksum format
    char payload1[] = "10:-10";
    uint8_t cs1 = calcChecksum(payload1, sizeof(payload1)-1);
    char line1[32]; snprintf(line1, sizeof(line1), "%s*%02X", payload1, cs1);
    testsTotal++; if (parsePilotCommandLine(String(line1), &t, &s, &seq) && t == 10 && s == -10 && seq == -1) testsPassed++;
    // Seq format
    testsTotal++; if (parsePilotCommandLine(String("50:50:100"), &t, &s, &seq) && t == 50 && s == 50 && seq == 100) testsPassed++;
    // Seq + Checksum
    char payload2[] = "20:-20:255";
    uint8_t cs2 = calcChecksum(payload2, sizeof(payload2)-1);
    char line2[32]; snprintf(line2, sizeof(line2), "%s*%02X", payload2, cs2);
    testsTotal++; if (parsePilotCommandLine(String(line2), &t, &s, &seq) && t == 20 && s == -20 && seq == 255) testsPassed++;

    return testsPassed * 100 / testsTotal >= 85;
}
#endif

// ============================================================================
// Section: LocalCommands.cpp
// ============================================================================
extern TUI tui;
extern bool ansiEnabled;
extern bool filterDebugEnabled;

bool processLine(const String& line, int* throttle, int* steering, int* seq)
{
    // Handle local commands
    if (line.equalsIgnoreCase("NOANSI")) { ansiEnabled = false; tui.setAnsiEnabled(false); tui.forceRedraw(); return false; }
    if (line.equalsIgnoreCase("ANSI")) { ansiEnabled = true; tui.setAnsiEnabled(true); tui.forceRedraw(); return false; }
    if (line.equalsIgnoreCase("FILTER_DEBUG")) {
        filterDebugEnabled = !filterDebugEnabled;
        mus4Logf("filter", "Filter Debug: %s", filterDebugEnabled ? "ON" : "OFF");
        return false;
    }
#ifdef ENABLE_DIAGNOSTIC_COMMANDS
    if (line.equalsIgnoreCase("FILTER_TEST")) {
        runFilterTests();
        return false;
    }
#endif

    return parsePilotCommandLine(line, throttle, steering, seq);
}

// ============================================================================
// Section: CommandDispatcher.cpp
// ============================================================================
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

bool dispatchCommandLine(const String& line, Print& out, SerialBuf& sb)
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
        if (seq >= 0) out.printf("ACK:%d\n", seq);
        else out.println("ACK");
        sb.frames++;
    } else {
        if (seq >= 0) out.printf("NACK:%d\n", seq);
        else out.println("NACK");
        sb.errors++;
    }
    return ok;
}

// ============================================================================
// Section: Diagnostics.cpp
// ============================================================================
extern TUI tui;

extern const int SERVO_MID_V;
extern const int SERVO_RANGE_V;

extern SensorData ina219Data;
extern SensorData mpu6050Data;
extern unsigned long lastUICycleDuration;
extern bool degradeMode;
extern uint32_t degradeReason;

extern SerialBuf serial0Buf;
extern bool processLine(const String& line, int* throttle, int* steering, int* seq);

void notifyDegrade()
{
    mus4LogLine("system", "DEGRADED MODE ACTIVE");
}

void evalDegrade()
{
    degradeReason = 0;
    if (!ina219Data.valid) degradeReason |= 0x01;
    if (!mpu6050Data.valid) degradeReason |= 0x02;
    if (lastUICycleDuration > 150) degradeReason |= 0x04;
    if (degradeReason != 0 && !degradeMode)
    {
        degradeMode = true;
        notifyDegrade();
    }
    if (degradeReason == 0 && degradeMode)
    {
        degradeMode = false;
    }
}

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runBenchmarks()
{
    unsigned long ts = millis();
    unsigned long loops = 0;
    unsigned long durStart = millis();
    while (millis() - durStart < 200)
    {
        tui.forceRedraw();
        tui.render();
        loops++;
    }
    unsigned long t1 = millis() - ts;
    unsigned long score = loops;
    mus4Logf("bench", "BENCH: loops=%lu duration=%lums", score, t1);
    return score > 1;
}

bool runRegression()
{
    int v = map(-100, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    int v2 = map(100, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    bool ok = (v <= v2);
    mus4Logf("regress", "REGRESS: ok=%d", ok ? 1 : 0);
    return ok;
}

bool runStress()
{
    uint32_t errs0 = serial0Buf.errors;
    for (int i = 0; i < 50; i++)
    {
        int tt, ss, seq;
        processLine(String("999:999"), &tt, &ss, &seq);
    }
    mus4Logf("stress", "STRESS: errors_delta=%lu", serial0Buf.errors - errs0);
    return true;
}
#endif

// ============================================================================
// Section: SteeringCalibration.cpp
// ============================================================================
extern TUI tui;
extern ControlData car_output;
extern uint16_t pwm_filtered[];

static WifiRuntimeState* g_ws = nullptr;

void setSteeringCalibrationRuntimeState(WifiRuntimeState& ws)
{
    g_ws = &ws;
}

static inline Preferences& prefs()
{
    return *g_ws->prefs;
}

SteeringCalibration steer_cal;
bool steer_cal_enabled = false;
SteerCalState steer_cal_state = STEER_CAL_IDLE;
unsigned long steer_cal_stage_start_ms = 0;
int16_t steer_cal_temp_min = 0;
int16_t steer_cal_temp_max = 0;

void loadSteeringCalibration()
{
    steer_cal.min_pwm = RC_STEERING_MIN;
    steer_cal.mid_pwm = RC_STEERING_MID;
    steer_cal.max_pwm = RC_STEERING_MAX;
    if (!prefs().begin(MUS4_PREF_NAMESPACE, true)) {
        steer_cal_enabled = false;
        return;
    }
    steer_cal_enabled = prefs().getBool(MUS4_PREF_STEER_CAL_EN_KEY, false);
    if (steer_cal_enabled) {
        steer_cal.min_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MIN_KEY, RC_STEERING_MIN);
        steer_cal.mid_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MID_KEY, RC_STEERING_MID);
        steer_cal.max_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MAX_KEY, RC_STEERING_MAX);
    }
    prefs().end();
    mus4Logf("cal", "steer_cal enabled=%d min=%d mid=%d max=%d",
             steer_cal_enabled ? 1 : 0, steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm);
}

bool saveSteeringCalibration()
{
    if (!prefs().begin(MUS4_PREF_NAMESPACE, false)) return false;
    prefs().putShort(MUS4_PREF_STEER_MIN_KEY, steer_cal.min_pwm);
    prefs().putShort(MUS4_PREF_STEER_MID_KEY, steer_cal.mid_pwm);
    prefs().putShort(MUS4_PREF_STEER_MAX_KEY, steer_cal.max_pwm);
    prefs().putBool(MUS4_PREF_STEER_CAL_EN_KEY, true);
    prefs().end();
    steer_cal_enabled = true;
    mus4Logf("cal", "saved min=%d mid=%d max=%d", steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm);
    return true;
}

void resetSteeringCalibration()
{
    steer_cal.min_pwm = RC_STEERING_MIN;
    steer_cal.mid_pwm = RC_STEERING_MID;
    steer_cal.max_pwm = RC_STEERING_MAX;
    steer_cal_enabled = false;
    if (prefs().begin(MUS4_PREF_NAMESPACE, false)) {
        prefs().remove(MUS4_PREF_STEER_MIN_KEY);
        prefs().remove(MUS4_PREF_STEER_MID_KEY);
        prefs().remove(MUS4_PREF_STEER_MAX_KEY);
        prefs().remove(MUS4_PREF_STEER_CAL_EN_KEY);
        prefs().end();
    }
    mus4LogLine("cal", "reset to defaults");
}

int mapSteeringCalibrated(int16_t pwm)
{
    if (pwm < steer_cal.mid_pwm) {
        return map(pwm, steer_cal.min_pwm, steer_cal.mid_pwm, -100, 0);
    } else {
        return map(pwm, steer_cal.mid_pwm, steer_cal.max_pwm, 0, 100);
    }
}

void printCalStatus(Print& out)
{
    out.printf("CAL_STATUS enabled=%d min=%d mid=%d max=%d state=%d\n",
               steer_cal_enabled ? 1 : 0,
               steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm,
               (int)steer_cal_state);
}

bool startSteerCalibration(Print& out)
{
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        return false;
    }
    steer_cal_state = STEER_CAL_CENTER;
    steer_cal_stage_start_ms = millis();
    steer_cal_temp_min = 32767;
    steer_cal_temp_max = -32768;
    tui.log("[CAL] Keep steering centered, auto-capture in 3s...");
    mus4LogLine("cal", "center stage started");
    return true;
}

void updateSteerCalibration()
{
    if (steer_cal_state == STEER_CAL_IDLE) return;

    unsigned long now = millis();
    unsigned long elapsed = now - steer_cal_stage_start_ms;

    if (steer_cal_state == STEER_CAL_CENTER) {
        if (elapsed < 3000) return;
        steer_cal.mid_pwm = (int16_t)pwm_filtered[CH_STEERING];
        char buf[64];
        snprintf(buf, sizeof(buf), "[CAL] Center captured: %d", steer_cal.mid_pwm);
        tui.log(buf);
        mus4Logf("cal", "center=%d", steer_cal.mid_pwm);
        steer_cal_state = STEER_CAL_MINMAX;
        steer_cal_stage_start_ms = now;
        steer_cal_temp_min = 32767;
        steer_cal_temp_max = -32768;
        tui.log("[CAL] Swing stick full left/right within 5s...");
    } else if (steer_cal_state == STEER_CAL_MINMAX) {
        int16_t current = (int16_t)pwm_filtered[CH_STEERING];
        if (current < steer_cal_temp_min) steer_cal_temp_min = current;
        if (current > steer_cal_temp_max) steer_cal_temp_max = current;
        if (elapsed < 5000) return;
        steer_cal.min_pwm = steer_cal_temp_min;
        steer_cal.max_pwm = steer_cal_temp_max;
        char buf[96];
        snprintf(buf, sizeof(buf), "[CAL] Range captured: min=%d max=%d", steer_cal.min_pwm, steer_cal.max_pwm);
        tui.log(buf);
        mus4Logf("cal", "range min=%d max=%d", steer_cal.min_pwm, steer_cal.max_pwm);
        steer_cal_state = STEER_CAL_DONE;
        snprintf(buf, sizeof(buf), "[CAL] Result: mid=%d min=%d max=%d", steer_cal.mid_pwm, steer_cal.min_pwm, steer_cal.max_pwm);
        tui.log(buf);
        tui.log("[CAL] Send CAL_SAVE / CAL_RETRY / CAL_ABORT");
    }
}

