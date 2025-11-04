import os
import sys
import struct
import json
import argparse
import platform

GEOFENCE_STRUCT = "fff"  # latitude, longitude, radiusMeters
GEOFENCE_SIZE = struct.calcsize(GEOFENCE_STRUCT)
GEOFENCE_FILENAME = "geofences"

def find_sd_card():
    candidates = []
    system = platform.system()
    
    if system == "Windows":
        # Windows: Check for removable drives
        import string
        from ctypes import windll
        
        # Get available drives
        drives = []
        bitmask = windll.kernel32.GetLogicalDrives()
        for letter in string.ascii_uppercase:
            if bitmask & 1:
                drives.append(f"{letter}:")
            bitmask >>= 1
        
        # Filter for removable drives (type 2)
        for drive in drives:
            try:
                drive_type = windll.kernel32.GetDriveTypeW(f"{drive}\\")
                if drive_type == 2:  # DRIVE_REMOVABLE
                    candidates.append(drive)
            except:
                pass
    else:
        # Unix-like systems (Linux, macOS)
        for mount in ["/media", "/mnt", "/Volumes"]:
            if os.path.exists(mount):
                for root, dirs, _ in os.walk(mount):
                    for d in dirs:
                        path = os.path.join(root, d)
                        if os.path.ismount(path):
                            candidates.append(path)
        # Also check common removable device names
        for dev in ["/media", "/mnt", "/Volumes"]:
            for entry in os.listdir(dev) if os.path.exists(dev) else []:
                path = os.path.join(dev, entry)
                if os.path.ismount(path):
                    candidates.append(path)
    
    # Remove duplicates
    candidates = list(set(candidates))
    return candidates

def prompt_sd_card_location(defaults):
    print("Possible SD card locations found:")
    for i, path in enumerate(defaults):
        print(f"  [{i+1}] {path}")
    choice = input(f"Select SD card location [1-{len(defaults)}] or enter a path: ").strip()
    if choice.isdigit() and 1 <= int(choice) <= len(defaults):
        return defaults[int(choice)-1]
    elif choice:
        return choice
    else:
        return defaults[0] if defaults else None

def prompt_geofence():
    def get_float(prompt):
        while True:
            try:
                return float(input(prompt))
            except ValueError:
                print("Please enter a valid number.")
    lat = get_float("Latitude: ")
    lon = get_float("Longitude: ")
    rad = get_float("Radius (meters): ")
    return lat, lon, rad

def confirm_location(path):
    resp = input(f"Use SD card location '{path}'? [Y/n]: ").strip().lower()
    return resp in ("", "y", "yes")

def load_geofences_from_json(json_path):
    """Load geofences from a JSON file.
    
    Expected JSON format:
    {
        "geofences": [
            {"latitude": 43.084, "longitude": -77.676, "radiusMeters": 1000},
            {"latitude": 43.085, "longitude": -77.677, "radiusMeters": 500}
        ]
    }
    """
    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
        
        if 'geofences' not in data:
            print(f"Error: JSON file must contain a 'geofences' array")
            sys.exit(1)
        
        geofences = []
        for i, gf in enumerate(data['geofences']):
            try:
                lat = float(gf['latitude'])
                lon = float(gf['longitude'])
                rad = float(gf['radiusMeters'])
                geofences.append((lat, lon, rad))
            except (KeyError, ValueError) as e:
                print(f"Error parsing geofence {i+1}: {e}")
                sys.exit(1)
        
        return geofences
    except FileNotFoundError:
        print(f"Error: JSON file '{json_path}' not found")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON format in '{json_path}': {e}")
        sys.exit(1)

def write_geofences(path, geofences):
    file_path = os.path.join(path, GEOFENCE_FILENAME)
    with open(file_path, "wb") as f:
        for lat, lon, rad in geofences:
            f.write(struct.pack(GEOFENCE_STRUCT, lat, lon, rad))
    print(f"Wrote {len(geofences)} geofences to {file_path}")

def main():
    parser = argparse.ArgumentParser(
        description='Write geofences to SD card in binary format',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Interactive mode
  python geofences_writer.py
  
  # Load from JSON file
  python geofences_writer.py --json geofences.json
  
  # Specify SD card location
  python geofences_writer.py --json geofences.json --path /media/sdcard
        '''
    )
    parser.add_argument('--json', '-j', metavar='FILE', 
                        help='Load geofences from a JSON file')
    parser.add_argument('--path', '-p', metavar='PATH',
                        help='Specify SD card path directly (skip auto-detection)')
    
    args = parser.parse_args()
    
    # Determine SD card path
    if args.path:
        sd_path = args.path
        if not os.path.isdir(sd_path):
            print(f"Error: Specified path '{sd_path}' is not a valid directory.")
            sys.exit(1)
    else:
        candidates = find_sd_card()
        sd_path = prompt_sd_card_location(candidates)
        if not sd_path or not os.path.isdir(sd_path):
            print("SD card location not found or invalid.")
            sys.exit(1)
        if not confirm_location(sd_path):
            print("Aborted by user.")
            sys.exit(0)
    
    # Load geofences
    if args.json:
        geofences = load_geofences_from_json(args.json)
        print(f"Loaded {len(geofences)} geofences from {args.json}:")
        for i, (lat, lon, rad) in enumerate(geofences):
            print(f"  Geofence {i+1}: Lat {lat}, Lon {lon}, Radius {rad} m")
    else:
        # Interactive mode
        geofences = []
        print("Enter geofences (CTRL+D to finish):")
        try:
            while True:
                geofence = prompt_geofence()
                geofences.append(geofence)
        except (EOFError, KeyboardInterrupt):
            print("\nInput finished.")
            for i, (lat, lon, rad) in enumerate(geofences):
                print(f"  Geofence {i+1}: Lat {lat}, Lon {lon}, Radius {rad} m")

    if geofences:
        write_geofences(sd_path, geofences)
    else:
        print("No geofences entered.")

if __name__ == "__main__":
    main()