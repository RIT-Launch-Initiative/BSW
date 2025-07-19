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

extern TinyGPSPlus gps;
extern float hdcTemp;
extern float hdcHum;
extern float ms5607Pressure;
extern float ms5607Altitude;
extern Adafruit_SPIFlash flash;
extern FatFileSystem fatfs;

void loadSettings() {}

void saveSettings() {}

void dataloggingInit() {}

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
    while (1) {
        String timeStr = "";
        if (gps.time.isValid()) {
            char buf[16];
            sprintf(buf, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(),
                    gps.time.second());
            timeStr = buf;
        }
        String latStr =
            gps.location.isValid() ? String(gps.location.lat(), 6) : "";
        String lonStr =
            gps.location.isValid() ? String(gps.location.lng(), 6) : "";
        String altStr =
            gps.altitude.isValid() ? String(gps.altitude.meters(), 2) : "";

        String humidStr = String(hdcHum, 2);
        String tempStr = String(hdcTemp, 2);
        String presStr = String(ms5607Pressure, 2);
        String alt2Str = String(ms5607Altitude, 2);

        log = fatfs.open(CSV_FILE, FILE_WRITE);
        if (log) {
            log.print(timeStr);
            log.print(",");
            log.print(latStr);
            log.print(",");
            log.print(lonStr);
            log.print(",");
            log.print(altStr);
            log.print(",");
            log.print(humidStr);
            log.print(",");
            log.print(tempStr);
            log.print(",");
            log.print(presStr);
            log.print(",");
            log.print(alt2Str);
            log.println();
            log.close();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}