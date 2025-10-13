#pragma once

struct Settings {
    float log_altitude_threshold;
};

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

struct Geofence {
    float latitude;
    float longitude;
    float radiusMeters;
};