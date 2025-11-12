#include "datalogging.h"
#include "filesystem.h"
#include "types.h"

#include <SdFat.h>


static char CSV_FILE[FILE_NAME_SIZE] = "";
static bool DROPPED = false;
static FsFile logFile;

extern SdFs sd;


static bool appendCsvHeaderIfNew(const char* path) {
    if (!sd.exists(path)) {
        FsFile f = sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
        if (!f) return false;
        f.println(
            "UptimeMillis,Time,Latitude,Longitude,GPSAltitude,BaroAltitude,"
            "Pressure,Temperature,Humidity,Dropped");
        f.close();

        return true;
    }

    return false;
}

static void printCardSize(const uint64_t cardSizeBytes) {
    uint64_t cardSizeInBytes = cardSizeBytes * 512ULL; // SD cards use 512-byte sectors
    Serial.print("\tCard size: ");
    if (cardSizeBytes < (2ULL * 1024 * 1024)) {
        Serial.print(cardSizeBytes / 1024);
        Serial.println(" KB");
    } else if (cardSizeBytes < (2ULL * 1024 * 1024 * 1024)) {
        Serial.print(cardSizeBytes / (1024 * 1024));
        Serial.println(" MB");
    } else {
        Serial.print(cardSizeBytes / (1024 * 1024 * 1024));
        Serial.println(" GB");
    }
}

void dataloggingInit() {
    Serial.println("=============");
    Serial.println(" Datalogging ");
    Serial.println("=============");

    uint32_t bootcount = getBootcount();

    // Log file name
    char indexedLogFile[FILE_NAME_SIZE] = {0};
    snprintf(indexedLogFile, FILE_NAME_SIZE, "log_%lu.csv", bootcount);
    appendCsvHeaderIfNew(indexedLogFile);
    strncpy(CSV_FILE, indexedLogFile, FILE_NAME_SIZE);
    CSV_FILE[FILE_NAME_SIZE - 1] = '\0';

    Serial.print("\tBoot count: ");
    Serial.println(bootcount);
    Serial.print("\tLog file: ");
    Serial.println(CSV_FILE);
}

void dataloggingExecute(const GnssData& gnssData,
                        const SensingData& sensingData) {
    if (!CSV_FILE[0]) {
        Serial.println("CSV_FILE not set, skipping datalogging");
        return;
    }

    logFile = sd.open(CSV_FILE, O_WRITE | O_CREAT | O_APPEND);
    if (!logFile) return;

    logFile.print(millis());
    logFile.print(',');
    logFile.print(gnssData.time);
    logFile.print(',');
    logFile.print(gnssData.latitude, 6);
    logFile.print(',');
    logFile.print(gnssData.longitude, 6);
    logFile.print(',');
    logFile.print(gnssData.altitude, 2);
    logFile.print(',');
    logFile.print(sensingData.baroAltitude, 2);
    logFile.print(',');
    logFile.print(sensingData.pressure, 2);
    logFile.print(',');
    logFile.print(sensingData.temperature, 2);
    logFile.print(',');
    logFile.print(sensingData.humidity, 2);
    logFile.print(',');
    logFile.print(DROPPED ? 'T' : 'F');
    logFile.println();
    logFile.close();
}

void closeDatalogger() {
    if (!CSV_FILE[0]) {
        Serial.println("CSV_FILE not set, skipping datalogging close");
        return;
    }

    if (logFile.isOpen()) {
        logFile.close();
    }
}