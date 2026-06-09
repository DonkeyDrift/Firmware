#include "Diagnostics.h"

#include "Mus4Log.h"

extern TUI tui;

extern const int SERVO_MID_V;
extern const int SERVO_RANGE_V;

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
#endif
