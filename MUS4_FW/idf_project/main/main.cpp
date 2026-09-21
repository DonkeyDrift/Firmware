#include <Arduino.h>

// Minimal "Arduino as an ESP-IDF component" shim for MUS4_FW Stage A.
// The arduino-esp32 component does NOT provide app_main; the application
// must provide it and drive setup()/loop().

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("MUS4 IDF arduino-esp32 shim: setup()");
}

void loop()
{
    static uint32_t n = 0;
    Serial.printf("shim loop %lu, free heap=%u\n", (unsigned long)n++, (unsigned)ESP.getFreeHeap());
    delay(1000);
}

extern "C" void app_main(void)
{
    initArduino();
    setup();
    for (;;) {
        loop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
