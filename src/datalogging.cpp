#include "datalogging.h"
#include "types.h"

#include <SPI.h>
#include <SdFat.h>
#include <Arduino.h>
#include <FreeRTOS_SAMD21.h>
#include <TinyGPS++.h>

#define SD_CS_PIN 2 // Digital Pin 2
#define INIT_MHZ 1
Settings g_settings;
uint32_t bootcount = 0;

static const char* BOOTCOUNT_FILE = "/.bootcount";
static const char* SETTINGS_FILE = "/.settings";
static const char* LOG_FILE = "/log.txt";
static constexpr size_t FILE_NAME_SIZE = 32;
static char CSV_FILE[FILE_NAME_SIZE] = "";

SdFat32 sd;    // new type
SdCard* card;  // handle for speed tuning

void loadSettings() {}

void saveSettings() {}

void dataloggingInit() {
    pinMode(MISO, INPUT_PULLUP);
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 10; ++i) SPI.transfer(0xFF);
    SPI.endTransaction();

    SdSpiConfig cfg(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(INIT_MHZ));

    Serial.println("cardBegin...");
    if (!sd.cardBegin(cfg)) {
        Serial.print("cardBegin FAIL  sdErr=0x");
        Serial.print(sd.sdErrorCode(), HEX);
        Serial.print("  sdData=0x");
        Serial.println(sd.sdErrorData(), HEX);
        return;
    }
    Serial.println("cardBegin OK");

    if (!sd.begin(cfg)) {
        Serial.print("FS mount FAIL  sdErr=0x");
        Serial.print(sd.sdErrorCode(), HEX);
        Serial.print("  sdData=0x");
        Serial.println(sd.sdErrorData(), HEX);
        return;
    }
    Serial.println("FS mount OK");

    File32 f = sd.open("hello.txt", O_WRITE | O_CREAT | O_APPEND);
    if (!f) {
        Serial.println("open failed");
        return;
    }
    f.println("hi");
    f.close();
    Serial.println("write OK");
}

// void dataloggingInit() {
//     // Be explicit about SPI mode & speed
//     SPI.begin();
//     // 80 clocks with CS high (spec requires ≥74)
//     pinMode(SD_CS_PIN, OUTPUT);
//     digitalWrite(SD_CS_PIN, HIGH);
//     SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
//     for (int i = 0; i < 10; ++i) SPI.transfer(0xFF);
//     SPI.endTransaction();

//     // Dedicated SPI keeps CS low for the whole transaction
//     SdSpiConfig cfg(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(1));  // 1 MHz for init
//     Serial.println("Initializing SD...");
//     if (!sd.begin(cfg)) {
//         Serial.print("sd.begin failed. errCode=0x");
//         Serial.print(sd.sdErrorCode(), HEX);
//         Serial.print(" errData=0x");
//         Serial.println(sd.sdErrorData(), HEX);
//         // If you see busy/timeout codes here, it still points to CS/MISO
//         // integrity.
//         return;
//     }

//     Serial.println("SD init OK");

//     //     Serial.println("Initializing SD card...");
//     //     Serial.println("Initializing SD...");
//     //     if (!sd.begin(SD_CS_PIN, SD_SCK_MHZ(1))) {
//     //         Serial.print("sd.begin failed. errCode=0x");
//     //         Serial.print(sd.sdErrorCode(), HEX);
//     //         Serial.print(" errData=0x");
//     //         Serial.println(sd.sdErrorData(), HEX);
//     //         return;
//     //     }

//     //     uint32_t cardSize = sd.card()->sectorCount();
//     //     if (cardSize == 0) {
//     //         Serial.println("Could not read SD card size!");
//     //     } else {
//     //         Serial.print("SD Card Size: ");
//     //         Serial.print((cardSize / 2048.0), 2); // MB
//     //         Serial.println(" MB");
//     //     }

//     //     // Bootcount logic
//     //     File file = sd.open(BOOTCOUNT_FILE, FILE_READ);
//     //     if (!file) {
//     //         bootcount = 1;
//     //         file.close();
//     //         file = sd.open(BOOTCOUNT_FILE, FILE_WRITE);
//     //         if (file) {
//     //             file.print(bootcount);
//     //             file.close();
//     //         }
//     //     } else {
//     //         bootcount = file.parseInt() + 1;
//     //         file.close();
//     //         file = sd.open(BOOTCOUNT_FILE, FILE_WRITE);
//     //         if (file) {
//     //             file.print(bootcount);
//     //             file.close();
//     //         }
//     //     }

//     //     char indexedLogFile[FILE_NAME_SIZE] = {0};
//     //     snprintf(indexedLogFile, FILE_NAME_SIZE, "log_%lu.csv", (unsigned
//     //     long)bootcount); File log = sd.open(indexedLogFile, FILE_WRITE); if
//     //     (log) {
//     //         log.println("Time,Latitude,Longitude,Altitude,Humidity,Temperature,Pressure,AltitudeTime");
//     //         log.close();
//     //     }

//     //     strncpy((char*)CSV_FILE, indexedLogFile, FILE_NAME_SIZE);

//     //     Serial.println("Datalogging initialized");
//     //     Serial.print("Boot count: ");
//     //     Serial.println(bootcount);
//     //     Serial.print("Log file: ");
//     //     Serial.println(CSV_FILE);
// }

void dataloggingExecute() {
    GnssData gnssData = {};
    SensingData sensingData = {};
    
    // Write to CSV
    File log = sd.open(CSV_FILE, FILE_WRITE);
    if (log) {
        log.print(gnssData.time);
        log.print(",");
        log.print(gnssData.latitude, 6);
        log.print(",");
        log.print(gnssData.longitude, 6);
        log.print(",");
        log.print(gnssData.altitude, 2);
        log.print(",");
        log.print(sensingData.humidity, 2);
        log.print(",");
        log.print(sensingData.temperature, 2);
        log.print(",");
        log.print(sensingData.pressure, 2);
        log.print(",");
        log.print(sensingData.baroAltitude, 2);
        log.println();
        log.close();
    }
}
