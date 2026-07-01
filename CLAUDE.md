# CLAUDE.md — Star Trail Instrument Cluster

## Project Overview

A digital instrument cluster for the **CrowPanel 1.28" Round ESP32-S3 Display**, designed as an automotive HUD replacement. Two major components:

- **`BenzCluster/`** — ESP32-S3 firmware (Arduino sketch) running LVGL on a 240x240 round GC9A01 display. Sensor fusion (MPU9250 + BME280), 7 swipeable LVGL widgets, BLE HID media control, BLE phone notifications, WiFi web dashboard, OTA updates.
- **`flutter_app/`** — Flutter companion app (`star_trail`) for Android/iOS that mirrors the dashboard, configures widgets/LEDs/system, OTA firmware updates over BLE/WiFi.

Also contains: Eagle CAD schematics, 3D enclosure models, component datasheets, factory firmware, example sketches, PC simulator.

## Repository Structure

```
CrowPanel_Repo/
├── BenzCluster/              # ESP32-S3 firmware (main deliverable)
│   ├── BenzCluster.ino       # Entry point: setup(), loop(), widget switching
│   ├── config.h              # Hardware pins, WiFi creds, sensor addresses
│   ├── partitions.csv        # Dual OTA partition table
│   ├── ota_upload.py         # CLI OTA uploader
│   ├── display.cpp/h         # ST7789V/LovyanGFX + LVGL init
│   ├── sensors.cpp/h         # MPU9250 + AK8963 + BME280 drivers
│   ├── leds.cpp/h            # NeoPixel LED strip (5 pins)
│   ├── wifi_manager.cpp/h    # WiFi, NTP, OpenWeatherMap
│   ├── ota_update.cpp/h      # Web dashboard, REST API, OTA
│   ├── ble_media.cpp/h       # BLE HID media keys
│   ├── ble_notify.cpp/h      # BLE phone notifications
│   ├── clock.cpp/h, compass.cpp/h, attitude.cpp/h, alttemp.cpp/h
│   ├── gforce.cpp/h, music.cpp/h, airplane.cpp/h
│   ├── sensorview.cpp/h, systemview.cpp/h, calibration.cpp/h
│   ├── calibration_widget.cpp/h, ledcolor.cpp/h, splash.cpp/h
│   ├── benz_logo.h, vw_logo.h, illuminati_logo.h, illuminati_text.h
│   ├── lv_conf.h             # LVGL 8.3.11 config
│   └── build/                # Pre-compiled binaries
│
├── flutter_app/              # Flutter companion app (star_trail)
│   ├── lib/
│   │   ├── main.dart         # App entry, routing, AppShell (bottom nav)
│   │   ├── models/device_data.dart       # DeviceData, WidgetConfig, AppSettings
│   │   ├── services/device_service.dart  # BLE/WiFi/Simulator communication
│   │   ├── services/update_service.dart  # GitHub release check + APK auto-install
│   │   ├── providers/theme_provider.dart  # Theme state (3 modes)
│   │   ├── theme/app_theme.dart           # 3 dark themes + ThemeData
│   │   ├── screens/                      # Welcome, Home, Config, OTA, DeviceControls
│   │   │   └── controls/                 # LED, Widget, System screens
│   │   ├── widgets/glass_container.dart  # Glassmorphism UI
│   │   ├── widgets/logo_widgets.dart     # Custom logo painters
│   │   └── painters/widget_painters.dart # 7 instrument CustomPainters
│   ├── pubspec.yaml         # flutter_blue_plus, http, provider, shared_preferences, package_info_plus, open_filex, path_provider, file_picker
│   └── test/
│
├── simulator/                # PC-side LVGL simulator (CMake + main.cpp)
├── example/                  # Example sketches, libraries, desktop agent
├── factory_firmware/         # Pre-compiled factory binaries + flashing tool
├── factory_soucecode/        # Factory source (SquareLine Studio)
├── Datasheet/                # Component datasheets
├── Eagle_SCH&PCB/            # Eagle CAD schematic + PCB layout
├── 3D file/                  # STEP model of enclosure
└── SensorDiag/               # Standalone sensor diagnostic sketch
```

## Build & Run

