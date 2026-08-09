#include "LedStatus.h"

extern CRGB leds[];
extern bool toggleActive;
extern bool toggleUse3Colors;
extern CRGB toggleColor1;
extern CRGB toggleColor2;
extern CRGB toggleColor3;
extern unsigned long toggleTime;
extern unsigned long toggleInterval;

// Modified setLEDColor function
void setLEDColor(CRGB targetColor)
{
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
    toggleColor1 = color1;
    toggleColor2 = color2;
    toggleUse3Colors = false;
    toggleActive = true;
    toggleTime = 0; // Ensure the first toggle runs immediately
}

// Three-color cycling variant: color1 -> color2 -> color3 -> color1 ...
void setLEDToggle(CRGB color1, CRGB color2, CRGB color3)
{
    toggleColor1 = color1;
    toggleColor2 = color2;
    toggleColor3 = color3;
    toggleUse3Colors = true;
    toggleActive = true;
    toggleTime = 0; // Ensure the first toggle runs immediately
}

void scanLEDToggle()
{
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
void runLedPowerOnSelfTest()
{
    setLEDColor(CRGB::Red);
    delay(1000);
    setLEDColor(CRGB::Green);
    delay(1000);
    setLEDColor(CRGB::Blue);
    delay(1000);
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
