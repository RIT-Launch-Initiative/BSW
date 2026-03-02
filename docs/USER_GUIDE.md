# BSW (Balloon Software) — User Guide

## Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Setup](#software-setup)
4. [Preparing for Flight](#preparing-for-flight)
5. [Geofence Configuration](#geofence-configuration)
6. [Operating the System](#operating-the-system)
7. [Data Retrieval and Analysis](#data-retrieval-and-analysis)
8. [Troubleshooting](#troubleshooting)

---

## Overview

Balloon Software (BSW) supports firmware projects for the HAB team. The firmware automatically logs environmental data, tracks GPS position, and triggers payload release based on configurable geofences and altitude thresholds.
The main loop runs at 1 Hz and handles four things in order: sensor polling, SD card logging, geofence checking, and a global altitude failsafe.

---

## Hardware Requirements

### Required Components

- **Adafruit ItsyBitsy M0** microcontroller (ATSAMD21G18A, 3.3V logic)
- **MS5607** barometric pressure/temperature sensor (I2C, address 0x76)
- **HDC2080** humidity/temperature sensor (I2C, address 0x40)
- **GPS module** with UART output (NMEA format, 9600 baud)
- **Servo motor** for payload release
- **MicroSD card** (FAT32 formatted)
- **Pull bridge circuit** for logging enable/disable

### Wiring Connections

| Component | ItsyBitsy Pin | Notes |
|---|---|---|
| SD Card CS | Pin 2 | SPI bus |
| MS5607 | SDA/SCL | I2C, address 0x76 |
| HDC2080 | SDA/SCL | I2C, address 0x40 |
| GPS Module TX | Serial1 RX | 9600 baud |
| Servo Signal | Pin 9 | |
| Pull Bridge Output | Pin 6 | Digital out |
| Pull Bridge Input | Pin 7 | Digital in, internal pull-up |

---

## Software Setup

### Installing PlatformIO

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the PlatformIO IDE extension from the VS Code marketplace
3. Restart VS Code

### Building the Project

```bash
# Via Make
make all

# Via PlatformIO CLI
pio run
```

Or use the PlatformIO sidebar in VS Code and click **Build** under PROJECT TASKS.

### Uploading to the Board

1. Connect the ItsyBitsy M0 via USB
2. Double-tap the reset button — the red LED should pulse, indicating bootloader mode
3. Upload:

```bash
pio run --target upload
```

Or click **Upload** in the PlatformIO PROJECT TASKS sidebar.

### Common Build Issue: MS56XX Library

If the build fails with a `multiple definition of 'main'` error, run:

```bash
make fix
```

This removes an erroneous `main.cpp` that ships inside the MS56XX library and conflicts with the project's own entry point. Run it once after the first build.

---

## Preparing for Flight

### Step 1: Format the SD Card

Format a microSD card as **FAT32**. The card is read once on boot — any changes made after power-on are ignored until the next reboot.

### Step 2: Configure Geofences

See [Geofence Configuration](#geofence-configuration) below.

### Step 3: Insert the SD Card

Insert the card fully and confirm it is latched. A partially seated card will fail silently or produce intermittent errors.

### Step 4: Pre-Flight Verification

Connect to a serial monitor at **115200 baud**, then power on or reset the board. You should see:

```
=============
 Filesystem 
=============
	SPI initialized
	Card initialized
	Card size: XXX MB
	FS mounted

=============
   Sensors   
=============
	MS5607 initialized
	HDC2080 initialized

=============
 Datalogging 
=============
	Boot count: 1
	Log file: log_1.csv

=============
    GNSS    
=============
	Loaded 2 geofences
	Geofence 0: Lat 43.084000, Lon -77.676000, Radius 1000.00 m, Alt 0.00-100.00 m
	Geofence 1: Lat 43.085000, Lon -77.677000, Radius 500.00 m, Alt 50.00-150.00 m
	GNSS initialized
```

### Pre-Flight Checklist

- [ ] SD card formatted FAT32 and seated
- [ ] Geofence file written and geofence count matches expectation in serial output
- [ ] Geofence altitude bounds are set to realistic values (see warning below)
- [ ] Boot count incremented — confirms SD card is readable and writable
- [ ] Barometric altitude reading is reasonable for your ground elevation
- [ ] Sensor readings are non-zero and plausible
- [ ] GPS fix acquired (may take 5–15 minutes outdoors with clear sky view)
- [ ] Servo moves to closed position on boot
- [ ] Pull bridge is **open** before launch
- [ ] `DEBUG` is set to `false` in `main.cpp`

> **⚠️ Altitude bounds warning:** If `minAltitudeMeters` and `maxAltitudeMeters` are both `0` (the default when omitted), the geofence altitude check will never be satisfied during flight because the balloon will not be at 0m altitude while over the target zone. **Always set realistic altitude bounds.** Failing to do so means the payload will not drop even if the balloon is directly over the target.

---

## Geofence Configuration

Geofences define the geographic area and altitude band in which payload release is triggered. Up to 32 geofences can be loaded. They are checked in order — the first match wins.

### Using the Geofence Writer Tool

The `geofences_writer.py` script writes geofence data to the SD card in the binary format the firmware expects. Requires Python 3.

**JSON mode (recommended for flight):**
```bash
python3 geofences_writer.py --json geofences.json --path /media/sdcard
```

**Interactive mode:**
```bash
python3 geofences_writer.py
```

The tool auto-detects SD cards on Windows, Linux, and macOS. If detection fails, pass the path explicitly with `--path`.

### JSON Format

```json
{
  "geofences": [
    {
      "latitude": 43.084,
      "longitude": -77.676,
      "radiusMeters": 1000,
      "minAltitudeMeters": 500,
      "maxAltitudeMeters": 5000
    }
  ]
}
```

| Field | Description |
|---|---|
| `latitude` | Center latitude, decimal degrees (positive = North) |
| `longitude` | Center longitude, decimal degrees (positive = East) |
| `radiusMeters` | Horizontal trigger radius in meters |
| `minAltitudeMeters` | Minimum altitude MSL for drop trigger (defaults to `0` if omitted) |
| `maxAltitudeMeters` | Maximum altitude MSL for drop trigger (defaults to `0` if omitted) |

### Drop Trigger Logic

A drop is triggered when **both** of the following are true simultaneously:

1. GPS position is within the geofence's horizontal radius
2. Altitude (barometric **or** GPS) is within the geofence's altitude range

Both altitude sources are checked — if either is within range, the drop triggers. This provides redundancy if one altitude source is unreliable.

### Global Altitude Failsafe

Independently of geofencing, a drop is triggered if the balloon exceeds `GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS` (default: 10,000m) anywhere. This is a failsafe for cases where GPS is lost or the balloon drifts outside all geofences. Do not change this value without consulting the software team.

---

## Operating the System

### Normal Operation Sequence

**Before launch:**

1. Power on the system
2. Verify serial output — all subsystems initialized, geofences loaded, sensor readings valid
3. Wait for GPS fix outdoors (5–15 minutes cold start)
4. Confirm servo is in the closed position
5. Keep the pull bridge **open** — logging is disabled until you intentionally enable it

**At launch:**

- Close the pull bridge (connect pins 6 and 7) to enable datalogging
- Serial output will confirm: `BRIDGED (logging enabled)`

**During flight:**

- System logs all sensor and GPS data once per second
- Geofence checks run continuously
- No operator action required

**Payload release:**

- Servo opens automatically when drop conditions are met
- LED blinks to indicate drop
- `Dropped` column in the log switches from `F` to `T`

**Recovery:**

1. **Before powering down**, open the pull bridge to disable logging
   - This ensures any in-progress SD write is completed cleanly before power is removed
   - Removing power mid-write can corrupt the log file or the filesystem
2. Power down the system
3. Remove the SD card

### Pull Bridge Reference

| Bridge State | Pins 6–7 | Logging |
|---|---|---|
| Closed | Connected | **Enabled** |
| Open | Disconnected | **Disabled** |

### Debug Mode

`DEBUG = true` in `main.cpp` enables verbose 1 Hz serial output including sensor readings, GPS position, and geofence check status. It is useful for pre-flight verification but **should be set to `false` before flight** to reduce overhead.

---

## Data Retrieval and Analysis

### Retrieving Log Files

1. Power down and remove SD card
2. Connect to computer
3. Copy `log_N.csv` where N matches the boot count shown during initialization

Multiple flights will produce multiple numbered files — they are never overwritten.

### Log File Format

| Column | Description | Units |
|---|---|---|
| UptimeMillis | Time since boot | ms |
| Time | GPS time (UTC) | HH:MM:SS |
| Latitude | GPS latitude | decimal degrees |
| Longitude | GPS longitude | decimal degrees |
| GPSAltitude | GPS altitude MSL | meters |
| BaroAltitude | Barometric altitude MSL | meters |
| Pressure | Atmospheric pressure | mbar |
| Temperature | MS5607 temperature | °C |
| Humidity | Relative humidity | % |
| Dropped | Payload release flag | T/F |

### Analyzing Data

**Python example:**
```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('log_1.csv')

# Altitude over time
plt.figure(figsize=(12, 6))
plt.plot(df['UptimeMillis']/60000, df['BaroAltitude'], label='Barometric')
plt.plot(df['UptimeMillis']/60000, df['GPSAltitude'], label='GPS')
plt.xlabel('Time (minutes)')
plt.ylabel('Altitude (m)')
plt.legend()
plt.grid(True)
plt.show()

# Drop event time
drop_rows = df[df['Dropped'] == 'T']
if not drop_rows.empty:
    drop_time = drop_rows['UptimeMillis'].iloc[0] / 60000
    print(f"Payload dropped at {drop_time:.2f} minutes")
```

**Mapping trajectory:**
- [GPS Visualizer](https://www.gpsvisualizer.com/) — import CSV directly
- Google Earth — accepts CSV with lat/lon columns
- Python: `folium` or `plotly` for interactive maps

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| `cardBegin failed` on serial | Bad card, wrong format, or SPI wiring issue | Reformat as FAT32; try a different card; check pin 2 |
| `FS mounted` not shown | Filesystem corruption | Reformat card |
| Boot count not incrementing | SD card is read-only | Check write-protect switch on card |
| Geofence count = 0 | Missing or corrupt `geofences` file | Re-run `geofences_writer.py` |
| GPS lat/lon = 0.0 | No fix yet | Move outdoors; wait 5–15 min; check antenna |
| Sensors reading 0 or garbage | I2C wiring issue or wrong address | Check SDA/SCL; run I2C scanner; verify 3.3V power |
| Servo doesn't move | No drop condition met, or power issue | Check geofence bounds; verify servo has adequate power supply |
| Payload doesn't drop in geofence | Altitude bounds set to 0, or no GPS fix | Set realistic altitude bounds; verify GPS fix before launch |
| No log file created | Pull bridge not closed, or SD issue | Check bridge; verify `BRIDGED (logging enabled)` in serial output |
| Log file has only header row | Bridge opened immediately after closing | Keep bridge closed for at least one loop cycle (1 second) |
| System continuously rebooting | Power supply brownout | Check current capacity; verify all components powered correctly |
| Build fails: multiple definition of `main` | MS56XX library ships its own `main.cpp` | Run `make fix` once |