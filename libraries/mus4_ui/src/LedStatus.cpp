#include "LedStatus.h"

extern CRGB leds[];
extern bool toggleActive;
extern CRGB toggleColor1;
extern CRGB toggleColor2;
extern unsigned long toggleTime;
extern unsigned long toggleInterval;

// Modified setLEDColor function
void setLEDColor(CRGB targetColor)
{
    if (toggleActive)
    {
        toggleActive = false; // Disable toggle mode
        toggleTime = 0;       // Reset toggle time
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
    toggleActive = true;
    toggleTime = 0; // Ensure the first toggle runs immediately
}

void scanLEDToggle()
{
    if (toggleActive && (millis() >= toggleTime))
    {
        CRGB currentColor = leds[0];
        CRGB nextColor = (currentColor == toggleColor1) ? toggleColor2 : toggleColor1;
        leds[0] = nextColor;
        FastLED.show();
        toggleTime = millis() + toggleInterval;
    }
}
