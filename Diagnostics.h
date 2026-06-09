#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"
#include "TUI.h"

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runBenchmarks();
bool runRegression();
#endif
