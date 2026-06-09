#include "GamepadMode.h"

#include "FirmwareConfig.h"

#ifdef ENABLE_GAMEPAD_MODE
#include <Arduino.h>
#include <BleGamepad.h>

extern BleGamepad bleGamepad;
extern volatile uint16_t pwm_value[];

void sendGamepadPacket() {
    if (bleGamepad.isConnected()) {
        // Map RC Channels (approx 1000-2000) to Gamepad Axes (-32767 to 32767)
        int lx = map(constrain(pwm_value[CH_STEERING], 1000, 2000), 1000, 2000, 0, 32767);
        int ly = map(constrain(pwm_value[CH_THROTTLE], 1300, 1800), 1300, 1800, 32767, 0);
        // int rx = map(constrain(pwm_value[CH_PARK], 1000, 2000), 1000, 2000, -32767, 32767);
        // int ry = map(constrain(pwm_value[CH_MODE], 1000, 2000), 1000, 2000, -32767, 32767);
        int rx = 0;
        int ry = 0;

        bleGamepad.setLeftThumb(0, ly);
        bleGamepad.setRightThumb(lx, 0);
    }
}
#endif
