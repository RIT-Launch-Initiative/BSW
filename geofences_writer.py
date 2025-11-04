import os
import sys
import struct

# latitude, longitude, radiusMeters,
# minAltitudeMeters, maxAltitudeMeters
GEOFENCE_STRUCT = "fffff"
GEOFENCE_SIZE = struct.calcsize(GEOFENCE_STRUCT)
GEOFENCE_FILENAME = "geofences"

def find_sd_card():
    candidates = []
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
    min_alt = get_float("Minimum Altitude (meters): ")
    max_alt = get_float("Maximum Altitude (meters): ")
    return lat, lon, rad, min_alt, max_alt

def confirm_location(path):
    resp = input(f"Use SD card location '{path}'? [Y/n]: ").strip().lower()
    return resp in ("", "y", "yes")

def write_geofences(path, geofences):
    file_path = os.path.join(path, GEOFENCE_FILENAME)
    with open(file_path, "wb") as f:
        for lat, lon, rad, min_alt, max_alt in geofences:
            f.write(struct.pack(GEOFENCE_STRUCT, lat, lon, rad, min_alt, max_alt))
    print(f"Wrote {len(geofences)} geofences to {file_path}")

def main():
    candidates = find_sd_card()
    sd_path = prompt_sd_card_location(candidates)
    if not sd_path or not os.path.isdir(sd_path):
        print("SD card location not found or invalid.")
        sys.exit(1)
    if not confirm_location(sd_path):
        print("Aborted by user.")
        sys.exit(0)

    geofences = []
    print("Enter geofences (CTRL+D to finish):")
    try:
        while True:
            geofence = prompt_geofence()
            geofences.append(geofence)
    except (EOFError, KeyboardInterrupt):
        print("\nInput finished.")
        for i, (lat, lon, rad, min_alt, max_alt) in enumerate(geofences):
            print(f"  Geofence {i+1}: Lat {lat}, Lon {lon}, Radius {rad} m, Alt {min_alt}-{max_alt} m")

    if geofences:
        write_geofences(sd_path, geofences)
    else:
        print("No geofences entered.")

if __name__ == "__main__":
    main()