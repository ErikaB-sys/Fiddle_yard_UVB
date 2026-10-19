#include <Arduino.h>

// ESP32 bring-up only: intentionally does not control FY hardware.
void setup() {
    Serial.begin(115200);
    Serial.println("FY ESP32 bring-up ready");
}

void loop() {
    delay(1000);
}
