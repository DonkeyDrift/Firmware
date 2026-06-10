#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"
#include "TUI.h"

void notifyDegrade();
void evalDegrade();

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runBenchmarks();
bool runRegression();
bool runStress();
#endif
