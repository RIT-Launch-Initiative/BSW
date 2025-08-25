#include "datalogging.h"
#include "types.h"

#include <Adafruit_SPIFlash.h>
#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <TinyGPS++.h>

Settings g_settings;
uint32_t bootcount = 0;

static const char* BOOTCOUNT_FILE = "/.bootcount";
static const char* SETTINGS_FILE = "/.settings";
static const char* LOG_FILE = "/log.txt";
static char CSV_FILE[32] = "";

QueueHandle_t gnssQueue;
QueueHandle_t sensingQueue;

Adafruit_FlashTransport_SPI flashTransport(EXTERNAL_FLASH_USE_CS, EXTERNAL_FLASH_USE_SPI);
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;

void loadSettings() {}

void saveSettings() {}

void dataloggingInit() {
    gnssQueue = xQueueCreate(4, sizeof(GnssData));
    sensingQueue = xQueueCreate(4, sizeof(SensingData));

    if (!fatfs.begin(&flash)) {
        Serial.println("Failed to initialize filesystem!");
        return;
    }

    File file = fatfs.open(BOOTCOUNT_FILE, FILE_READ);
    if (!file) {
        bootcount = 1;
        file = fatfs.open(BOOTCOUNT_FILE, FILE_WRITE);
        if (file) {
            file.print(bootcount);
            file.close();
        }
    } else {
        bootcount = file.parseInt() + 1;
        file.close();
        file = fatfs.open(BOOTCOUNT_FILE, FILE_WRITE);
        if (file) {
            file.print(bootcount);
            file.close();
        }
    }

    char indexedLogFile[32];
    snprintf(indexedLogFile, sizeof(indexedLogFile), "/data/log_%lu.csv", (unsigned long)bootcount);
    File log = fatfs.open(indexedLogFile, FILE_WRITE);
    if (log) {
        log.println("Time,Latitude,Longitude,Altitude,Humidity,Temperature,Pressure,AltitudeTime");
        log.close();
    }

    strncpy((char*)CSV_FILE, indexedLogFile, sizeof(indexedLogFile));

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
    File log = fatfs.open(CSV_FILE, FILE_WRITE);
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
    while (1) {
        dataloggingExecute();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}