#pragma once
#include <Arduino.h>


struct Settings {
    float log_altitude_threshold;
};

void dataloggingInit();
void dataloggingExecute();
void dataloggingTask(void* pvParameters);