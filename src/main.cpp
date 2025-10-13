#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>

#include "datalogging.h"
#include "filesystem.h"
#include "gnss.h"
#include "sensing.h"
#include "settings.h"
#include "types.h"

bool DEBUG = true;

static SensingData sensorData{0};
static GnssData gnssData{0};

static void printSensingData() {
    Serial.print("MS5607 Altitude: ");
    Serial.print(sensorData.baroAltitude);
    Serial.print(" m, ");
    Serial.print("Pressure: ");
    Serial.print(sensorData.pressure);
    Serial.print(" mbar, ");
    Serial.print("Temp: ");
    Serial.print(sensorData.temperature);
    Serial.println(" C");

    Serial.print("HDC2080 Temp: ");
    Serial.print(sensorData.hdcTemperature);
    Serial.print(" C, ");
    Serial.print("Humidity: ");
    Serial.print(sensorData.humidity);
    Serial.println(" %");
}

static void printGnssData() {
    Serial.print("GNSS Time: ");
    Serial.print(gnssData.time);
    Serial.print(", Lat: ");
    Serial.print(gnssData.latitude, 6);
    Serial.print(", Lon: ");
    Serial.print(gnssData.longitude, 6);
    Serial.print(", Alt: ");
    Serial.print(gnssData.altitude);
    Serial.println(" m");
}

void setup() {
    Serial.begin(115200);
    delay(3000);

    filesystemInit();

    sensingInit();
    dataloggingInit();
    gnssInit();

    loadSettings(settings);

    if (DEBUG) {
        Serial.println("Debug logs active");
    }
}

void loop() {
    sensingExecute(sensorData);
    gnssExecute(gnssData);
    dataloggingExecute(gnssData, sensorData);
    int withinGeofenceIndex = isWithinGeofence(gnssData.latitude, gnssData.longitude);

    if (withinGeofenceIndex >= 0) {
        static bool ledState = false;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        ledState = !ledState;
    }

    if (DEBUG) {
        printSensingData();
        printGnssData();
        if (withinGeofenceIndex >= 0) {
            Serial.print("Within geofence index: ");
            Serial.println(withinGeofenceIndex);
        }
    }
    delay(1000);
}