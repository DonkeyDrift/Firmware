#include "Diagnostics.h"

#include "Mus4Log.h"

extern TUI tui;

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
#endif
