#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include "gnss.h"
#include "sensing.h"

bool DEBUG = true;

void setup() {
    Serial.begin(115200);
    delay(5000);

    sensingInit();
    dataloggingInit();
    gnssInit();

    // xTaskCreate(sensingTask, "Sensing Task", 8192, NULL, 1, NULL);
    // xTaskCreate(dataloggingTask, "Datalogging Task", 8192, NULL, 1, NULL);
    // xTaskCreate(gnssTask, "GNSS Task", 8192, NULL, 1, NULL);
}

void loop() {
    sensingExecute(DEBUG);
    delay(1000);
}