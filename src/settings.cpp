#include "settings.h"

#include <ArduinoJson.h>
#include <SdFat.h>

#include "filesystem.h"
#include "types.h"
static char SETTINGS_FILE[FILE_NAME_SIZE] = "settings.json";
extern SdFs sd;

void loadSettings(Settings& settings) {
    Serial.println("==============");
    Serial.println(" Load Settings ");
    Serial.println("==============");
    
    JsonDocument doc;
    uint8_t buffer[256] = {0};

    if (!sd.exists(SETTINGS_FILE)) {
        Serial.println("Settings file not found, using default settings");
        return;
    }

    FsFile file = sd.open(SETTINGS_FILE, O_READ);
    if (!file) {
        Serial.println("Failed to open settings file");
        return;
    }

    uint32_t fileSize = file.size();
    if (fileSize > sizeof(buffer)) {
        Serial.println("Settings file too large, using default settings");
        file.close();
        return;
    }

    file.read(buffer, fileSize);
    DeserializationError error = deserializeJson(doc, buffer);
    if (error) {
        Serial.print("Failed to parse settings file: ");
        Serial.println(error.c_str());
    } else {
        settings.logAltitudeThresholdMeters = doc["logAltitudeThresholdMeters"] | 100.0f;
        settings.baroAltitudeTolerance = doc["baroAltitudeTolerance"] | 5.0f;
        settings.gpsAltitudeTolerance = doc["gpsAltitudeTolerance"] | 5.0f;
    }

    file.close();
}