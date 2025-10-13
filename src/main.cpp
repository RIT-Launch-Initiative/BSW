#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include "filesystem.h"
#include "gnss.h"
#include "sensing.h"
#include "types.h"

bool DEBUG = true;

static SensingData sensorData{0};
static GnssData gnssData{0};

static void printSensingData() {
    Serial.print("MS5607 Altitude: ");
    Serial.print(sensorData.baroAltitude);
    Serial.print(" m, ");
    Serial.print("Pressure: ");
    Serial.print(sensorData.pressure);
    Serial.print(" mbar, ");
    Serial.print("Temp: ");
    Serial.print(sensorData.temperature);
    Serial.println(" C");

    Serial.print("HDC2080 Temp: ");
    Serial.print(sensorData.hdcTemperature);
    Serial.print(" C, ");
    Serial.print("Humidity: ");
    Serial.print(sensorData.humidity);
    Serial.println(" %");
}

void setup() {
    Serial.begin(115200);
    delay(3000);

    sensingInit();
    dataloggingInit();
    gnssInit();

    if (DEBUG) {
        Serial.println("Debug logs active");
    }
}

void loop() {
    sensingExecute(sensorData);
    // TODO: gnssExecute(gnssData);
    dataloggingExecute(gnssData, sensorData);

    if (DEBUG) {
        printSensingData();
    }
    delay(1000);
}