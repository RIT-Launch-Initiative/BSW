#pragma once

#include <types.h>

void sensingInit();
void sensingExecute(SensingData& data);
void sensingTask(void* pvParameters);