### Flutter App
```bash
cd flutter_app/
flutter analyze                    # Lint + type check
flutter build apk --debug          # Debug APK
flutter build apk --release        # Release APK (50.7MB)
flutter run                        # Run on connected device
```

### Firmware (Arduino IDE)
1. Open `BenzCluster/BenzCluster.ino` in Arduino IDE
2. Board: ESP32-S3 Dev Module, Partition: "Huge APP (3MB No OTA/1MB SPIFFS)"
3. Install: Adafruit NeoPixel 1.15.3+, ArduinoJson 7.4.2+, LovyanGFX 1.2.19+, lvgl 8.3.11, NimBLE-Arduino 2.3.7+, ESPAsyncWebServer 3.1.0+
4. Select COM port, Upload

### Firmware (PlatformIO)
```ini
[env:esp32-s3-dev]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.partitions = partitions.csv
lib_deps =
    adafruit/Adafruit NeoPixel@^1.15.3
    bblanchon/ArduinoJson@^7.4.2
    lovyan03/LovyanGFX@^1.2.19
    lvgl@^8.3.11
    h2zero/NimBLE-Arduino@^2.3.7
    me-no-dev/ESPAsyncWebServer@^3.1.0
```

### OTA Upload
```bash
python BenzCluster/ota_upload.py --ip <device_ip> --bin build/BenzCluster.ino.bin
```

## Architecture

### Communication Flow
```
Flutter App ◄── BLE (notify + write) ──► ESP32-S3 (BenzCluster)
           ◄── WiFi REST API (port 80)──►  - LVGL UI, MPU9250/BME280
                                            - NeoPixels x5, Rotary Encoder
                                            - BLE HID Media
```

### REST API Endpoints (firmware)
| Endpoint | Method | Params | Description |
|----------|--------|--------|-------------|
| `/api/status` | GET | — | Full JSON status (sensors, system) |
| `/api/led` | GET | `state`, `color`, `brightness` | LED control |
| `/api/brightness` | GET | `v` | Display brightness |
| `/api/timeout` | GET | `v` | Screen timeout seconds |
| `/api/music` | GET | `cmd` | Media command |
| \`/api/widgets\` | GET | \`enabled\`, \`order\`, \`swipe\`, \`knob\` | Widget config |
| \`/api/app-version\` | GET | — | Returns Flutter app version info for auto-update |
| \`/reboot\` | GET | — | Reboot device |
| \`/update\` | POST | firmware.bin | OTA firmware update |

### BLE Data Format (JSON)
```json
{"heading":180.5,"pitch":2.1,"roll":-1.3,"temp":25.4,"alt_ft":3018,
 "pressure":1013.2,"mx":12,"my":-34,"mz":56,"ax":0.1,"ay":-0.2,"az":1.0,
 "uptime":3600,"heap":204800,"rssi":-65,"ip":"10.91.5.73","ssid":"V"}
