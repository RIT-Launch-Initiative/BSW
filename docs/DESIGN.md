# BSW (Balloon Software) — Developer Guide

## Table of Contents

- [BSW (Balloon Software) — Developer Guide](#bsw-balloon-software--developer-guide)
  - [Table of Contents](#table-of-contents)
  - [Hardware Platform](#hardware-platform)
  - [System Architecture](#system-architecture)
    - [Main Control Loop](#main-control-loop)
    - [Data Flow](#data-flow)
  - [Module Reference](#module-reference)
    - [`filesystem.cpp`](#filesystemcpp)
    - [`gnss.cpp`](#gnsscpp)
    - [`sensing.cpp`](#sensingcpp)
    - [`datalogging.cpp`](#dataloggingcpp)
    - [`servo.cpp`](#servocpp)
    - [`settings.cpp`](#settingscpp)
    - [`geofences_writer.py`](#geofences_writerpy)
  - [Drop Trigger Logic](#drop-trigger-logic)
    - [Primary Trigger: Geofence + Altitude](#primary-trigger-geofence--altitude)
    - [Backup Trigger: Global Altitude Failsafe](#backup-trigger-global-altitude-failsafe)
  - [Sensor Redundancy Strategy](#sensor-redundancy-strategy)
  - [Pin Assignments](#pin-assignments)
  - [Libraries and Dependencies](#libraries-and-dependencies)
  - [Build System](#build-system)
  - [Future Considerations](#future-considerations)
  - [Adding a New Geofence Field](#adding-a-new-geofence-field)

---

## Hardware Platform

| Property | Value |
|---|---|
| Board | Adafruit ItsyBitsy M0 |
| MCU | ATSAMD21G18A (ARM Cortex-M0+ @ 48MHz) |
| Flash | 256KB |
| RAM | 32KB |
| Framework | Arduino via PlatformIO |
| RTOS | FreeRTOS SAMD21 port (pulled in but not actively used — see [Known Issues](#known-issues-and-tech-debt)) |

---

## System Architecture

### Main Control Loop

The system runs a simple sequential 1 Hz loop. There is no active FreeRTOS task scheduling — `loop()` is the only execution context.

```
setup()
  filesystemInit()      // SPI + SD card + boot counter
  sensingInit()         // I2C + MS5607 + HDC2080
  dataloggingInit()     // Log file naming
  gnssInit()            // Load geofences + start Serial1

loop() [1 Hz]
  handleTelemetryGet()       // Poll sensors + GPS
  handleDatalogging()        // Write CSV if bridge closed
  handleGeofencing()         // Check geofences, trigger drop
  withinGlobalAltitudeDrop() // Failsafe altitude trigger
  [debug serial output]
  delay(1000ms)
```

**Why 1 Hz:** Sufficient temporal resolution for balloon flights, manageable log file sizes, and adequate geofence response time. Barometric and GPS sensors don't produce meaningful new data faster than this at balloon ascent rates.

### Data Flow

```
Serial1 (GPS) --> TinyGPS++ --> GnssData
I2C (MS5607/HDC2080) --> SensingData
                               ||
                               \/                    
                         handleGeofencing()
                         handleDatalogging() --> log_N.csv (SD)
```

---

## Module Reference

### `filesystem.cpp`

Owns the global `SdFs sd` object, which is shared across modules via `extern SdFs sd`. Initializes SPI manually before handing off to SdFat. This is required on SAMD21 to avoid race conditions during card enumeration.

SD initialization is done in two steps intentionally: `cardBegin()` probes the physical card, then `sd.begin()` mounts the filesystem. This gives separate error messages for card-level vs. filesystem-level failures, which is useful for debugging in the field.

**Key constants:**
```cpp
#define SD_CS_PIN 2
#define INIT_MHZ 1   // Conservative init speed. SdFat may renegotiate higher after mount
```

**Why SdFat over Arduino SD:** More robust, supports exFAT, better error reporting, and faster write throughput. The open/close-per-write pattern in `datalogging.cpp` relies on SdFat's file handle management.

**Why MISO pull-up:** Without it, reading the card normally fails. A floating MISO line can cause spurious card detection failures. This should improve signal integrity which means success is more consistent. Pull-ups on the MISO line are standard for SD card designs, but the EEs didn't wire one in.

**Boot counter:** Stored in `/.bootcount` as a plain decimal string. Read and incremented on every boot to produce unique log filenames automatically without any input.

---

### `gnss.cpp`

Reads NMEA sentences from `Serial1` at 9600 baud using TinyGPS++. GPS fields default to `0.0` when no fix is available — see [Known Issues](#known-issues-and-tech-debt) for the null island risk this creates.

**Geofence binary format:**

Geofences are loaded from `/geofences` as a raw binary blob. The on-disk layout is 5 packed `float`s (20 bytes per entry, little-endian on SAMD21):

```
[latitude][longitude][radiusMeters][minAltitudeMeters][maxAltitudeMeters]
```

This exactly matches `struct.pack("fffff", ...)` in `geofences_writer.py`. Maximum 32 geofences (640 bytes total).

**Why binary over JSON:** Faster to read on the M0, no parser overhead, and no dynamic memory allocation. The tradeoff is that the Python writer must stay in sync with the C struct layout — see [Adding a New Geofence Field](#adding-a-new-geofence-field) for the process.

**Distance calculation:** Uses TinyGPS++'s built-in functions for distance calculations. At the scales involved in balloon flights this is more than adequate.

**`isWithinGeofence()` return value:** Returns the geofence **index** (0-based) on match, or `-1` on no match — not a bool. The caller passes this index to `isWithinGeofenceAltitude()` to look up altitude bounds.

---

### `sensing.cpp`

**MS5607:** Accessed at I2C address `0x76` (low). `ms5607.doBaro(true)` blocks for ADC conversion. At 1 Hz this is fine; at higher loop rates, switch to async conversion (start conversion on one iteration, read on the next).

**HDC2080:** Configured for autonomous 1 Hz periodic measurement. `readHumidity()` and `readTemp()` return the most recent completed conversion without blocking.

**HDC2080 temperature offset:** `HDC2080_OFFSET_LSB = 90` represents a 14 C offset (14 / 0.16 is around 90 LSB). This constant is defined but **not applied** in the current read path — `hdc2080.readTemp()` returns the raw uncalibrated value. If you need calibrated HDC temperature, you need to add the offset correction explicitly:

```cpp
data.hdcTemperature = hdc2080.readTemp() - (HDC2080_OFFSET_LSB * 0.16f);
```

---

### `datalogging.cpp`

Opens and closes the log file on **every loop iteration**. This is intentional since it ensures no data is lost if power is cut mid-flight. At 1 Hz on a modern SD card this is fast enough. If you increase loop rate significantly, switch to periodic `flush()` calls instead of open/close cycling.

The `DROPPED` flag is global and sticky. Once `dataloggingSetDropped(true)` is called it remains true for the rest of the flight session. All subsequent log rows will show `T` in the Dropped column.

---

### `servo.cpp`

```cpp
static constexpr int SERVO_OPEN = 40;  // degrees
// servoClose() writes 0 degrees
```

**Note on the pin parameter:** `servoClose(pin)` and `servoOpen(pin)` both accept a pin argument, but it's unused — a single static `Servo` object is attached at `servoInit()` and that's what gets commanded. The pin parameter is a vestigial API. Don't rely on it for multi-servo designs without refactoring.

---

### `settings.cpp`

Fully implemented and correctly parses `settings.json` from the SD card. However, `loadSettings()` is **commented out in `main.cpp`**, and the `Settings` struct fields (`baroAltitudeTolerance`, `gpsAltitudeTolerance`) are unused in the drop logic. This is not a "future feature" it's implemented but disconnected for now until HAB wants thresholding.

---

### `geofences_writer.py`

Python 3 tool that writes geofence data to SD cards. Uses `struct.pack("fffff", ...)` to produce the exact binary layout the firmware expects.

**Platform detection:** Auto-discovers SD cards via removable drive enumeration on Windows and mount point scanning on Linux/macOS. Falls back to manual `--path` if detection fails.

---

## Drop Trigger Logic

### Primary Trigger: Geofence + Altitude

```cpp
int idx = isWithinGeofence(lat, lon);
if (idx >= 0) {
    if (isWithinGeofenceAltitude(idx, baroAltitude) ||
        isWithinGeofenceAltitude(idx, gpsAltitude)) {
        handleDrop();
    }
}
```

Both horizontal position and altitude must be satisfied simultaneously. The altitude check uses OR logic across both sensors. If either baro or GPS altitude is within the geofence's altitude range, the drop triggers. This provides redundancy if one altitude source fails.

**Geofences are evaluated in load order.** First match wins. Design your geofence file order intentionally if you have overlapping zones.

### Backup Trigger: Global Altitude Failsafe

```cpp
if (GLOBAL_ALTITUDE_DROP_ALLOWED &&
    (baroAltitude >= GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS ||
     gpsAltitude  >= GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS)) {
    handleDrop();
}
```

Triggers if the balloon exceeds 10,000m (default) regardless of geofence. Handles GPS loss, balloon drift outside all geofences, or unexpected ascent.

---

## Sensor Redundancy Strategy

| Source | Strengths | Weaknesses |
|---|---|---|
| MS5607 barometric | Stable, no signal dependency | Affected by pressure anomalies at altitude |
| GPS altitude | Independent of atmospheric pressure | Requires satellite fix; noisier than baro |

Both are checked with OR logic for drop decisions. The current implementation has a known issue — see [Known Issues](#known-issues-and-tech-debt).

---

## Pin Assignments

| Pin | Function | Direction | Notes |
|---|---|---|---|
| 2 | SD Card CS | Output | SPI chip select |
| 6 | Pull Bridge Output | Output | Drives bridge circuit |
| 7 | Pull Bridge Read | Input (pull-up) | Active low — LOW = bridge closed = logging enabled |
| 9 | Servo Control | Output | PWM via Arduino Servo library |
| Serial1 RX | GPS UART | Input | 9600 baud NMEA |
| SDA/SCL | I2C bus | Bidirectional | MS5607 @ 0x76, HDC2080 @ 0x40 |
| MISO/MOSI/SCK | SPI bus | Bidirectional | SD card, managed by SdFat |

---

## Libraries and Dependencies

| Library | Version | Purpose |
|---|---|---|
| MS56XX | ^1.1.1 | MS5607 barometer driver |
| Lime Labs HDC2080 | ^1.1.2 | Humidity/temperature sensor driver |
| TinyGPSPlus | ^1.1.0 | NMEA GPS parsing, Haversine distance |
| Adafruit SPIFlash | ^5.1.1 | SPI flash (transitive dependency) |
| ArduinoJson | ^7.4.2 | Settings file parsing |
| Servo | —  | Servo PWM control |
| SdFat | — | SD card filesystem (via Adafruit SPIFlash) |

---

## Build System

PlatformIO is used over Arduino IDE for reproducible builds, library dependency pinning, and CLI support.

```bash
# Build
make all # or pio run

# Flash
pio run --target upload

# Serial monitor
pio device monitor --baud 115200

# Fix MS56XX library conflict (run once after first build if needed)
make fix
```

`make fix` removes `main.cpp` from inside the MS56XX library package, which conflicts with the project's own entry point and causes a `multiple definition of 'main'` linker error.

---

## Future Considerations

1. **Settings integration:** Migrate `GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS` and tolerance values to `settings.json` and actually call `loadSettings()`. The infrastructure is already there.
2. **Watchdog timer:** No WDT implementation. A hung I2C transaction or SD write could lock the system indefinitely with no recovery.
3. **FreeRTOS tasks:** If loop rate needs to increase, move sensor polling to a dedicated task so GPS serial bytes aren't dropped during SD writes.
5. **Sensor failure detection:** The OR logic on altitude sources handles single-sensor failure silently. Explicit failure detection with a logged fault flag would make post-flight analysis more useful.

---

## Adding a New Geofence Field

1. Update `struct Geofence` in `include/types.h`
2. Update `GEOFENCE_STRUCT` in `geofences_writer.py` to match (must be same field order, same types, packed)
3. Update JSON parsing in `load_geofences_from_json()` in `geofences_writer.py`
4. Update the serial debug print in `gnssInit()`
5. Add any new trigger logic as needed

**Watch for struct padding.** The current struct is all `float` — no padding is inserted. If you add a non-float field (e.g., `uint8_t flags`), the C compiler may pad it to 4-byte alignment, which won't match the Python struct output. Use `__attribute__((packed))` on the struct or keep to uniform 4-byte types.

---
