#include "sensing.h"
#include "datalogging.h"
#include "types.h"

#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <HDC2080.h>
#include <MS56XX.h>
#include <Wire.h>

#define HDC2080_ADDR 0x40

MS56XX ms5607(MS56XX_ADDR_LOW, MS5607);
HDC2080 hdc2080(HDC2080_ADDR);

uint8_t HDC2080_OFFSET_LSB = 90; // offset / 0.16 = 14 / 0.16 = 90

static void hdc2080Init() {

    hdc2080.begin();
    hdc2080.reset();

    hdc2080.setRate(ONE_HZ);
    hdc2080.setMeasurementMode(TEMP_AND_HUMID);
    hdc2080.setTempRes(FOURTEEN_BIT);
    hdc2080.setHumidRes(FOURTEEN_BIT);

    // This will have measurements trigger periodically
    hdc2080.triggerMeasurement();
}

void sensingInit() {
    Wire.begin();
    Serial.println("=============");
    Serial.println("   Sensors   ");
    Serial.println("=============");
    ms5607.begin();
    Serial.println("\tMS5607 initialized");
    hdc2080Init();
    Serial.println("\tHDC2080 initialized");
}

void sensingExecute(SensingData& data) {
    ms5607.doBaro(true);
    // HDC2080 doesn't need a call if setup properly

    data.humidity = hdc2080.readHumidity();
    data.hdcTemperature = hdc2080.readTemp();

    data.pressure = ms5607.pressure;
    data.temperature = ms5607.temperature;
    data.baroAltitude = ms5607.altitude;

}

void sensingTask(void* pvParameters) {
    Serial.println("Sensing task started");

    SensingData data;
    while (true) {
        sensingExecute(data);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}