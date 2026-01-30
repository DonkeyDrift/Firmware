
#define BUAD_RATE_0 115200
#define BUAD_RATE_1 115200
#define BUAD_RATE_2 115200
#define RX_1_PIN 16
#define TX_1_PIN 17
#define TX_2_PIN 18
#define RX_2_PIN 19
#define UART_SEL 12

void setup()
{
    pinMode(UART_SEL, OUTPUT);
    digitalWrite(UART_SEL, HIGH);

    Serial.begin(BUAD_RATE_0);                                  // TypeC
    Serial1.begin(BUAD_RATE_1, SERIAL_8N1, RX_1_PIN, TX_1_PIN); // RS232: rx = 16, tx = 17
    Serial2.begin(BUAD_RATE_2, SERIAL_8N1, RX_2_PIN, TX_2_PIN); // RS232: rx = 18, tx = 19
    
    Serial.println("ESP32 Receiver Serial Ready!");
    Serial1.println("ESP32 Receiver Serial1 Ready!");
    Serial2.println("ESP32 Receiver Serial2 Ready!");

}



void loop()
{
    if (Serial.available())
    {
        char c = Serial.read();
        Serial1.write(c);
        Serial2.write(c);
    }
    if (Serial1.available())
    {
        char c = Serial1.read();
        // Serial.write(c);
        Serial.println(c);
    }
    if (Serial2.available())
    {
        char c = Serial2.read();
        // Serial.write(c);
        Serial.println(c);
    }
}
