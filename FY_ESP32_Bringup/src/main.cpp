#include <Arduino.h>
#include <freertos/task.h>
#include "FY_Bringup.h"

// Entry point only. Huginn and Muninn implementation lives in modules.
void setup() {
    (void)fy::startBringup();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
