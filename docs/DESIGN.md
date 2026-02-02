# BSW (Balloon Software) - Design Documentation

## Overview

TODO

## Hardware Platform

- **Microcontroller**: Adafruit ItsyBitsy M0 (ATSAMD21G18)
- **Architecture**: ARM Cortex-M0+ @ 48MHz
- **Framework**: Arduino with PlatformIO
- **Operating System**: FreeRTOS (SAMD21 port)

## System Architecture

### Main Control Loop

The system operates on a simple sequential execution model (see [main.cpp](../src/main.cpp#L155-L168)):

```
loop() {
    handleTelemetryGet()    → Collect sensor and GPS data
    handleDatalogging()     → Write data to SD card
    handleGeofencing()      → Check location and trigger release
    (altitude check)        → Global altitude-based release backup
    (debug output)          → Serial monitoring
    delay(1000ms)           → 1Hz update rate
}
```

**Rationale**: A 1Hz update rate was chosen for the following reasons:
- Sufficient temporal resolution for balloon flights
- Manageable log file sizes
- Adequate response time for geofence triggering

## File Structure and Responsibilities

### Source Files (`src/`)

#### main.cpp
**Purpose**: Application entry point and main control logic

**Key Responsibilities**:
- System initialization sequence
- Main control loop orchestration
- Hardware pin configuration
- Pull bridge monitoring for datalogging enable/disable
- Dual-condition drop logic (geofence + altitude)

**Design Decisions**:
- Pull bridge mechanism (pins 6/7) allows external control of logging without firmware modification
- LED feedback provides visual indication of system state
- `DEBUG` flag enables/disables serial output for development vs. flight operation

#### gnss.cpp/gnss.h
**Purpose**: GPS/GNSS data acquisition and geofence management

**Key Responsibilities**:
- GNSS serial communication (9600 baud on Serial1)
- NMEA sentence parsing via TinyGPS++ library
- Geofence binary data loading from SD card
- Distance calculation between current position and geofence centers
- Altitude range checking for geofences

**Design Decisions**:
- **Binary geofence storage**: More compact and faster to read than JSON on embedded systems
- **Maximum 32 geofences**: Balances flexibility with memory constraints (640 bytes total)
- **TinyGPS++ library**: Lightweight, well-tested NMEA parser suitable for resource-constrained systems
- **Distance algorithm**: Uses TinyGPS++`s built-in Haversine formula for accurate great-circle distance

**Geofence Structure**:
```cpp
struct Geofence {
    float latitude;           // Center point latitude (degrees)
    float longitude;          // Center point longitude (degrees)
    float radiusMeters;       // Horizontal trigger radius
    float minAltitudeMeters;  // Minimum altitude for trigger
    float maxAltitudeMeters;  // Maximum altitude for trigger
}
```

#### sensing.cpp/sensing.h
**Purpose**: Environmental sensor data collection

**Sensors Integrated**:
- **MS5607**: High-precision barometric pressure/temperature sensor
  - Pressure measurement → Altitude calculation
  - Temperature compensation
- **HDC2080**: Humidity and temperature sensor
  - Configured for 14-bit resolution
  - 1Hz automatic measurement mode

**Design Decisions**:
- **I2C communication**: Industry-standard protocol, minimal wiring
- **Barometric altitude**: More reliable than GPS altitude for payload release decisions
- **Dual temperature sensors**: Provides redundancy and different measurement locations
- **HDC2080 offset calibration**: 14°C offset compensated via `HDC2080_OFFSET_LSB`

#### servo.cpp/servo.h
**Purpose**: Servo motor control for payload release mechanism

**Key Responsibilities**:
- Servo initialization and attachment to control pin (pin 9)
- Open position (40°) and closed position (0°) control

**Design Decisions**:
- **40° maximum opening**: Sufficient for release mechanism while preventing over-rotation
- **Simple position control**: Binary open/closed states (no intermediate positions needed)
- **Standard Arduino Servo library**: Well-tested, minimal overhead

#### datalogging.cpp/datalogging.h
**Purpose**: CSV data logging to SD card

**Key Responsibilities**:
- Boot-indexed log file creation (`log_N.csv`)
- CSV header generation for new files
- Real-time data appending
- File handle management (open/close based on pull bridge)

**Log File Format**:
```
UptimeMillis,Time,Latitude,Longitude,GPSAltitude,BaroAltitude,Pressure,Temperature,Humidity,Dropped
```

**Design Decisions**:
- **CSV format**: Human-readable, easy to import into analysis tools
- **Boot-indexed filenames**: Prevents data overwriting between flights
- **Explicit file closing**: Ensures data integrity if power is lost
- **Pull bridge integration**: Physical switch enables logging only when desired
- **Dropped flag**: Marks exact moment of payload release in log data

#### filesystem.cpp/filesystem.h
**Purpose**: SD card initialization and file I/O utilities

**Key Responsibilities**:
- SPI bus initialization with proper timing
- SD card detection and mounting
- Boot counter persistence
- Generic file read/write utilities

**Design Decisions**:
- **SdFat library**: More robust and feature-rich than Arduino SD library
- **1MHz initialization**: Conservative clock speed for reliability
- **MISO pull-up**: Improves signal integrity on noisy power systems
- **Boot counter**: Automatic log file indexing without external intervention
- **Hidden .bootcount file**: Tracks reboots across power cycles

#### settings.cpp/settings.h
**Purpose**: JSON-based configuration loading (future feature)

**Design Note**: Currently commented out in main.cpp but implemented for future use. Allows non-programmer configuration of system parameters.

**Configuration Schema**:
```json
{
  "logAltitudeThresholdMeters": 0,
  "baroAltitudeTolerance": 5,
  "gpsAltitudeTolerance": 5
}
```

### Header Files (`include/`)

#### types.h
**Purpose**: Shared data structure definitions

**Design Rationale**: Centralized type definitions prevent circular dependencies and ensure consistent data layouts across modules.

### Supporting Tools

#### geofences_writer.py
**Purpose**: Cross-platform tool for writing geofence data to SD cards

**Key Features**:
- Interactive geofence entry mode
- JSON batch import mode
- Automatic SD card detection (Windows/Linux/macOS)
- Binary struct packing matching embedded format

**Design Decisions**:
- **Python implementation**: Cross-platform, no compilation needed
- **Struct packing**: Ensures byte-for-byte compatibility with embedded C structs
- **Platform detection**: Automatic SD card discovery improves user experience
- **JSON support**: Allows version control and reusable geofence definitions

## Drop Trigger Logic

The system implements **dual-condition triggering** for redundancy:

### Primary Trigger: Geofence + Altitude
```cpp
if (isWithinGeofence() && isWithinGeofenceAltitude()) {
    handleDrop();
}
```

**Logic**:
1. Check if current GPS position is within any geofence's horizontal radius
2. If yes, check if current altitude (GPS OR barometric) is within that geofence's altitude range
3. Both conditions must be true to trigger

**Rationale**: Prevents premature drops due to GPS drift or altitude sensor errors by requiring both horizontal and vertical position confirmation.

### Backup Trigger: Global Altitude Drop
```cpp
if (altitude >= GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS) {
    handleDrop();
}
```

**Configuration**:
- Default threshold: 10,000 meters
- Can be enabled/disabled via `GLOBAL_ALTITUDE_DROP_ALLOWED`

**Rationale**: Safety backup in case:
- GPS signal is lost (no geofence checking possible)
- Balloon drifts outside all defined geofences
- Balloon ascends higher than expected

**Comment from code**: "Make this a config eventually" - indicates future migration to settings.json

## Sensor Redundancy Strategy

The system uses **dual altitude sources** for reliability:

1. **Barometric altitude** (MS5607): More stable, less susceptible to GPS signal issues
2. **GPS altitude**: Independent measurement, immune to pressure sensor failures

**Drop trigger logic** (from [main.cpp](../src/main.cpp#L141-L147)):
```cpp
if (isWithinGeofenceAltitude(index, baroAltitude) ||
    isWithinGeofenceAltitude(index, gpsAltitude)) {
    handleDrop();
}
```

**Rationale**: OR logic ensures system functions even if one sensor fails. Comment in code acknowledges this needs improvement: "Fix before next launch because we don't know if sensors or GPS will fail LOL"

## Pin Assignments

| Pin | Function | Direction | Notes |
|-----|----------|-----------|-------|
| 2 | SD Card CS | Output | SPI chip select |
| 6 | Pull Bridge Output | Output | Drives pull bridge circuit |
| 7 | Pull Bridge Read | Input (Pullup) | Detects bridge state (active low) |
| 9 | Servo Control | Output | PWM servo signal |
| Serial1 | GPS UART | Input | 9600 baud NMEA |
| I2C (SDA/SCL) | Sensors | Bidirectional | MS5607 (0x76), HDC2080 (0x40) |
| SPI (MISO/MOSI/SCK) | SD Card | Bidirectional | SdFat library managed |

## Libraries and Dependencies

| Library | Purpose | Rationale |
|---------|---------|-----------|
| FreeRTOS_SAMD21 | RTOS framework | Structured task management (currently underutilized, foundation for future expansion) |
| MS56XX | MS5607 driver | Proven barometric sensor library |
| HDC2080 | Humidity sensor driver | Simple I2C interface |
| TinyGPSPlus | NMEA parsing | Lightweight, reliable GPS parsing |
| Adafruit SPIFlash | Flash memory | Dependency of SdFat |
| ArduinoJson | JSON parsing | Settings file parsing |
| Servo | Servo control | Standard Arduino library |
| SdFat | SD card file system | More robust than Arduino SD |

## Build System

**PlatformIO** was chosen over Arduino IDE for:
- Library dependency management
- Reproducible builds
- Command-line build support
- Better toolchain integration

**Makefile**: Provides convenient wrapper around PlatformIO commands:
- `make all`: Build project
- `make fix`: Removes problematic main.cpp from MS56XX library (known issue with that library)

## Future Design Considerations

Based on code comments and structure:

1. **Settings integration**: Migrate hardcoded values to settings.json
2. **Sensor failure handling**: Improve dual-sensor logic with formal failure detection
3. **FreeRTOS tasks**: Currently unused task framework suggests future multi-tasking architecture
4. **Watchdog timer**: No current implementation; would improve reliability
5. **Power management**: No sleep modes currently implemented

## Debug and Development Features

- **DEBUG flag**: Enables verbose serial output
- **Serial monitoring**: 115200 baud for fast data streaming
- **LED feedback**: Built-in LED indicates system state
- **CSV logging**: Complete telemetry record for post-flight analysis

---

*This design documentation reflects the current implementation. For usage instructions, see [USER_GUIDE.md](USER_GUIDE.md).*
