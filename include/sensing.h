#pragma once

#include <types.h>

void sensingInit();
void sensingExecute(SensingData& data, const bool debug);
void sensingTask(void* pvParameters);