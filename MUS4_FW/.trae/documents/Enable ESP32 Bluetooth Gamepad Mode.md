I will modify the `mus4.ino` firmware to add Bluetooth Gamepad support and verify it using the provided Python script.

1.  **Modify `mus4.ino`**:
    *   **Add Definitions**: Insert `#define ENABLE_GAMEPAD_MODE` and `#include <BleGamepad.h>` at the beginning of the file.
    *   **Global Object**: Declare `BleGamepad bleGamepad("Gamepad MU03", "Espressif", 100);`.
    *   **Setup**: In `setup()`, add `bleGamepad.begin()` wrapped in `#ifdef ENABLE_GAMEPAD_MODE`.
    *   **Implement `sendGamepadPacket()`**:
        *   Create this function to read `pwm_value` array.
        *   Map RC channels (1000-2000µs) to Gamepad Axes (-32767 to 32767).
        *   Mapping: Steering -> Left X, Throttle -> Left Y, Park -> Right X, Mode -> Right Y.
        *   Use `bleGamepad.setLeftThumb()` and `bleGamepad.setRightThumb()`.
    *   **Loop Integration**: Call `sendGamepadPacket()` inside the `loop()` function (wrapped in `#ifdef`), ensuring it runs periodically.

2.  **Verify & Upload**:
    *   Run `python ~/project/mus4/arduino-cli.py -cum` to compile the sketch, upload it to the ESP32, and open the serial monitor to check for successful initialization.
