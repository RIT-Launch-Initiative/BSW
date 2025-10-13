#pragma once

struct Settings {
    float logAltitudeThresholdMeters;
    float baroAltitudeTolerance;
    float gpsAltitudeTolerance;
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