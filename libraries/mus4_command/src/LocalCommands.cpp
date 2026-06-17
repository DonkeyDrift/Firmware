#include "LocalCommands.h"

#include "CommandParser.h"
#include "Mus4Log.h"
#include "RcFilter.h"
#include "TUI.h"

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
