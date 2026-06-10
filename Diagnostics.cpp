#include "Diagnostics.h"

#include "Mus4Log.h"

extern TUI tui;

extern const int SERVO_MID_V;
extern const int SERVO_RANGE_V;

extern SensorData ina219Data;
extern SensorData mpu6050Data;
extern unsigned long lastUICycleDuration;
extern bool degradeMode;
extern uint32_t degradeReason;

struct SerialBuf { char buf[256]; uint16_t len; uint32_t frames; uint32_t errors; bool overflow; };
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
