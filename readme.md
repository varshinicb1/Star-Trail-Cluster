# Star Trail Instrument Cluster

A premium digital instrument cluster for the **CrowPanel 1.28" Round ESP32-S3 Display** — a complete automotive HUD replacement with a companion mobile app.

```
ESP32-S3 (BenzCluster firmware)    ◄── BLE/WiFi ──►    Flutter App (star_trail)
├── 7 LVGL widgets on 240x240 GC9A01              ├── Real-time dashboard mirror
├── MPU9250 9-DOF + BME280 sensors                ├── Widget/LED/System controls
├── 5x NeoPixel status LEDs                       ├── OTA firmware updates
├── BLE HID media keys                            └── BLE scan + WiFi connect
├── BLE phone notifications
├── WiFi web dashboard + REST API
└── Rotary encoder navigation
```

## Features

### Hardware
- **Display:** 1.28" round GC9A01 (240x240) SPI display
- **Sensors:** MPU9250 (accelerometer + gyroscope + magnetometer), BME280 (temperature, humidity, pressure, altitude)
- **Input:** Rotary encoder with push button, capacitive touch (CST816D)
- **Lighting:** 5 individually addressable NeoPixel LEDs
- **Connectivity:** WiFi (ESP32-S3), BLE (NimBLE stack)

### Firmware (BenzCluster)
- 7 swipeable LVGL widgets: **Clock, Compass, Attitude Indicator, Altitude/Temperature, G-Force, Music Remote, Airplane**
- 3 clock faces (digital, analog, minimal)
- ICAO-standard attitude indicator
- BLE HID media control (play/pause/next/prev/volume)
- BLE phone notification display
- WiFi web dashboard with REST API on port 80
- OTA firmware updates over WiFi
- Rotary encoder for brightness, volume, widget navigation
- 3-second encoder hold for system overlay (sensor viewer, system info, calibration)

### Companion App (star_trail)
- Real-time dashboard mirror with animated gauges
- 3 themes: **Star Trail** (Mercedes cyan/blue), **Illuminati** (VW red), **DR** (personal purple)
- Glassmorphism UI with premium automotive HUD aesthetic
- Widget configuration (enable/disable, reorder, swipe direction, knob mode)
- LED control (color picker, patterns, brightness, speed)
- System settings (display brightness, screen timeout, WiFi config, device info)
- OTA firmware update upload (file picker + progress bar)
- Auto-update via GitHub Releases (checks for new APK versions, downloads & installs)
- Connection via BLE scan, WiFi IP, or built-in simulator mode

## Quick Start

### Prerequisites
- CrowPanel 1.28" ESP32-S3 Round Display
- Arduino IDE (with ESP32-S3 board support) or PlatformIO
- Flutter SDK 3.12.2+ (for companion app)
- USB-C cable

### Flash Firmware (Arduino IDE)
1. Open `BenzCluster/BenzCluster.ino` in Arduino IDE
2. Install required libraries:
   - Adafruit NeoPixel 1.15.3+
   - ArduinoJson 7.4.2+
   - LovyanGFX 1.2.19+
   - lvgl 8.3.11
   - NimBLE-Arduino 2.3.7+
   - ESPAsyncWebServer 3.1.0+
3. Board: **ESP32-S3 Dev Module**
4. Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)** (or use `partitions.csv` for dual OTA)
5. Select the correct COM port
6. Click **Upload**

### Build Companion App
```bash
cd flutter_app/

# Analyze for errors
flutter analyze

# Debug APK
flutter build apk --debug

# Release APK
flutter build apk --release

# Or run on connected device
flutter run
```

The release APK will be at `flutter_app/build/app/outputs/flutter-apk/app-release.apk`.

### OTA Update (from CLI)
```bash
python BenzCluster/ota_upload.py --ip 192.168.x.x --bin build/BenzCluster.ino.bin
```

## Architecture

### Communication
The ESP32-S3 runs a WiFi access point (or connects to your network) with a REST API server on port 80, and simultaneously acts as a BLE peripheral broadcasting sensor data and accepting commands. The Flutter app connects via either channel.

