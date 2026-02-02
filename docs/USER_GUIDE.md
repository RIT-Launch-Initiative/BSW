# BSW (Balloon Software) - User Guide

## Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Setup](#software-setup)
4. [Preparing for Flight](#preparing-for-flight)
5. [Geofence Configuration](#geofence-configuration)
6. [Operating the System](#operating-the-system)
7. [Data Retrieval and Analysis](#data-retrieval-and-analysis)
8. [Troubleshooting](#troubleshooting)

## Overview

The Balloon Software (BSW) is an autonomous payload management system for high-altitude balloon flights. It automatically logs environmental data, tracks GPS position, and triggers payload release based on configurable geofences and altitude thresholds.

![System Overview Diagram]()

## Hardware Requirements

### Required Components

- **Adafruit ItsyBitsy M0** microcontroller
- **MS5607** barometric pressure/temperature sensor (I2C)
- **HDC2080** humidity/temperature sensor (I2C)
- **GPS module** with UART output (9600 baud, NMEA format)
- **Servo motor** for payload release mechanism
- **SD card** (formatted FAT32) for data logging
- **Pull bridge circuit** (optional, for logging control)

### Wiring Connections

| Component | Connection | ItsyBitsy Pin |
|-----------|------------|---------------|
| SD Card CS | SPI CS | Pin 2 |
| MS5607 | I2C (SDA/SCL) | I2C pins |
| HDC2080 | I2C (SDA/SCL) | I2C pins |
| GPS Module | UART TX | Serial1 RX |
| Servo | Signal | Pin 9 |
| Pull Bridge Output | Digital Out | Pin 6 |
| Pull Bridge Input | Digital In | Pin 7 |

![Wiring Diagram]()

**Important Notes**:
- MS5607 I2C address: 0x76 (low)
- HDC2080 I2C address: 0x40
- GPS baud rate: 9600
- Power all components with appropriate voltage levels (3.3V for ItsyBitsy M0)

## Software Setup

### Installing PlatformIO

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the PlatformIO IDE extension from the VS Code marketplace
3. Restart VS Code

### Building the Project

**Option 1: Using Make**
```bash
cd /path/to/BSW
make all
```

**Option 2: Using PlatformIO CLI**
```bash
pio run
```

**Option 3: Using VS Code**
- Open the project folder in VS Code
- Click the PlatformIO icon in the sidebar
- Select "Build" under PROJECT TASKS

### Uploading to Board

1. Connect the ItsyBitsy M0 to your computer via USB
2. Put the board in bootloader mode (double-tap reset button - red LED should pulse)
3. Upload using one of these methods:

**Via PlatformIO**:
```bash
pio run --target upload
```

**Via VS Code**: Click "Upload" in PlatformIO PROJECT TASKS

### Common Build Issues

**MS56XX Library Issue**: If build fails due to MS56XX library, run:
```bash
make fix
```
This removes the problematic `main.cpp` file from the MS56XX library.

## Preparing for Flight

### Step 1: Format SD Card

1. Format a micro SD card as **FAT32**
2. Ensure the card is empty or contains only geofence data

### Step 2: Configure Geofences

Geofences define geographic boundaries where the payload should be released. You can configure them using the included Python tool.

#### Using the Geofence Writer Tool

**Prerequisites**:
- Python 3.x installed
- SD card connected to your computer

**Interactive Mode** (enter geofences manually):
```bash
python geofences_writer.py
```

The tool will:
1. Auto-detect your SD card location
2. Prompt you to enter geofence parameters:
   - Latitude (decimal degrees)
   - Longitude (decimal degrees)
   - Radius (meters)
   - Minimum altitude (meters)
   - Maximum altitude (meters)
3. Press Ctrl+D (Linux/Mac) or Ctrl+Z (Windows) when finished
4. Confirm and write to SD card

**JSON Mode** (load from file):
```bash
python geofences_writer.py --json geofences_example.json
```

**Specify SD Card Path**:
```bash
python geofences_writer.py --json geofences.json --path /media/sdcard
```

#### JSON Geofence Format

Create a file `geofences.json`:

```json
{
  "geofences": [
    {
      "latitude": 43.084,
      "longitude": -77.676,
      "radiusMeters": 1000,
      "minAltitudeMeters": 0,
      "maxAltitudeMeters": 100
    },
    {
      "latitude": 43.085,
      "longitude": -77.677,
      "radiusMeters": 500,
      "minAltitudeMeters": 50,
      "maxAltitudeMeters": 150
    }
  ]
}
```

**Field Descriptions**:
- `latitude`: Center point latitude in decimal degrees (positive = North, negative = South)
- `longitude`: Center point longitude in decimal degrees (positive = East, negative = West)
- `radiusMeters`: Horizontal trigger radius in meters
- `minAltitudeMeters`: Minimum altitude (MSL) for payload release (optional, defaults to 0)
- `maxAltitudeMeters`: Maximum altitude (MSL) for payload release (optional, defaults to 0)

**Important**: If altitude fields are 0 or omitted, altitude checking is effectively disabled for that geofence.

### Step 3: Insert SD Card

1. Safely eject SD card from computer
2. Insert into the SD card slot on your payload electronics
3. Ensure card is fully seated and latched

### Step 4: Pre-Flight Testing

1. Connect the ItsyBitsy M0 to a computer via USB
2. Open a serial monitor (115200 baud)
3. Power on or reset the board
4. Verify initialization messages:

```
=============
 Filesystem 
=============
	SPI initialized
	Card initialized
	Card size: XXX MB
	FS mounted

=============
    GNSS    
=============
	Loaded 2 geofences
	Geofence 0: Lat 43.084000, Lon -77.676000, Radius 1000.00 m, Alt 0.00-100.00 m
	Geofence 1: Lat 43.085000, Lon -77.677000, Radius 500.00 m, Alt 50.00-150.00 m
	GNSS initialized

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
```

5. Check that sensor data is being read correctly
6. Verify GPS acquires a fix (may take several minutes outdoors)

## Operating the System

### Normal Operation Sequence

1. **Pre-flight**:
   - Power on the system
   - Wait for GPS fix (LED may blink during acquisition)
   - Verify serial output shows valid sensor readings
   - Close servo to secure payload

2. **Launch**:
   - Enable datalogging by bridging pins 6 and 7 (pull bridge closed)
   - System will begin logging to SD card
   - Serial output: `BRIDGED (logging enabled)`

3. **Flight**:
   - System continuously monitors:
     - GPS position vs. geofences
     - Altitude (barometric and GPS)
     - Environmental sensors
   - Data is logged every 1 second

4. **Payload Release**:
   - System triggers when **BOTH** conditions are met:
     - GPS position is within a geofence radius
     - Altitude is within that geofence's altitude range
   - Servo opens to release payload
   - LED blinks to indicate drop
   - Log file marks drop event with 'T' in Dropped column

5. **Backup Release**:
   - If geofence trigger fails, global altitude backup activates
   - Default threshold: 10,000 meters
   - Ensures payload release even if GPS is lost

6. **Post-Flight Recovery**
   - Before powering down the ItsyBitsy M0, remove the bridge that enables SD card logging
     - This prevents the SD card from potentially becoming corrupted if a write is attempted upon power loss
   - It is now safe to power down the ItsyBitsy M0

### Pull Bridge Operation

The pull bridge controls datalogging:

- **Bridge CLOSED** (pins 6-7 connected): Logging ENABLED
- **Bridge OPEN** (pins 6-7 disconnected): Logging DISABLED

This allows external control (e.g., mechanical switch) to start/stop logging without reprogramming.

### Debug Mode

Set `DEBUG = true` in [main.cpp](../src/main.cpp#L12) for verbose serial output:

```cpp
bool DEBUG = true;
```

This enables:
- Sensor readings printed every second
- GPS data output
- Geofence check status
- Logging state changes

**Disable for flight** to reduce power consumption and improve reliability.

## Data Retrieval and Analysis

### Retrieving Log Files

1. Power down the system
2. Remove SD card
3. Connect to computer
4. Copy `log_N.csv` files (where N is the boot count)

### Log File Format

CSV format with the following columns:

| Column | Description | Units |
|--------|-------------|-------|
| UptimeMillis | Time since boot | milliseconds |
| Time | GPS time (UTC) | HH:MM:SS |
| Latitude | GPS latitude | decimal degrees |
| Longitude | GPS longitude | decimal degrees |
| GPSAltitude | GPS altitude | meters MSL |
| BaroAltitude | Barometric altitude | meters MSL |
| Pressure | Atmospheric pressure | mbar |
| Temperature | MS5607 temperature | °C |
| Humidity | Relative humidity | % |
| Dropped | Payload release flag | T/F |

### Analyzing Data

**Excel/Google Sheets**:
1. Open CSV file
2. Data is already properly formatted
3. Create graphs of altitude vs. time, temperature profiles, etc.

**Python Analysis Example**:
```python
import pandas as pd
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv('log_1.csv')

# Plot altitude over time
plt.figure(figsize=(12, 6))
plt.plot(df['UptimeMillis']/1000/60, df['BaroAltitude'], label='Barometric')
plt.plot(df['UptimeMillis']/1000/60, df['GPSAltitude'], label='GPS')
plt.xlabel('Time (minutes)')
plt.ylabel('Altitude (m)')
plt.legend()
plt.grid(True)
plt.show()

# Find drop event
drop_time = df[df['Dropped'] == 'T']['UptimeMillis'].iloc[0] / 1000 / 60
print(f"Payload dropped at {drop_time:.2f} minutes")
```

**Mapping Trajectory**:
Use Latitude/Longitude columns with tools like:
- Google Earth (import CSV)
- GPS Visualizer (https://www.gpsvisualizer.com/)
- Python libraries (folium, plotly)

## Troubleshooting

### SD Card Not Detected

**Symptoms**: Error messages during filesystem initialization

**Solutions**:
- Verify SD card is FAT32 formatted
- Try a different SD card (some cards are incompatible)
- Check CS pin connection (pin 2)
- Ensure SD card is fully inserted
- Try reseating the card

### GPS Not Getting Fix

**Symptoms**: GPS coordinates show 0.0, or "GPS not initialized"

**Solutions**:
- GPS requires clear view of sky
- Cold start can take 5-15 minutes
- Verify GPS module is powered (3.3V)
- Check UART connection (Serial1)
- Confirm GPS baud rate is 9600

### Sensors Reading Zero or Invalid Data

**Symptoms**: Temperature/pressure/humidity show 0 or unrealistic values

**Solutions**:
- Check I2C connections (SDA/SCL)
- Verify sensor addresses (MS5607: 0x76, HDC2080: 0x40)
- Run I2C scanner to detect devices
- Ensure sensors are powered correctly
- Try replacing sensor module

### Servo Not Moving

**Symptoms**: Servo doesn't open/close during testing

**Solutions**:
- Check servo power supply (may need separate 5V source)
- Verify signal wire connected to pin 9
- Test servo separately with Arduino servo example
- Ensure servo isn't mechanically jammed

### Geofences Not Loading

**Symptoms**: "Loaded 0 geofences" message

**Solutions**:
- Verify `geofences` file exists on SD card (no extension)
- Re-run `geofences_writer.py` to regenerate file
- Check SD card is readable
- Ensure SD card was properly ejected after writing geofences

### Payload Not Dropping in Geofence

**Symptoms**: Balloon enters geofence but servo doesn't trigger

**Solutions**:
- Check altitude is within geofence's altitude range
- Verify GPS has valid fix (lat/lon not 0.0)
- Enable DEBUG mode to see geofence check status
- Check servo is functional
- Verify geofence coordinates are correct

### Log Files Empty or Missing

**Symptoms**: No CSV file created or file has only headers

**Solutions**:
- Verify pull bridge is closed (logging enabled)
- Check serial output for "BRIDGED (logging enabled)"
- Ensure SD card has free space
- Verify filesystem initialized successfully
- Check SD card isn't write-protected

### System Keeps Rebooting

**Symptoms**: Continuous resets, increasing boot count

**Solutions**:
- Check power supply stability (brown-out detection)
- Verify sufficient current capacity for all components
- Look for short circuits
- Test with minimal configuration (no sensors/GPS)

## Advanced Configuration

### Adjusting Drop Altitude Threshold

Edit [main.cpp](../src/main.cpp#L13-L14):

```cpp
bool GLOBAL_ALTITUDE_DROP_ALLOWED = true;  // Enable/disable backup trigger
int GLOBAL_ALTITUDE_DROP_THRESHOLD_METERS = 10000;  // Altitude in meters
```

**Note**: Planning to migrate this to settings.json in future versions.

### Custom Pin Assignments

Edit pin definitions in [main.cpp](../src/main.cpp#L18-L27):

```cpp
#define PULL_BRIDGE_OUTPUT_PIN 6
#define PULL_BRIDGE_READ_PIN 7
#define SERVO_CONTROL_PIN 9
```

Edit SD CS pin in [filesystem.cpp](../src/filesystem.cpp#L11):

```cpp
#define SD_CS_PIN 2
```
