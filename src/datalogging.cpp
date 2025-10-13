#include "datalogging.h"
#include "filesystem.h"
#include "types.h"

#include <SdFat.h>


static char CSV_FILE[FILE_NAME_SIZE] = "";
static const char* BOOTCOUNT_FILE = "/.bootcount";

void dataloggingInit() {
    // Bootcount
    uint32_t bootcount = readUintFromFile(BOOTCOUNT_FILE, 0) + 1;
    writeUintToFile(BOOTCOUNT_FILE, bootcount);

    // Log file name
    char indexedLogFile[FILE_NAME_SIZE] = {0};
    snprintf(indexedLogFile, FILE_NAME_SIZE, "log_%lu.csv",
             (unsigned long)bootcount);
    appendCsvHeaderIfNew(indexedLogFile);
    strncpy(CSV_FILE, indexedLogFile, FILE_NAME_SIZE);

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

    FsFile log = sd.open(CSV_FILE, O_WRITE | O_CREAT | O_APPEND);
    if (!log) return;

    log.print(millis());
    log.print(',');
    log.print(gnssData.time);
    log.print(',');
    log.print(gnssData.latitude, 6);
    log.print(',');
    log.print(gnssData.longitude, 6);
    log.print(',');
    log.print(gnssData.altitude, 2);
    log.print(',');
    log.print(sensingData.baroAltitude, 2);
    log.print(',');
    log.print(sensingData.pressure, 2);
    log.print(',');
    log.print(sensingData.temperature, 2);
    log.print(',');
    log.print(sensingData.humidity, 2);
    log.println();
    log.close();
}
