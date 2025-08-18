#include "datalogging.h"
#include "gnss.h"
#include "types.h"

#include <Adafruit_SPIFlash.h>
#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <TinyGPS++.h>

static Geofence geofences[32]{0};
static size_t geofenceCount = 0;

static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
extern Adafruit_SPIFlash flash;
extern FatFileSystem fatfs;

extern QueueHandle_t gnssQueue;

static void printFloat(float val, bool valid, int len, int prec) {
    if (!valid) {
        while (len-- > 1) Serial.print('*');
        Serial.print(' ');
    } else {
        Serial.print(val, prec);
        int vi = abs((int)val);
        int flen = prec + (val < 0.0 ? 2 : 1);  // . and -
        flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
        for (int i = flen; i < len; ++i) Serial.print(' ');
    }
}

static void printInt(unsigned long val, bool valid, int len) {
    char sz[32] = "*****************";
    if (valid) sprintf(sz, "%ld", val);
    sz[len] = 0;
    for (int i = strlen(sz); i < len; ++i) sz[i] = ' ';
    if (len > 0) sz[len - 1] = ' ';
    Serial.print(sz);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t) {
    if (!d.isValid()) {
        Serial.print(F("********** "));
    } else {
        char sz[32];
        sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
        Serial.print(sz);
    }
    if (!t.isValid()) {
        Serial.print(F("******** "));
    } else {
        char sz[32];
        sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
        Serial.print(sz);
    }
    printInt(d.age(), d.isValid(), 5);
}

static void printStr(const char *str, int len) {
    int slen = strlen(str);
    for (int i = 0; i < len; ++i) Serial.print(i < slen ? str[i] : ' ');
}

static bool isWithinGeofence() {
    if (!gps.location.isValid()) return false;
    double lat = gps.location.lat();
    double lon = gps.location.lng();
    for (size_t i = 0; i < geofenceCount; ++i) {
        double dist = TinyGPSPlus::distanceBetween(
            lat, lon, geofences[i].latitude, geofences[i].longitude);
        if (dist <= geofences[i].radiusMeters) {
            return true;
        }
    }
    return false;
}

void loadGeofences() {
    if (!fatfs.begin(&flash)) {
        geofenceCount = 0;
        return;
    }

    File file = fatfs.open("/geofences", FILE_READ);
    if (!file) {
        geofenceCount = 0;
        return;
    }

    size_t fileSize = file.size();
    size_t structSize = sizeof(Geofence);
    geofenceCount = fileSize / structSize;
    if (geofenceCount > 32) geofenceCount = 32;
    file.read((uint8_t *)geofences, geofenceCount * structSize);
    file.close();
}

void gnssInit() {
    Serial1.begin(GPSBaud); 
    Serial.println("GNSS initialized");
}

void gnssTask(void *pvParameters) {
    Serial.println("GNSS task started");
    Serial.println(F(
        "Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt   "
        " Course Speed Card  Chars Sentences Checksum"));
    Serial.println(
        F("           (deg)      (deg)       Age                      Age  (m) "
          "   --- from GPS ----  RX    RX        Fail"));
    Serial.println(F(
        "----------------------------------------------------------------------"
        "--------------------------------------------------------------"));
    while (1) {
        while (Serial1.available()) gps.encode(Serial1.read());
        GnssData data = {};
        if (gps.time.isValid()) {
            sprintf(data.time, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
        } else {
            data.time[0] = '\0';
        }
        data.latitude = gps.location.isValid() ? gps.location.lat() : 0.0;
        data.longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
        data.altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
        xQueueOverwrite(gnssQueue, &data);
        printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
        printFloat(gps.hdop.hdop(), gps.hdop.isValid(), 6, 1);
        printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
        printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
        printInt(gps.location.age(), gps.location.isValid(), 5);
        printDateTime(gps.date, gps.time);
        printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
        printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
        printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
        printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "*** ", 6);
        printInt(gps.charsProcessed(), true, 6);
        printInt(gps.sentencesWithFix(), true, 10);
        printInt(gps.failedChecksum(), true, 9);
        Serial.println();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
