#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include <Arduino.h>
#include <oJSON.h>
#include <LittleFS.h>

Settings g_settings;
uint32_t g_bootcount = 0;

static const char* BOOTCOUNT_FILE = "/.bootcount";
static const char* SETTINGS_FILE = "/.settings";
static const char* LOG_FILE = "/log.txt";

void loadSettings() {
    File f = LittleFS.open(SETTINGS_FILE, "r");
    if (!f) {
        // Default settings if file doesn't exist
        g_settings.log_altitude_threshold = 100.0f;
        return;
    }
    char buf[256];
    size_t len = f.readBytes(buf, sizeof(buf) - 1);
    buf[len] = '\0';
    oJSON doc;
    doc.parse(buf);
    g_settings.log_altitude_threshold = doc["log_altitude_threshold"].toFloat(100.0f);
    f.close();
}

void saveSettings() {
    File f = LittleFS.open(SETTINGS_FILE, "w");
    if (!f) return;
    oJSON doc;
    doc["log_altitude_threshold"] = g_settings.log_altitude_threshold;
    String out = doc.stringify();
    f.print(out);
    f.close();
}

void dataloggingInit() {
    LittleFS.begin();
    // Bootcount
    File f = LittleFS.open(BOOTCOUNT_FILE, "r+");
    if (!f) {
        g_bootcount = 1;
        f = LittleFS.open(BOOTCOUNT_FILE, "w");
        f.print(g_bootcount);
    } else {
        g_bootcount = f.parseInt() + 1;
        f.seek(0);
        f.print(g_bootcount);
    }
    f.close();
    // Settings
    loadSettings();
}

void dataloggingTask(void* pvParameters) {
    Serial.println("Datalogging task started");
    while (1) {
        extern float ms5607Altitude, ms5607Pressure, ms5607Temperature;
        extern float hdcTemp, hdcHum;
        if (ms5607Altitude > g_settings.log_altitude_threshold) {
            File log = LittleFS.open(LOG_FILE, "a");
            if (log) {
                log.print("ALT:"); log.print(ms5607Altitude);
                log.print(", PRES:"); log.print(ms5607Pressure);
                log.print(", TEMP:"); log.print(ms5607Temperature);
                log.print(", HDC_TEMP:"); log.print(hdcTemp);
                log.print(", HDC_HUM:"); log.print(hdcHum);
                log.print(", BOOT:"); log.print(g_bootcount);
                log.println();
                log.close();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}