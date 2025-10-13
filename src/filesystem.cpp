#include "filesystem.h"

#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <SPI.h>
#include <SdFat.h>
#include <TinyGPS++.h>



#define SD_CS_PIN 2
#define INIT_MHZ 1

static const char* BOOTCOUNT_FILE = "/.bootcount";
static uint32_t bootcount = 0;
SdFs sd;

uint32_t readUintFromFile(const char* path, uint32_t fallback) {
    FsFile f = sd.open(path, O_READ);
    if (!f) return fallback;
    char buf[32] = {0};
    int n = f.read(buf, sizeof(buf) - 1);
    f.close();
    if (n <= 0) return fallback;
    return (uint32_t)strtoul(buf, nullptr, 10);
}

bool writeUintToFile(const char* path, uint32_t v) {
    FsFile f = sd.open(path, O_WRITE | O_CREAT | O_TRUNC);
    if (!f) return false;
    f.println(v);
    f.close();
    return true;
}

uint32_t getBootcount() {
    return bootcount;
}


void filesystemInit() {
    Serial.println("=============");
    Serial.println(" Filesystem ");
    Serial.println("=============");

    pinMode(MISO, INPUT_PULLUP);
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 10; ++i) SPI.transfer(0xFF);
    SPI.endTransaction();
    Serial.println("\tSPI initialized");

    SdSpiConfig cfg(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(INIT_MHZ));

    if (!sd.cardBegin(cfg)) {
        Serial.print("\tcardBegin failed  sdErr=0x");
        Serial.print(sd.sdErrorCode(), HEX);
        Serial.print(" sdData=0x");
        Serial.println(sd.sdErrorData(), HEX);
        return;
    }
    Serial.println("\tCard initialized");

    uint64_t cardSize = sd.card()->cardSize();
    uint64_t cardSizeBytes = cardSize * 512;
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
}
