# AGENTS.md — Star Trail Instrument Cluster

## Project Overview

A digital instrument cluster for the **CrowPanel 1.28" Round ESP32-S3 Display**, designed as an automotive HUD replacement. The project consists of two major components:

- **`BenzCluster/`** — ESP32-S3 firmware (Arduino sketch) running LVGL on a 240x240 round GC9A01 display. Handles sensor fusion (MPU9250 + BME280), 7 swipeable LVGL widgets, BLE HID media control, BLE phone notifications, WiFi web dashboard, and OTA updates.
- **`flutter_app/`** — Flutter companion app (`star_trail`) for Android/iOS that mirrors the dashboard, configures widgets/LEDs/system settings, and performs OTA firmware updates over BLE or WiFi.

The repo also contains Eagle CAD schematics, 3D enclosure models, component datasheets, factory firmware, example sketches, and a PC simulator.

## Repository Structure

```
CrowPanel_Repo/
├── BenzCluster/                    # ESP32-S3 firmware (main deliverable)
│   ├── BenzCluster.ino            # Entry point: setup(), loop(), widget switching
│   ├── config.h                   # Hardware pins, WiFi creds, sensor addresses
│   ├── partitions.csv             # Dual OTA partition table (6.5MB + 6.5MB + SPIFFS)
│   ├── ota_upload.py              # CLI OTA uploader script
│   ├── display.cpp/h              # ST7789V/LovyanGFX + LVGL init
│   ├── sensors.cpp/h              # MPU9250 + AK8963 + BME280 drivers
│   ├── leds.cpp/h                 # NeoPixel LED strip (5 pins)
│   ├── wifi_manager.cpp/h         # WiFi, NTP, OpenWeatherMap
│   ├── ota_update.cpp/h           # Web dashboard, REST API, OTA
│   ├── ble_media.cpp/h            # BLE HID media keys (play/pause/next/prev)
│   ├── ble_notify.cpp/h           # BLE phone notifications
│   ├── clock.cpp/h                # 3-face clock widget
│   ├── compass.cpp/h              # Compass widget
│   ├── attitude.cpp/h             # ICAO attitude indicator
│   ├── alttemp.cpp/h              # Altitude + temperature
│   ├── gforce.cpp/h               # G-force display
│   ├── music.cpp/h                # BLE music remote UI
│   ├── sensorview.cpp/h           # Raw sensor data view
│   ├── systemview.cpp/h           # System info overlay
│   ├── airplane.cpp/h             # Airplane attitude widget
│   ├── calibration.cpp/h          # Magnetometer calibration
│   ├── calibration_widget.cpp/h   # Calibration wizard UI
│   ├── ledcolor.cpp/h             # NeoPixel color picker
│   ├── splash.cpp/h               # Splash screen (3 themes)
│   ├── benz_logo.h, vw_logo.h, illuminati_logo.h, illuminati_text.h  # Splash assets
│   ├── lv_conf.h                  # LVGL 8.3.11 configuration
│   └── build/                     # Pre-compiled firmware binaries
│
├── flutter_app/                    # Flutter companion app
│   ├── lib/
│   │   ├── main.dart              # App entry, routing, AppShell with bottom nav
│   │   ├── models/
│   │   │   └── device_data.dart   # DeviceData, WidgetConfig, AppSettings
│   │   ├── services/
│   │   │   ├── device_service.dart # BLE/WiFi/Simulator communication service
│   │   │   └── update_service.dart  # GitHub release check + APK auto-install
│   │   ├── providers/
│   │   │   └── theme_provider.dart # Theme state (starTrail/illuminati/neutral)
│   │   ├── theme/
│   │   │   └── app_theme.dart     # 3 complete dark themes with full ThemeData
│   │   ├── screens/
│   │   │   ├── welcome_screen.dart # Animated splash + theme selector
│   │   │   ├── home_screen.dart    # Main dashboard with instrument gauges
│   │   │   ├── config_screen.dart  # BLE scan, WiFi connect, settings
│   │   │   ├── device_controls_screen.dart  # Menu: Widgets/LED/System
│   │   │   ├── ota_screen.dart     # Firmware OTA update UI
│   │   │   └── controls/
│   │   │       ├── led_screen.dart # LED color/pattern/brightness control
│   │   │       ├── widget_screen.dart # Widget toggles, order, swipe/knob config
│   │   │       └── system_screen.dart # Brightness, timeout, WiFi, device info, reboot
│   │   ├── widgets/
│   │   │   ├── glass_container.dart # Glassmorphism UI components
│   │   │   └── logo_widgets.dart    # Custom logo painters (Mercedes/VW/DR)
│   │   └── painters/
│   │       └── widget_painters.dart # 7 instrument cluster CustomPainters
│   ├── pubspec.yaml               # Dependencies: flutter_blue_plus, http, provider, shared_preferences, package_info_plus, open_filex, path_provider, file_picker
│   └── test/
│
├── simulator/                      # PC-side LVGL simulator (CMakeLists.txt + main.cpp)
├── example/                        # Example sketches, libraries, desktop agent
├── factory_firmware/               # Pre-compiled factory flash binaries + flashing tool
├── factory_soucecode/              # Factory source (SquareLine Studio)
├── Datasheet/                      # Component datasheets (display, encoder, FPC, hall sensor)
├── Eagle_SCH&PCB/                  # Eagle CAD schematic + PCB layout
├── 3D file/                        # STEP model of enclosure
├── SensorDiag/                     # Standalone sensor diagnostic sketch
├── build_output.txt                # Arduino CLI build log
└── build_errors.txt                # Build error log
```

