#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>


struct Settings {
    float log_altitude_threshold;
    // Add more settings as needed
};

extern Settings g_settings;
extern uint32_t g_bootcount;

void dataloggingInit();
void dataloggingTask(void* pvParameters);