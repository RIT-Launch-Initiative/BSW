#pragma once

struct Geofence {
    float latitude;
    float longitude;
    float radiusMeters;
};

void gnssInit();
void gnssExecute();
void gnssTask(void* pvParameters);