### REST API Endpoints
| Endpoint | Method | Params | Description |
|----------|--------|--------|-------------|
| `/api/status` | GET | — | Full sensor + system JSON |
| `/api/led` | GET | `state`, `color`, `brightness`, `pattern`, `speed` | LED control |
| `/api/brightness` | GET | `v` | Display brightness |
| `/api/timeout` | GET | `v` | Screen timeout seconds |
| `/api/music` | GET | `cmd` | Media commands (play/next/prev/vol_up/vol_down) |
| `/api/widgets` | GET | `enabled`, `order`, `swipe`, `knob` | Widget configuration |
| `/api/app-version` | GET | — | Returns Flutter app version info for auto-update |
| `/reboot` | GET | — | Reboot device |
| `/update` | POST | firmware.bin | OTA firmware update |

### BLE Protocol
- **Notify characteristic:** Broadcasts JSON with `heading`, `pitch`, `roll`, `temp`, `alt_ft`, `pressure`, accelerometer/magnetometer raw data, uptime, heap, RSSI, IP, SSID
- **Write characteristic:** Accepts commands matching the REST API path format

### Widget System
The firmware renders 7 widgets swipeable on the round display. Each widget is a separate LVGL screen created in its own `.cpp`/`.h` file pair. The companion app mirrors these with `CustomPainter` implementations.

## Hardware Pinout

| Component | Interface | Pins |
|-----------|-----------|------|
| GC9A01 Display | SPI | SCLK=10, MOSI=11, DC=3, CS=9, RST=14, BL=46 |
| CST816D Touch | I2C (Wire1) | SDA=6, SCL=7, RST=13, INT=5 |
| MPU9250 | I2C (Wire) | SDA=38, SCL=39, ADDR=0x68 |
| QMC5883L | I2C (Wire) | ADDR=0x0D |
| BME280 | I2C (Wire) | ADDR=0x76 |
| Rotary Encoder | GPIO | A=45, B=42, SW=41 |
| NeoPixel | GPIO | PIN=48, COUNT=5 |

## Project Structure

```
CrowPanel_Repo/
├── BenzCluster/          ESP32-S3 firmware (Arduino sketch)
├── flutter_app/          Flutter companion app (star_trail)
├── Datasheet/            Component datasheets
├── Eagle_SCH&PCB/        Circuit schematic and PCB layout
├── 3D file/              Enclosure STEP model
├── simulator/            PC LVGL simulator
├── factory_firmware/     Pre-compiled factory binaries + flash tool
├── factory_soucecode/    Factory SquareLine Studio project
├── example/              Example sketches and third-party libraries
└── SensorDiag/           Standalone sensor diagnostic tool
```

## CI/CD & Release

### Building for Release

**Firmware**
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 --partitions-scheme huge_app BenzCluster/
```

**Flutter APK**
```bash
cd flutter_app/
flutter build apk --release --build-name=1.0.1 --build-number=2
```

Release APK: `flutter_app/build/app/outputs/flutter-apk/app-release.apk` (50.7MB)
Debug APK: `flutter_app/build/app/outputs/flutter-apk/app-debug.apk`

### Creating a GitHub Release
1. Build firmware and APK (commands above)
2. Create a GitHub Release with tag `v1.0.1` (match `--build-name`)
3. Upload `app-release.apk` as a release asset
4. Set `GITHUB_REPO=edgehax/star_trail` in `lib/services/update_service.dart` (update to your repo)
5. Users' apps auto-detect the new version on next launch

### Auto-Update Flow
- `UpdateService` checks GitHub API for latest release on app launch
- Compares versions semantically against installed app version
- If newer, downloads APK to temp directory and opens via Android FileProvider
- Requires Android 7+ (FileProvider for secure APK install)

### Verification
- `flutter analyze` — 0 issues
- `flutter test` — 19 tests pass
- Firmware: ESP32-S3 Huge APP partition, ~57% flash usage

## License

Private project. All rights reserved.
