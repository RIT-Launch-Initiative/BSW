#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include "gnss.h"
#include "sensing.h"
#include "types.h"

bool DEBUG = true;

static SensingData data;

static void printSensingData() {
    Serial.print("MS5607 Altitude: ");
    Serial.print(data.baroAltitude);
    Serial.print(" m, ");
    Serial.print("Pressure: ");
    Serial.print(data.pressure);
    Serial.print(" mbar, ");
    Serial.print("Temp: ");
    Serial.print(data.temperature);
    Serial.println(" C");

    Serial.print("HDC2080 Temp: ");
    Serial.print(data.hdcTemperature);
    Serial.print(" C, ");
    Serial.print("Humidity: ");
    Serial.print(data.humidity);
    Serial.println(" %");
}

void setup() {
    Serial.begin(115200);
    delay(3000);

    sensingInit();
    dataloggingInit();
    gnssInit();
}

void loop() {
    sensingExecute(data);

    if (DEBUG) {
        printSensingData();
    }
    delay(1000);
}