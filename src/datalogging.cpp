#include "datalogging.h"

#include <Arduino.h>

void dataloggingInit() {

}

void dataloggingTask(void* pvParameters) {
    Serial.println("Datalogging task started");

    while (1) {
        // TODO
        Serial.println("Datalogging task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}