## Architecture

### Communication Flow

```
┌──────────────┐     BLE (notify + write)     ┌──────────────────┐
│              │ ◄──────────────────────────► │                  │
│  Flutter App │                              │  ESP32-S3 (BenzCluster)  │
│  (star_trail)│     WiFi REST API (port 80)  │                  │
│              │ ◄──────────────────────────► │  - LVGL UI       │
└──────────────┘                              │  - MPU9250/BME280│
                                              │  - NeoPixels x5   │
                                              │  - Rotary Encoder │
                                              │  - BLE HID Media  │
                                              └──────────────────┘
```

### Firmware Widget System (7 swipeable widgets)

1. **Clock** (3 faces: digital, analog, minimal) — `clock.cpp`
2. **Compass** — `compass.cpp`
3. **Attitude Indicator** (ICAO standard) — `attitude.cpp`
4. **Altitude + Temperature** — `alttemp.cpp`
5. **G-Force** — `gforce.cpp`
6. **Music** (BLE media remote) — `music.cpp`
7. **Airplane** — `airplane.cpp`

Hidden system overlay (3s encoder hold): sensorview, systemview, calibration.

### Flutter App Architecture

- **State management:** Provider (ChangeNotifier)
- **Navigation:** Named routes `(/welcome, /home, /config, /ota)` + `PageView` with `AppShell` (3-tab bottom nav: Dashboard, Controls, OTA)
- **Connection modes:** `none`, `ble`, `wifi`, `simulator` (test without hardware)
- **Theme system:** 3 complete dark themes with 20 color properties each + full Material3 ThemeData

### REST API Endpoints (firmware)

