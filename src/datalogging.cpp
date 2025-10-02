#include "datalogging.h"
#include "types.h"

#include <SPI.h>
#include <SdFat.h>
#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <TinyGPS++.h>

#define SD_CS_PIN 2 // Digital Pin 2

Settings g_settings;
uint32_t bootcount = 0;

static const char* BOOTCOUNT_FILE = "/.bootcount";
static const char* SETTINGS_FILE = "/.settings";
static const char* LOG_FILE = "/log.txt";
static constexpr size_t FILE_NAME_SIZE = 32;
static char CSV_FILE[FILE_NAME_SIZE] = "";

QueueHandle_t gnssQueue;
QueueHandle_t sensingQueue;


SdFat sd;

void loadSettings() {}

void saveSettings() {}

void dataloggingInit() {
    gnssQueue = xQueueCreate(4, sizeof(GnssData));
    sensingQueue = xQueueCreate(4, sizeof(SensingData));
    

    Serial.println("Initializing SD card...");
    if (!sd.begin(SD_CS_PIN)) {
        Serial.println("Failed to initialize SD card!");
        return;
    }

    uint32_t cardSize = sd.card()->sectorCount();
    if (cardSize == 0) {
        Serial.println("Could not read SD card size!");
    } else {
        Serial.print("SD Card Size: ");
        Serial.print((cardSize / 2048.0), 2); // MB
        Serial.println(" MB");
    }

    // Bootcount logic
    File file = sd.open(BOOTCOUNT_FILE, FILE_READ);
    if (!file) {
        bootcount = 1;
        file.close();
        file = sd.open(BOOTCOUNT_FILE, FILE_WRITE);
        if (file) {
            file.print(bootcount);
            file.close();
        }
    } else {
        bootcount = file.parseInt() + 1;
        file.close();
        file = sd.open(BOOTCOUNT_FILE, FILE_WRITE);
        if (file) {
            file.print(bootcount);
            file.close();
        }
    }

    char indexedLogFile[FILE_NAME_SIZE] = {0};
    snprintf(indexedLogFile, FILE_NAME_SIZE, "log_%lu.csv", (unsigned long)bootcount);
    File log = sd.open(indexedLogFile, FILE_WRITE);
    if (log) {
        log.println("Time,Latitude,Longitude,Altitude,Humidity,Temperature,Pressure,AltitudeTime");
        log.close();
    }

    strncpy((char*)CSV_FILE, indexedLogFile, FILE_NAME_SIZE);

    Serial.println("Datalogging initialized");
    Serial.print("Boot count: ");
    Serial.println(bootcount);
    Serial.print("Log file: ");
    Serial.println(CSV_FILE);
}

void dataloggingExecute() {
    GnssData gnssData = {};
    SensingData sensingData = {};
    
    xQueueReceive(gnssQueue, &gnssData, 0);
    xQueueReceive(sensingQueue, &sensingData, 0);
    // Write to CSV
    File log = sd.open(CSV_FILE, FILE_WRITE);
    if (log) {
        log.print(gnssData.time);
        log.print(",");
        log.print(gnssData.latitude, 6);
        log.print(",");
        log.print(gnssData.longitude, 6);
        log.print(",");
        log.print(gnssData.altitude, 2);
        log.print(",");
        log.print(sensingData.humidity, 2);
        log.print(",");
        log.print(sensingData.temperature, 2);
        log.print(",");
        log.print(sensingData.pressure, 2);
        log.print(",");
        log.print(sensingData.baroAltitude, 2);
        log.println();
        log.close();
    }
}

void dataloggingTask(void* pvParameters) {
    Serial.println("Datalogging task started");
    while (true) {
        dataloggingExecute();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}