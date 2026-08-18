#include "LedBlinkPreference.h"

// Idle LED blink is fixed at red+green+blue (7); no longer persisted or editable.
uint8_t getLedBlinkMask()
{
    return 7;
}
