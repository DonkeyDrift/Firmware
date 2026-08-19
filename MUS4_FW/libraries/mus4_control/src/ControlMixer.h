#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// Update driving mode according to the RC mode channel value.
void mode_change(bool modeValid);

// Set the car driving mode from a host command (0=manual / 1=semi / 2=full).
// Returns false if the value is out of range; the current mode is left unchanged.
bool setCarModeCommand(int mode);

// Compute final throttle/steering output based on mode, park state, RC/Pilot inputs.
void updateControlOutput();
