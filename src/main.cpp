#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include "gnss.h"
#include "sensing.h"

bool DEBUG = true;

void setup() {
    Serial.begin(115200);
    delay(3000);

    sensingInit();
    dataloggingInit();
    gnssInit();
}

void loop() {
    sensingExecute(DEBUG);
    Serial.println("-----");
    // delay(1000);
}