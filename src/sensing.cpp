#include "sensing.h"

#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <HDC2080.h>
#include <MS56XX.h>
#include <Wire.h>


#define HDC2080_ADDR 0x40

MS5607 ms5607(MS56XX_ADDR_LOW, MS5607);
HDC2080 hdc2080(HDC2080_ADDR);

void sensingInit() {
    Wire.begin();
}

void sensingTask(void* pvParameters) {
    Serial.println("Sensing task started");
    while (1) {
        float ms5607Pressure;
        float ms5607Temperature;

        float hdcTemp;
        float hdcHum;

        Serial.print("MS5607 Pressure: ");
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