```

### Command Routing (Flutter → Device)
| App Command | Routed To |
|-------------|-----------|
| `reboot` | `/reboot` |
| `brightness=N` | `/api/brightness?v=N` |
| `timeout=N` | `/api/timeout?v=N` |
| `led_on/off` | `/api/led?state=on/off` |
| `led_color=RRGGBB` | `/api/led?color=RRGGBB` |
| \`play/next/prev\` | \`/api/music?cmd=play/next/prev\` |
| \`vol_up/vol_down\` | \`/api/music?cmd=vol_up/vol_down\` |
| \`widgets=X\` | \`/api/widgets?widgets=X\` |
| \`widget_order=X\` | \`/api/widgets?widget_order=X\` |
| \`swipe_dir=X\` | \`/api/widgets?swipe_dir=X\` |
| \`knob_mode=X\` | \`/api/widgets?knob_mode=X\` |
| \`app_version\` | \`/api/app-version\` |

## Firmware Widget System (7 swipeable)
1. Clock (3 faces), 2. Compass, 3. Attitude (ICAO), 4. Alt/Temp, 5. G-Force, 6. Music (BLE remote), 7. Airplane
Hidden: sensorview, systemview, calibration (3s encoder hold)

## Flutter App Architecture
- **State:** Provider (ChangeNotifier) — ThemeProvider, DeviceService
- **Nav:** Named routes (`/welcome`, `/home`, `/config`, `/ota`) + AppShell (3-tab bottom nav)
- **Connection:** `none`, `ble`, `wifi`, `simulator`
- **Theme:** 3 dark themes (Star Trail/Mercedes cyan, Illuminati/VW red, DR/purple) with 20-color system + Material3
- **Key widgets:** `GlassContainer`, `GlassButton`, `GlowingDot` (glassmorphism), `ThemeLogo` (logo painters)

## Code Conventions

### Flutter (Dart)
- **Naming:** `snake_case` files, `camelCase` vars/funcs, `PascalCase` classes
- **Imports:** `package:` for deps, relative for internal (`../../../` pattern)
- **Screens:** One file per screen, controls in `screens/controls/`
- **State:** `context.watch<T>()` for reactivity, `context.read<T>()` for one-shot
- **Style:** No trailing commas on single-line widgets; composition over inheritance

### Firmware (C++)
- **Naming:** `camelCase` functions, `PascalCase` classes, `UPPER_SNAKE` constants
- **LVGL:** Event-driven; screen creation in widget constructors
- **Sensors:** I2C via Wire/Wire1; feed task watchdog in long loops
- **JSON:** `ArduinoJson`; keys match across BLE and REST API
- **BLE:** NimBLE stack; notify char broadcasts JSON, write char accepts commands

## Hardware Pin Mapping
| Component | Pins |
|-----------|------|
| Display (SPI GC9A01) | SCLK=10, MOSI=11, DC=3, CS=9, RST=14, BL=46 |
| Touch (I2C CST816D) | SDA=6, SCL=7, RST=13, INT=5 (Wire1) |
| External I2C | SDA=38, SCL=39 (Wire) |
| MPU9250 @ 0x68 / QMC5883L @ 0x0D / BME280 @ 0x76 | Wire |
| Rotary Encoder | A=45, B=42, SW=41 |
| NeoPixel | PIN=48, COUNT=5 |

## Default WiFi
- SSID: `V`, Password: `varshu99`
- Location: Bangalore (declination -1.09, elevation 920m/3018ft)
- NTP: pool.ntp.org GMT+5:30 IST

## Key Patterns

### Adding a Flutter Screen
```dart
// 1. Create file in lib/screens/ or lib/screens/controls/
// 2. Extend StatefulWidget
// 3. Use theme: context.watch<ThemeProvider>().theme
// 4. Use DeviceService: context.watch<DeviceService>()
// 5. Use GlassContainer/GlassButton from widgets/glass_container.dart
```

### Adding a Firmware Widget
```cpp
// 1. Create widgetname.cpp + widgetname.h (LVGL screen creation)
// 2. Register in BenzCluster.ino widgets[] array
// 3. Add icon to widget_icons[]
// 4. Optionally add to /api/widgets in ota_update.cpp
```

### Sending Commands
```dart
final svc = context.read<DeviceService>();
await svc.sendCommand('brightness=80');
await svc.sendCommand('led_color=FF0000');
await svc.sendCommand('reboot');
```

### Reading Sensor Data
```dart
final svc = context.watch<DeviceService>();
final heading = svc.data.heading;
final temp = svc.data.temp;
```

## App Identifiers
- Package: `com.edgehax.star_trail`
- SDK: Flutter 3.12.2+ / Dart ^3.12.2
- Android: compileSdk = flutter.compileSdkVersion, NDK = flutter.ndkVersion
- Firmware: Star Trail v8.0
- App: 1.0.0+1 (star_trail)

## CI/CD & Release

### Build for Release
\`\`\`bash
# Firmware
arduino-cli compile --fqbn esp32:esp32:esp32s3 --partitions-scheme huge_app BenzCluster/

# Flutter APK
cd flutter_app/
flutter build apk --release --build-name=1.0.1 --build-number=2
\`\`\`

### GitHub Release
1. Build firmware + APK
2. Create GitHub Release with tag \`v1.0.1\`
3. Upload \`app-release.apk\` (50.7MB) as release asset
4. Set \`githubRepo = 'edgehax/star_trail'\` in \`update_service.dart\` (update to your repo)
5. App auto-update checks GitHub API on launch, compares versions, downloads & installs APK via Android FileProvider

### Checks
- \`flutter analyze\` — 0 issues
- \`flutter test\` — 19 tests pass
- Firmware: ESP32-S3 Huge APP, ~57% flash
