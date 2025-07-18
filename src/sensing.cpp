#include <FreeRTOS_SAMD21.h>
#include "sensing.h"
#include <Arduino.h>
#include <Wire.h>
#include <MS5607.h>
#include <SparkFun_HDC2080.h>

#define MS5607_ADDR 0xED
#define HDC2080_ADDR 0x40

MS5607 ms5607(MS5607_ADDR);
HDC2080 hdc2080(HDC2080_ADDR);

void sensingInit() {
    Wire.begin();
    ms5607.begin();

}

void sensingTask(void* pvParameters) {
    Serial.println("Sensing task started");
    while (1) {
        ms5607.read();
        float pressure = ms5607.getPressure();
        float temperature = ms5607.getTemperature();

        // float hdcTemp = hdc2080.readTemp();
        // float hdcHum = hdc2080.readHumidity();

        Serial.print("MS5607 Pressure: "); 
        Serial.print(pressure); 
        Serial.print(" mbar, ");
        Serial.print("Temp: "); 
        Serial.print(temperature); 
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