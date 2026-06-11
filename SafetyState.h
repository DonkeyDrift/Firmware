#pragma once
#include <Arduino.h>

enum EmergencyStopState
{
    EST_IDLE,
    EST_READY,
    EST_BRAKING,
    EST_DONE
};

extern EmergencyStopState emergencyStopState;

// Emergency stop state machine (updates car_output.throttle)
void emergencyStop();

// Park button state machine (updates rc_data.park / car_output.park)
void park_change();
