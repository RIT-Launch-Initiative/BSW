#include "gnss.h"

#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <TinyGPS++.h>


static Geofence geofences[32]{0};
static size_t geofenceCount = 0;

void gnssInit() {}


void gnssTask(void* pvParameters) {
    Serial.println("GNSS task started");

    while (1) {
        Serial.println("GNSS task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}