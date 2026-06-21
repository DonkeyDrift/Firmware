#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// Update driving mode according to the RC mode channel value.
void mode_change(bool modeValid);

// Compute final throttle/steering output based on mode, park state, RC/Pilot inputs.
void updateControlOutput();
