#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>

#include "datalogging.h"
#include "filesystem.h"
#include "gnss.h"
#include "sensing.h"
#include "servo.h"
#include "settings.h"
#include "types.h"

bool DEBUG = true;
bool GLOBAL_ALTITUDE_DROP_ALLOWED = true; // Jonathan - Make this a config eventually
int GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS = 10000;

static SensingData sensorData{0};
static GnssData gnssData{0};
static Settings settings{0};

#ifndef PULL_BRIDGE_OUTPUT_PIN
#define PULL_BRIDGE_OUTPUT_PIN 6
#endif
#ifndef PULL_BRIDGE_READ_PIN
#define PULL_BRIDGE_READ_PIN 7
#endif

#ifndef SERVO_CONTROL_PIN
#define SERVO_CONTROL_PIN 9
#endif

static bool withinGlobalAltitudeDrop(int dropMeters) {
    return GLOBAL_ALTITUDE_DROP_ALLOWED && dropMeters >= GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS;
}


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

    // loadSettings(settings);

    pinMode(PULL_BRIDGE_OUTPUT_PIN, OUTPUT);
    digitalWrite(PULL_BRIDGE_OUTPUT_PIN, LOW);
    pinMode(PULL_BRIDGE_READ_PIN, INPUT_PULLUP);

    servoInit(SERVO_CONTROL_PIN);
    servoClose(SERVO_CONTROL_PIN);

    if (DEBUG) {
        Serial.println("Debug logs active");
    }
}

static void handleTelemetryGet() {
    sensingExecute(sensorData);
    gnssExecute(gnssData);
}

static void handleDatalogging() {
    bool writePinHigh = (digitalRead(PULL_BRIDGE_READ_PIN) == LOW);
    static bool loggerOpen = false;

    if (writePinHigh) {
        if (DEBUG && !loggerOpen) {
            Serial.println("BRIDGED (logging enabled)");
        }

        loggerOpen = true;
        dataloggingExecute(gnssData, sensorData);
    } else if (loggerOpen) {
        if (DEBUG) {
            Serial.println("OPEN (logging disabled)");
        }

        loggerOpen = false;
        closeDatalogger();
    }
}


static void handleDrop() {
    static bool ledState = false;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    ledState = !ledState;
    servoOpen(SERVO_CONTROL_PIN);
    Serial.println("Altitude within geofence range");
    dataloggingSetDropped(true);
}

static void handleGeofencing() {
    int withinGeofenceIndex = isWithinGeofence(gnssData.latitude, gnssData.longitude);

    if (withinGeofenceIndex >= 0) {
        if (DEBUG) {
            Serial.print("Within geofence index: ");
            Serial.println(withinGeofenceIndex);
        }

        // Check if altitude is within the geofence's altitude range

        // Jonathan - Fix before next launch because we don't know if sensors or GPS will fail LOL
        if (isWithinGeofenceAltitude(withinGeofenceIndex,
                                     sensorData.baroAltitude) ||
            isWithinGeofenceAltitude(withinGeofenceIndex,
                                     gnssData.altitude)) {
            handleDrop();
        } else {
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("Altitude outside geofence range");
        }
    }
}


void loop() {
    handleTelemetryGet();
    handleDatalogging();
    handleGeofencing();

    if (withinGlobalAltitudeDrop(sensorData.baroAltitude) ||
        withinGlobalAltitudeDrop(gnssData.altitude)) {
            handleDrop();
    }

    if (DEBUG) {
        printSensingData();
        printGnssData();
    }
    delay(1000);
}