#include "LedStatus.h"
#include "Buzzer.h"

extern Buzzer buzzer; // MUS4_FW.ino 全局实例

extern CRGB leds[];
extern bool toggleActive;
extern bool toggleUse3Colors;
extern CRGB toggleColor1;
extern CRGB toggleColor2;
extern CRGB toggleColor3;
extern unsigned long toggleTime;
extern unsigned long toggleInterval;

// Defined further below; while the OTA glitch effect owns the LED, the normal
// status-LED entry points stay silent so ControlMixer updates cannot fight it.
bool isLedOtaGlitchActive();

// Modified setLEDColor function
void setLEDColor(CRGB targetColor)
{
    if (isLedOtaGlitchActive())
        return;
    if (toggleActive)
    {
        toggleActive = false;    // Disable toggle mode
        toggleUse3Colors = false;
        toggleTime = 0;          // Reset toggle time
    }
    if (leds[0] != targetColor)
    {
        leds[0] = targetColor;
        FastLED.show(); // Update display only when the color differs
    }
}

// Added setLEDToggle function
void setLEDToggle(CRGB color1, CRGB color2)
{
    if (isLedOtaGlitchActive())
        return;
    toggleColor1 = color1;
    toggleColor2 = color2;
    toggleUse3Colors = false;
    toggleActive = true;
    toggleTime = 0; // Ensure the first toggle runs immediately
}

// Three-color cycling variant: color1 -> color2 -> color3 -> color1 ...
void setLEDToggle(CRGB color1, CRGB color2, CRGB color3)
{
    if (isLedOtaGlitchActive())
        return;
    toggleColor1 = color1;
    toggleColor2 = color2;
    toggleColor3 = color3;
    toggleUse3Colors = true;
    toggleActive = true;
    toggleTime = 0; // Ensure the first toggle runs immediately
}

void scanLEDToggle()
{
    if (isLedOtaGlitchActive())
        return;
    if (toggleActive && (millis() >= toggleTime))
    {
        CRGB currentColor = leds[0];
        CRGB nextColor;
        if (toggleUse3Colors)
        {
            if (currentColor == toggleColor1)
                nextColor = toggleColor2;
            else if (currentColor == toggleColor2)
                nextColor = toggleColor3;
            else
                nextColor = toggleColor1;
        }
        else
        {
            nextColor = (currentColor == toggleColor1) ? toggleColor2 : toggleColor1;
        }
        leds[0] = nextColor;
        FastLED.show();
        toggleTime = millis() + toggleInterval;
    }
}

// Power-on self test: red, green, blue each solid for 1 second (3 seconds total).
// Blocking; called once from setup() after FastLED init.
// The 1s holds are sliced so buzzer.update() keeps running: the boot melody
// (AP start sound) is already playing when the self test runs, and a plain
// 3s delay() would stretch its first note until the loop takes over.
static void delaySelfTestHold(unsigned long ms)
{
    unsigned long start = millis();
    while (millis() - start < ms)
    {
        buzzer.update();
        delay(10);
    }
}

void runLedPowerOnSelfTest()
{
    setLEDColor(CRGB::Red);
    delaySelfTestHold(1000);
    setLEDColor(CRGB::Green);
    delaySelfTestHold(1000);
    setLEDColor(CRGB::Blue);
    delaySelfTestHold(1000);
}

// Apply the idle blink color selection: bit0 red, bit1 green, bit2 blue.
// 0 colors -> LED off, 1 color -> on/off blink (same 250ms interval as the
// multi-color toggle, so on and off last equally long), 2/3 colors ->
// alternating flash.
void applyLedBlinkMask(uint8_t mask)
{
    CRGB colors[3];
    int count = 0;
    if (mask & 1) colors[count++] = CRGB::Red;
    if (mask & 2) colors[count++] = CRGB::Green;
    if (mask & 4) colors[count++] = CRGB::Blue;

    if (count == 0)
        setLEDColor(CRGB::Black);
    else if (count == 1)
        setLEDToggle(colors[0], CRGB::Black);
    else if (count == 2)
        setLEDToggle(colors[0], colors[1]);
    else
        setLEDToggle(colors[0], colors[1], colors[2]);
}

// OTA transfer glitch effect: while an OTA upload is in progress the status
// LED flickers randomly — off/red/green/blue at random 30-120 ms intervals,
// like an electrical fault. Driven directly from the OTA upload callbacks
// (HTTP /update and ArduinoOTA onProgress) because the main loop, and with it
// scanLEDToggle(), is blocked inside those handlers for the whole transfer.
//
// After a successful OTA the effect carries on across the reboot: the upload
// handlers set a RTC-memory mark before ESP.restart(), setup() picks it up
// (takeLedOtaGlitchAfterReboot) and starts the effect again in
// "until buzzer idle" mode — the flicker then lasts until the boot melody has
// finished playing (loop() watches buzzer.isPlaying() and calls
// stopLedOtaGlitch() after a short idle grace period).
static unsigned long otaGlitchNextMs = 0;
static bool otaGlitchActive = false;
static bool otaGlitchUntilBuzzerIdle = false;

void startLedOtaGlitch()
{
    otaGlitchActive = true;
    toggleActive = false;    // take over from the normal blink state machine
    toggleUse3Colors = false;
    otaGlitchNextMs = 0;     // first scan applies immediately
}

void startLedOtaGlitchUntilBuzzerIdle()
{
    startLedOtaGlitch();
    otaGlitchUntilBuzzerIdle = true;
}

bool isLedOtaGlitchActive()
{
    return otaGlitchActive;
}

bool ledOtaGlitchWaitsForBuzzer()
{
    return otaGlitchUntilBuzzerIdle;
}

void scanLedOtaGlitch()
{
    unsigned long now = millis();
    if (now < otaGlitchNextMs)
        return;
    static const CRGB glitchColors[4] = {CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Blue};
    leds[0] = glitchColors[random(0, 4)];
    FastLED.show();
    otaGlitchNextMs = now + random(30, 121);
}

void stopLedOtaGlitch()
{
    // Clear the toggle state so ControlMixer re-applies the normal status LED
    // (mode color / blink mask) on the next loop.
    otaGlitchActive = false;
    otaGlitchUntilBuzzerIdle = false;
    toggleActive = false;
    toggleUse3Colors = false;
}

// RTC slow memory survives ESP.restart() (but is re-initialized on cold
// power-on), which makes it a safe one-shot channel from the OTA success
// handlers to the next boot.
RTC_DATA_ATTR static bool otaGlitchRebootMark = false;

void markLedOtaGlitchAfterReboot()
{
    otaGlitchRebootMark = true;
}

bool takeLedOtaGlitchAfterReboot()
{
    bool marked = otaGlitchRebootMark;
    otaGlitchRebootMark = false;
    return marked;
}
