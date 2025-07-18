#include "sensing.h"

#include <Arduino.h>

void sensingInit() {
}

void sensingTask(void* pvParameters) {
    // Task initialization
    Serial.println("Sensing task started");

    // Task loop
    while (1) {
        Serial.println("Sensing task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}