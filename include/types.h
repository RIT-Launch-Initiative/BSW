#pragma once

struct GnssData {
    char time[16];
    double latitude;
    double longitude;
    double altitude;
};

struct SensingData {
    float humidity;
    float temperature;
    float pressure;
    float baroAltitude;
    float hdcTemperature;
};
