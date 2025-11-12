# Balloon Software
Software that supports RIT Launch Initiative's High Altitude Balloon team

## Geofence Writer Tool

The `geofences_writer.py` script allows you to write geofence data to an SD card in the binary format expected by the balloon software.

### Usage

**Interactive Mode:**
```bash
python geofences_writer.py
```

**JSON Mode:**
```bash
python geofences_writer.py --json geofences.json
```

**Specify SD Card Path:**
```bash
python geofences_writer.py --json geofences.json --path /media/sdcard
```

### JSON Format

See `geofences_example.json` for the expected format:

```json
{
  "geofences": [
    {
      "latitude": 43.084,
      "longitude": -77.676,
      "radiusMeters": 1000
    },
    {
      "latitude": 43.085,
      "longitude": -77.677,
      "radiusMeters": 500
    }
  ]
}
```

### Platform Support

The script supports automatic SD card detection on:
- **Windows**: Detects removable drives (D:, E:, etc.)
- **Linux**: Checks `/media` and `/mnt` mount points
- **macOS**: Checks `/Volumes` mount points

You can also manually specify the path using the `--path` option.
