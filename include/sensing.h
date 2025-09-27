#pragma once

void sensingInit();
void sensingExecute(SensingData& data, const bool debug);
void sensingTask(void* pvParameters);