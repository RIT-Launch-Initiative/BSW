#include "datalogging.h"

#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <SPI.h>
#include <SdFat.h>

#define SD_CS_PIN 2
#define INIT_MHZ 1
static constexpr size_t FILE_NAME_SIZE = 32;

Settings g_settings;
uint32_t bootcount = 0;

static const char* BOOTCOUNT_FILE = "/.bootcount";
static char CSV_FILE[FILE_NAME_SIZE] = "";

SdFs sd;

// TODO: If this is ever used. Initialize them with xQueueCreate
QueueHandle_t gnssQueue = nullptr;
QueueHandle_t sensingQueue = nullptr;

static uint32_t readUintFromFile(const char* path, uint32_t fallback = 0) {
    FsFile f = sd.open(path, O_READ);
    if (!f) return fallback;
    char buf[32] = {0};
    int n = f.read(buf, sizeof(buf) - 1);
    f.close();
    if (n <= 0) return fallback;
    return (uint32_t)strtoul(buf, nullptr, 10);
}

static bool writeUintToFile(const char* path, uint32_t v) {
    FsFile f = sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
    if (!f) return false;
    f.println(v);
    f.close();
    return true;
}

static bool appendCsvHeaderIfNew(const char* path) {
    if (!sd.exists(path)) {
        FsFile f = sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
        if (!f) return false;
        f.println(
            "UptimeMillis,Time,Latitude,Longitude,GPSAltitude,BaroAltitude,"
            "Pressure,Temperature,Humidity");
        f.close();
    }
    return true;
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

    pinMode(MISO, INPUT_PULLUP);
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 10; ++i) SPI.transfer(0xFF);
    SPI.endTransaction();
    Serial.println("SPI initialized");

    SdSpiConfig cfg(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(INIT_MHZ));

    if (!sd.cardBegin(cfg)) {
        Serial.print("\tcardBegin failed  sdErr=0x");
        Serial.print(sd.sdErrorCode(), HEX);
        Serial.print(" sdData=0x");
        Serial.println(sd.sdErrorData(), HEX);
        return;
    }
    Serial.println("\tCard initialized");

    uint64_t cardSizeInSectors = sd.card()->sectorCount();
    printCardSize(cardSizeInSectors);

    if (!sd.begin(cfg)) {
        Serial.print("\tFS mount failed sdErr=0x");
        Serial.print(sd.sdErrorCode(), HEX);
        Serial.print(" sdData=0x");
        Serial.println(sd.sdErrorData(), HEX);
        return;
    }
    Serial.println("\tFS mounted");

    // Bootcount
    bootcount = readUintFromFile(BOOTCOUNT_FILE, 0) + 1;
    writeUintToFile(BOOTCOUNT_FILE, bootcount);

    // Log file name
    char indexedLogFile[FILE_NAME_SIZE] = {0};
    snprintf(indexedLogFile, FILE_NAME_SIZE, "log_%lu.csv",
             (unsigned long)bootcount);
    if (!appendCsvHeaderIfNew(indexedLogFile)) {
        Serial.println(
            "\tERROR: Failed to create log file or write CSV header.");
        return;
    }
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

    FsFile log = sd.open(CSV_FILE, O_WRITE | O_CREAT | O_APPEND);
    if (!log) {
        Serial.print("Failed to open CSV file: ");
        Serial.print(CSV_FILE);
        Serial.print(" (sdErrorCode: ");
        Serial.print(sd.sdErrorCode());
        Serial.print(", sdErrorData: ");
        Serial.print(sd.sdErrorData());
        Serial.println(")");
        return;
    }

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

void dataloggingTask(void* pvParameters) {
    Serial.println("Datalogging task started");

    while (true) {
        GnssData gnssData = {0};
        SensingData sensingData = {0};

        if (gnssQueue) {
            xQueueReceive(gnssQueue, &gnssData, 0);
        }
        if (sensingQueue) {
            xQueueReceive(sensingQueue, &sensingData, 0);
        }

        dataloggingExecute(gnssData, sensingData);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
