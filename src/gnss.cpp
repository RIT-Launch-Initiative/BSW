#include "datalogging.h"
#include "gnss.h"
#include "types.h"

#include <Adafruit_SPIFlash.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <cstdio>

static constexpr size_t MAX_GEOFENCES = 32;
static Geofence geofences[MAX_GEOFENCES]{0};
static size_t geofenceCount = 0;
static const uint32_t GPSBaud = 9600;

extern SdFs sd;
TinyGPSPlus gps;

int isWithinGeofence(double latitude, double longitude) {
    if (!gps.location.isValid()) return false;
    for (size_t i = 0; i < geofenceCount; ++i) {
        double dist = TinyGPSPlus::distanceBetween(
            latitude, longitude, geofences[i].latitude, geofences[i].longitude);
        if (dist <= geofences[i].radiusMeters) {
            return i;
        }
    }
    return -1;
}

void loadGeofences() {
    FsFile file = sd.open("/geofences", FILE_READ);
    if (!file) {
        geofenceCount = 0;
        return;
    }

    size_t fileSize = file.size();
    size_t structSize = sizeof(Geofence);
    geofenceCount = fileSize / structSize;
    if (geofenceCount > MAX_GEOFENCES) geofenceCount = MAX_GEOFENCES;
    file.read((uint8_t *)geofences, geofenceCount * structSize);
    file.close();
}

void gnssInit() {
    Serial.println("=============");
    Serial.println("    GNSS    ");
    Serial.println("=============");

    loadGeofences();
    Serial.print("\tLoaded ");
    Serial.print(geofenceCount);
    Serial.println(" geofences");
    for (size_t i = 0; i < geofenceCount; ++i) {
        Serial.print("\tGeofence ");
        Serial.print(i);
        Serial.print(": Lat ");
        Serial.print(geofences[i].latitude, 6);
        Serial.print(", Lon ");
        Serial.print(geofences[i].longitude, 6);
        Serial.print(", Radius ");
        Serial.print(geofences[i].radiusMeters);
        Serial.println(" m");
    }


    Serial1.begin(GPSBaud);
    Serial.println("GNSS initialized");
}

void gnssExecute(GnssData& data) {
    while (Serial1.available()) {
        gps.encode(Serial1.read());
    }

    if (gps.time.isValid()) {
        snprintf(data.time, sizeof(data.time), "%02d:%02d:%02d",
                 gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
        data.time[0] = '\0';
    }

    data.latitude  = gps.location.isValid() ? gps.location.lat()    : 0.0;
    data.longitude = gps.location.isValid() ? gps.location.lng()    : 0.0;
    data.altitude  = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
    snprintf(data.time, sizeof(data.time), "%02d:%02d:%02d",
             gps.time.hour(), gps.time.minute(), gps.time.second());
}