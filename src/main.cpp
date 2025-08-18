#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include "gnss.h"
#include "sensing.h"

void setup() {
    Serial.begin(115200);
    delay(5000);

    sensingInit();
    dataloggingInit();
    gnssInit();


    xTaskCreate(sensingTask, "Sensing Task", 4096, NULL, 1, NULL);
    // xTaskCreate(dataloggingTask, "Datalogging Task", 4096, NULL, 1, NULL);
    // xTaskCreate(gnssTask, "GNSS Task", 4096, NULL, 1, NULL);
}

void loop() {
    Serial.println("Main loop running");
    delay(1000);
    
}