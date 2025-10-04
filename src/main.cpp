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
    // gnssInit();
}

void loop() {
    Serial.println("Main loop running");
    sensingExecute();
    // dataloggingExecute();
}