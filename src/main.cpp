#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include "gnss.h"
#include "sensing.h"
#include <types.h>

bool DEBUG = true;

static SensingData data;

void setup() {
    Serial.begin(115200);
    delay(3000);

    sensingInit();
    dataloggingInit();
    gnssInit();
}

void loop() {
    sensingExecute(data, DEBUG);
    Serial.println("-----");
    // delay(1000);
}