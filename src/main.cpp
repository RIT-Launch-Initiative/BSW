#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include "gnss.h"
#include "sensing.h"

void setup() {
    Serial.begin(9600);
    sensingInit();
    dataloggingInit();
    gnssInit();

    xTaskCreate(sensingTask, "Sensing Task", 2048, NULL, 1, NULL);
    xTaskCreate(dataloggingTask, "Datalogging Task", 2048, NULL, 1, NULL);
    xTaskCreate(gnssTask, "GNSS Task", 2048, NULL, 1, NULL);
}

void loop() {
}