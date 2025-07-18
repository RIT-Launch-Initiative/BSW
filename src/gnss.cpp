#include "gnss.h"

#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <TinyGPS++.h>
void gnssInit() {}


void gnssTask(void* pvParameters) {
    Serial.println("GNSS task started");

    while (1) {
        Serial.println("GNSS task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}