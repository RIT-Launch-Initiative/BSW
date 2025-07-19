#include "datalogging.h"

#include <Adafruit_SPIFlash.h>
#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <TinyGPS++.h>

Settings g_settings;
uint32_t g_bootcount = 0;

static const char* BOOTCOUNT_FILE = "/.bootcount";
static const char* SETTINGS_FILE = "/.settings";
static const char* LOG_FILE = "/log.txt";
static const char* CSV_FILE = "/log.csv";

QueueHandle_t gnssQueue;
QueueHandle_t sensingQueue;

struct GnssData {
    char time[16];
    double latitude;
    double longitude;
    double altitude;
};

struct SensingData {
    float humidity;
    float temperature;
    float pressure;
    float baroAltitude;
};

Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;

void loadSettings() {}

void saveSettings() {}

void dataloggingInit() {
    gnssQueue = xQueueCreate(4, sizeof(GnssData));
    sensingQueue = xQueueCreate(4, sizeof(SensingData));
}

void dataloggingTask(void* pvParameters) {
    Serial.println("Datalogging task started");
    File log = fatfs.open(CSV_FILE, FILE_READ);
    if (!log) {
        log = fatfs.open(CSV_FILE, FILE_WRITE);
        if (log) {
            log.println(
                "Time,Latitude,Longitude,Altitude,Humidity,Temperature,"
                "Pressure,AltitudeTime");
            log.close();
        }
    } else {
        log.close();
    }
    GnssData gnssData = {};
    SensingData sensingData = {};
    while (1) {
        // Wait for latest GNSS and Sensing data
        xQueueReceive(gnssQueue, &gnssData, 0);
        xQueueReceive(sensingQueue, &sensingData, 0);
        // Write to CSV
        log = fatfs.open(CSV_FILE, FILE_WRITE);
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
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}