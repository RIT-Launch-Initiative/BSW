#pragma once

#include "types.h"

void dataloggingExecute(const GnssData& gnssData,
                        const SensingData& sensingData);
void dataloggingTask(void* pvParameters);