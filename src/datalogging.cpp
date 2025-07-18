#include <FreeRTOS_SAMD21.h>
#include "datalogging.h"
#include <Arduino.h>

Settings g_settings;
uint32_t g_bootcount = 0;

static const char* BOOTCOUNT_FILE = "/.bootcount";
static const char* SETTINGS_FILE = "/.settings";
static const char* LOG_FILE = "/log.txt";

void loadSettings() {

}

void saveSettings() {

}

void dataloggingInit() {

}

void dataloggingTask(void* pvParameters) {
    Serial.println("Datalogging task started");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}