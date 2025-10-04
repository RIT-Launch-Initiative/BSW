#pragma once
#include <Arduino.h>

#include "types.h"

struct Settings {
    float log_altitude_threshold;
};

void dataloggingInit();
void dataloggingExecute(const GnssData& gnssData, const SensingData& sensingData);
void dataloggingTask(void* pvParameters);