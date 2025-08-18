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

extern QueueHandle_t sensingQueue;

void sensingInit() {
    Wire.begin();
    ms5607.begin();
    hdc2080.begin();

    Serial.println("Sensors initialized");
}

void sensingTask(void* pvParameters) {
    Serial.println("Sensing task started");
    while (1) {
        ms5607.doBaro(true);
        float ms5607Pressure = ms5607.pressure;
        float ms5607Temperature = ms5607.temperature;
        float ms5607Altitude = ms5607.altitude;
        float hdcTemp = hdc2080.readTemp();
        float hdcHum = hdc2080.readHumidity();

        SensingData data;
        data.humidity = hdcHum;
        data.temperature = hdcTemp;
        data.pressure = ms5607Pressure;
        data.baroAltitude = ms5607Altitude;
        xQueueOverwrite(sensingQueue, &data);

        Serial.print("MS5607 Altitude: ");
        Serial.print(ms5607Altitude);
        Serial.print(" m, ");
        Serial.print("Pressure: ");
        Serial.print(ms5607Pressure);
        Serial.print(" mbar, ");
        Serial.print("Temp: ");
        Serial.print(ms5607Temperature);
        Serial.println(" C");

        Serial.print("HDC2080 Temp: ");
        Serial.print(hdcTemp);
        Serial.print(" C, ");
        Serial.print("Humidity: ");
        Serial.print(hdcHum);
        Serial.println(" %");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}