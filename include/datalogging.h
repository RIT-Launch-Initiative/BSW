#pragma once

#include "types.h"

void dataloggingInit();

void dataloggingExecute(const GnssData& gnssData,
                        const SensingData& sensingData);

void closeDatalogger();

void dataloggingSetDropped(bool dropped);