| Endpoint | Method | Params | Description |
|----------|--------|--------|-------------|
| `/api/status` | GET | — | Full JSON status (sensors, system info) |
| `/api/led` | GET | `state`, `color`, `brightness` | LED control |
| `/api/brightness` | GET | `v` | Display brightness |
| `/api/timeout` | GET | `v` | Screen timeout seconds |
| `/api/music` | GET | `cmd` | Media command (play/next/prev/vol_up/vol_down) |
| \`/api/widgets\` | GET | \`enabled\`, \`order\`, \`swipe\`, \`knob\` | Widget config |
| \`/api/app-version\` | GET | — | Returns Flutter app version info for auto-update |
| \`/api/led\` | GET | \`state\`, \`color\`, \`brightness\`, \`pattern\`, \`speed\` | Full LED control |
| \`/reboot\` | GET | — | Reboot device |
| \`/update\` | POST | firmware.bin | OTA firmware update |

### BLE Protocol

- **Service (notify):** Temperature, Pressure, Accelerometer, Gyroscope, Magnetometer
- **Service (cmd):** Write characteristic for sending commands
- Data format: JSON with keys `heading`, `pitch`, `roll`, `temp`, `alt_ft`, `pressure`, `mx`, `my`, `mz`, `ax`, `ay`, `az`, `uptime`, `heap`, `rssi`, `ip`, `ssid`

### Command Routing (Flutter → Device)

The Flutter app's `DeviceService.sendCommand()` uses `_routeCommand()` to map high-level commands to the correct firmware path:

| App Command | Routed To |
|-------------|-----------|
| `reboot` | `/reboot` |
| `brightness=N` | `/api/brightness?v=N` |
| `timeout=N` | `/api/timeout?v=N` |
| `led_on` / `led_off` | `/api/led?state=on` / `off` |
| `led_color=RRGGBB` | `/api/led?color=RRGGBB` |
| `led_pattern=X` | `/api/led?pattern=X` |
| `led_brightness=N` | `/api/led?brightness=N` |
| `play` / `next` / `prev` | `/api/music?cmd=play` / `next` / `prev` |
| `vol_up` / `vol_down` | `/api/music?cmd=vol_up` / `vol_down` |
| `widgets=X` | `/api/widgets?widgets=X` |
| `widget_order=X` | `/api/widgets?widget_order=X` |
| `swipe_dir=X` | `/api/widgets?swipe_dir=X` |
| \`knob_mode=X\` | \`/api/widgets?knob_mode=X\` |
| \`app_version\` | \`/api/app-version\` |

## Build & Run

### Flutter App

```bash
cd flutter_app/

# Analyze
flutter analyze

# Debug APK
flutter build apk --debug

# Release APK
flutter build apk --release

# Run on connected device
flutter run
```

### Firmware (Arduino IDE)

1. Open `BenzCluster/BenzCluster.ino` in Arduino IDE
2. Set board: ESP32-S3 Dev Module
3. Set Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)" or custom `partitions.csv`
4. Install libraries: Adafruit NeoPixel 1.15.3+, ArduinoJson 7.4.2+, LovyanGFX 1.2.19+, lvgl 8.3.11, NimBLE-Arduino 2.3.7+, ESPAsyncWebServer 3.1.0+
5. Select correct COM port
6. Upload

### Firmware (PlatformIO)

Create `platformio.ini`:
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
python BenzCluster/ota_upload.py --ip 10.91.5.73 --bin build/BenzCluster.ino.bin
```

## Code Conventions

### Flutter (Dart)
- **State management:** Provider with `ChangeNotifier`
- **Theming:** Custom `AppTheme` class with color constants + standard `ThemeData`
- **Widgets:** Composition over inheritance; prefer `StatelessWidget` + `Consumer`
- **Naming:** `snake_case` for files, `camelCase` for variables/functions, `PascalCase` for classes
- **Imports:** `package:` for Flutter/pub deps, relative for internal (`../../../` pattern)
- **CustomPainters:** All instrument gauges in `painters/widget_painters.dart`
- **Screens:** One file per screen, grouped in `screens/` with `controls/` subdirectory
- **Services:** Singleton `ChangeNotifier` provided at app root via `MultiProvider`

### Firmware (C++)
- **Naming:** `camelCase` functions, `PascalCase` classes, `UPPER_SNAKE` constants
- **LVGL:** Event-driven UI with `lv_*` API calls; screen creation in widget constructors
- **Sensors:** I2C via Wire/Wire1; task watchdog fed in long loops
- **OTA:** `ESPAsyncWebServer` on port 80 with `/update` for firmware + LittleFS upload
- **BLE:** `NimBLE` stack; notify characteristic broadcasts JSON status; write characteristic accepts commands
- **JSON:** Built with `ArduinoJson`; keys match across BLE and REST API

## Key Patterns

### Adding a New Flutter Screen
1. Create file in `lib/screens/` or `lib/screens/controls/`
2. Extend `StatefulWidget` with `Consumer<DeviceService>` or `context.watch<DeviceService>()`
3. Use `GlassContainer`, `GlassButton` from `widgets/glass_container.dart`
4. Import `AppTheme` from `theme/app_theme.dart` and use `context.watch<ThemeProvider>().theme`
5. Register route in `main.dart` if top-level, or navigate via `Navigator.push`

### Adding a New Firmware Widget
1. Create `widgetname.cpp` + `widgetname.h` with LVGL screen creation
2. Register in `BenzCluster.ino` `widgets[]` array
3. Add icon to `widget_icons[]` for widget bar
4. Optional: add to `/api/widgets` endpoint in `ota_update.cpp`

### Sending Commands from Flutter
```dart
final svc = context.read<DeviceService>();
await svc.sendCommand('brightness=80');
await svc.sendCommand('led_color=FF0000');
await svc.sendCommand('reboot');
```

### Reading Sensor Data in Flutter
```dart
final svc = context.watch<DeviceService>();
final heading = svc.data.heading;
final pitch = svc.data.pitch;
final temp = svc.data.temp;
```

## Hardware Pin Mapping

| Component | Pins |
|-----------|------|
| Display (SPI GC9A01) | SCLK=10, MOSI=11, DC=3, CS=9, RST=14, BL=46 |
| Touch (I2C CST816D) | SDA=6, SCL=7, RST=13, INT=5 (Wire1) |
| External I2C | SDA=38, SCL=39 (Wire) |
| MPU9250 | 0x68 on Wire |
| QMC5883L | 0x0D on Wire |
| BME280 | 0x76 on Wire |
| Rotary Encoder | A=45, B=42, SW=41 |
| NeoPixel | PIN=48, COUNT=5 |

## Default WiFi Credentials
- SSID: `V`
- Password: `varshu99`

## Version History
- Firmware: Star Trail v8.0
- Flutter App: 1.0.0+1 (star_trail)

## App Identifiers
- Package name: \`com.edgehax.star_trail\`
- Minimum SDK: Flutter 3.12.2 / Dart ^3.12.2
- Android compileSdk: flutter.compileSdkVersion
- Android NDK: flutter.ndkVersion

## CI/CD & Release

### Building for Release
\`\`\`bash
# Firmware
arduino-cli compile --fqbn esp32:esp32:esp32s3 --partitions-scheme huge_app BenzCluster/

# Flutter APK
cd flutter_app/
flutter build apk --release --build-name=1.0.1 --build-number=2
\`\`\`

### GitHub Release Workflow
1. Build firmware and APK
2. Create a GitHub Release with tag \`v1.0.1\` (match \`--build-name\`)
3. Upload \`app-release.apk\` (50.7MB) as a release asset
4. Set \`githubRepo = 'edgehax/star_trail'\` in \`lib/services/update_service.dart\` (update to your actual repo)
5. Users' apps auto-detect new version on next launch

### Auto-Update Details
- \`UpdateService\` checks GitHub API (\`/repos/{owner}/{repo}/releases/latest\`) on launch
- Semver comparison against \`package_info_plus\` installed version
- Downloads APK via HTTP to temp dir (\`path_provider\`), installs via \`open_filex\`
- FileProvider configured in \`android/app/src/main/res/xml/file_paths.xml\` + \`AndroidManifest.xml\` for Android 7+

### Build Verification
- \`flutter analyze\` — 0 issues
- \`flutter test\` — 19 tests pass
- Firmware: ESP32-S3, Huge APP partition, ~57% flash usage
