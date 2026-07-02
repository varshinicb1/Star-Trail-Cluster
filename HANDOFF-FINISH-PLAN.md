# HANDOFF — Finish Star Trail Cluster for Customer Shipment

This document is the complete, honest state of the project and a step-by-step
plan for an autonomous coding agent to finish it. Read it fully before writing
code. Everything listed as DONE is committed, compiled, and verified. Everything
under PENDING is what actually remains to ship.

---

## 1. Product summary

ESP32-S3 round-display (240×240 GC9A01, LVGL 8.3) car-dashboard instrument
cluster ("Star Trail", Mercedes-Benz styling) + Flutter companion app
(`flutter_app/`, package `star_trail`). Device mounts in a car on a 25–30°
angled enclosure. Customers must NEVER calibrate anything — a one-time factory
calibration (done by the builder via the app) handles mount tilt + magnetometer.

Repo: `varshinicb1/Star-Trail-Cluster`, working branch
`claude/compassionate-moser-ec21ce`, integration branch `V11` (PRs #1–#4 merged).
Latest release: v1.1.0 (APK only). The FINAL release must contain firmware
`.bin` + APK, and is to be cut ONLY when everything below is done and verified
("hold release until everything is polished" — explicit user decision).

## 2. Hardware (confirmed by user — do not change)

- MPU6050 @ 0x68 (accel+gyro, NO magnetometer) — external I2C `Wire` SDA=38 SCL=39
- QMC5883L @ 0x0D (magnetometer)
- BME280 @ 0x76 (pressure/temp → altitude)
- Display SPI GC9A01 (LovyanGFX), touch CST816D on Wire1 (SDA=6 SCL=7)
- Rotary encoder A=45 B=42 SW=41; NeoPixel pin 48 ×5

## 3. DONE (do not redo)

### Firmware (`BenzCluster/`) — compiles at 1.95 MB (62%)
Compile command (must stay green after every change):
```
cd BenzCluster
arduino-cli compile -b esp32:esp32:esp32s3:PartitionScheme=app3M_fat9M_16MB,FlashSize=16M
```
- `sensors.cpp/h` — REWRITTEN for MPU6050+QMC5883L. Complementary pitch/roll
  fusion, tilt-compensated true-north heading (hard/soft-iron + declination
  `MAGNETIC_DECLINATION` −1.09° Bangalore + wrap-aware smoothing).
  Factory-cal API already implemented & persisted to SPIFFS `/cal.dat`:
  - `sensors_factory_zero_orientation()` — captures mount tilt (car level)
  - `sensors_factory_mag_calibrate(int seconds)` — forced figure-8 cal
  - `sensors_set_declination(float)`
  - `sensors_debug_string(char*, int)` — raw+fused values for diagnostics
  ⚠ Axis signs of tilt-comp are UNVALIDATED on real hardware (no board in the
  build environment). User will flash and report; expect possible sign flips in
  `fusePitchRoll()` / `computeHeading()` in `sensors.cpp`.
- `compass.cpp` — REWRITTEN: linear scrolling compass. Black bg, white scale
  (ticks every 15°, labels N/NE/E/SE/S/SW/W/NW every 45°), fixed RED needle at
  centre, big white heading number + "HEADING" caption. Verified in simulator.
- Custom widget pipeline: `custom_widget.cpp/h` (JSON→LVGL interpreter, 7
  element types, SPIFFS `/custom/layout.json`), `custom_screen.cpp/h`
  (widget slot #7 "Custom"), BLE chunked push (see §5 contracts), WiFi route
  `POST /api/custom_layout`.
- Widget rotation in `BenzCluster.ino`: 8 widgets
  (Clock,Compass,Attitude,AltTemp,G-Force,Music,Airplane,Custom).

### Simulator (`simulator/`) — the verification loop, WORKS
- Build: `simulator/build_sim.bat` (MSVC 2022 + vcpkg SDL2 at `C:/vcpkg`,
  LVGL v8.3.9 auto-fetched). Compiles the REAL firmware widget .cpp files.
- Headless screenshots: `simulator/build/simulator.exe --shots simulator/shots`
  → BMPs per widget. ⚠ Colour pipeline is 16-bit RGB565 with
  `LV_COLOR_16_SWAP 0` — do NOT change to 32-bit (LVGL kconfig ignores it here
  and you get channel-swap bugs; already debugged twice).
- After any widget change: rebuild sim → regenerate shots → convert to round
  PNGs into `BenzCluster/docs/widgets/` (see `git log` for the python snippet)
  and keep README gallery current.

### Flutter app (`flutter_app/`) — analyze clean, 20 tests pass
- Premium "Benz" theme (default) + real Mercedes emblem assets
  (`assets/branding/emblem.png`, launcher icons, splash). `google_fonts` Inter.
- Designer screen `lib/screens/designer_screen.dart` + model
  `lib/models/widget_layout.dart` (MUST stay byte-compatible with firmware
  `custom_widget.h` JSON contract) + preview painter
  `lib/painters/custom_widget_painter.dart`.
- `DeviceService.pushLayout()` — BLE chunked ('B'/'C'/'F' opcodes, 160-byte
  chunks) with WiFi POST fallback. `routeCommand()` maps app commands→HTTP.
- Build: `flutter build apk --release` (Flutter at `C:\tools\flutter`).

## 4. PENDING — the actual remaining work, in order

### P1. Firmware: Alt/Temp minimal redesign (`alttemp.cpp`)
Replace the current altimeter-dial with the user's spec: pure-black screen,
altitude icon + altitude value (ft, from BME280) top half, thermometer icon +
temperature (°C) bottom half. White text, minimal. Use LVGL symbols
(`LV_SYMBOL_*`) or simple drawn glyphs; Montserrat fonts 14–44 are enabled.
Keep the existing `alttemp_init/show/hide/update(float tempC, float altFt)` API.

### P2. Firmware: 4 attitude-indicator styles (`attitude.cpp` or new files)
All must be smooth + international-standard accurate. Styles:
1. **Classic round ICAO** (DEFAULT — current implementation is close; keep):
   blue sky/brown ground, pitch ladder ±10/20°, bank arc ticks 10/20/30/45/60°,
   fixed sky pointer, moving roll pointer.
2. **Full-screen horizon** — sky/ground fill the entire 240×240, no bezel ring.
3. **Minimalist line** — black bg, white horizon line, white pitch ladder,
   white roll arc; no colour fills (matches product's black/minimal language).
4. **Tape/EFIS** — pitch tape + roll scale on top, fixed aircraft symbol.
Implementation approach: keep ONE `attitude_*` API; internally switch on a
`static uint8_t attitudeStyle` (0–3). Persist to SPIFFS (`/attitude_style.txt`).
Rebuild the screen objects on style change (delete + re-init container).
Reference geometry/code: repo root `avionics.txt` (miercemk, MPU6050+QMC5883L,
same math domain — port the drawing geometry, not the framebuffer code).

### P3. Firmware: style/calibration command routing
Add to `ota_update.cpp` web server AND to the BLE notify write handler
(`ble_notify.cpp` `NotifyCallbacks::onWrite` currently only stores toast text —
extend it to parse `cmd:`-prefixed strings):
- `attitude_style=N` (0–3) → set + persist + live re-render
- `factory_zero` → `sensors_factory_zero_orientation()`
- `factory_magcal=SECONDS` → `sensors_factory_mag_calibrate(s)`
- `declination=F` → `sensors_set_declination(f)`
- `debug_sensors` → reply with `sensors_debug_string()` (HTTP body / BLE notify)
Wire the same commands into the app's `DeviceService.routeCommand()`.

### P4. Firmware: restyle remaining widgets to black-minimal
`clock.cpp` (keep 3 faces), `gforce.cpp`, `music.cpp`, `airplane.cpp`:
pure-black backgrounds, white primary text, single restrained accent (red
`0xE01020` like the compass needle, or steel-blue `0x6FA8C7`), remove heavy
bezel rings/colour fills. Do NOT change their public APIs or update signatures.

### P5. App: rebuild the Designer into a real editor (biggest app task)
Current editor is basic and has a KNOWN BUG the user hit: **dragging an element
pans the whole screen** — the designer lives inside the AppShell `PageView`,
whose horizontal-drag gesture wins. Fixes:
- Wrap the canvas in a gesture arena winner (e.g. `RawGestureDetector` with
  eager `PanGestureRecognizer`) or set the PageView `physics:
  NeverScrollableScrollPhysics()` while the Designer tab is active (simplest:
  expose a flag from DesignerScreen via provider; AppShell switches physics).
Feature-rich requirements (user explicitly asked; current one felt "half built"):
- Selection handles with **resize** (corner drag) and optional rotate.
- Snap-to-grid + centre guides; nudge arrows; z-order (bring front/send back).
- Duplicate + delete; multi-layout save/load (list, rename) via
  SharedPreferences (`custom_layouts_*` keys already started).
- Full property editing for every element type in `widget_layout.dart`
  (bind, min/max, font S/M/L, colour palette, text, icon token, shape, filled).
- Live preview values already animate via ticker — keep.
- Import current device layout (optional GET route would need firmware
  `GET /api/custom_layout` returning the saved JSON — add it, trivial).
The JSON contract (`CwType`/`CwBind` token names) MUST NOT drift from firmware
`custom_widget.h` — that contract is the compatibility backbone.

### P6. App: parity + factory calibration + style picker screens
- **Attitude style picker**: watch-face-style horizontal carousel of the 4
  styles (render previews with CustomPainter), sends `attitude_style=N`.
- **Factory Calibrate flow** (app → device): guided 2-step wizard:
  1) "Park on level ground" → send `factory_zero`
  2) "Rotate device in figure-8 for 20s" → send `factory_magcal=20`, progress bar
  Show `debug_sensors` output live if available.
- **Parity**: home dashboard mirrors the real widget set (linear compass strip,
  minimal alt/temp) — reuse `widget_painters.dart`, restyle to black/minimal so
  app and gadget look IDENTICAL (explicit user requirement: "it has to be true").
- Merge duplicate themes: keep ONE premium silver/chrome "Star Trail" theme
  (with real emblem) + Illuminati. Remove the redundant cyan/benz split
  (`AppThemeMode` currently `{benz, starTrail, illuminati}` → collapse benz+
  starTrail; update `theme_provider`, `logo_widgets`, welcome, tests).
- Premium polish pass on all screens (spacing, typography, no default-Material
  leftovers). Mercedes-grade.

### P7. Tests + verification (before ANY release)
- Firmware: arduino-cli compile green; simulator `--shots` → visually check all
  widgets incl. 4 attitude styles (add sim keys 1–4 to switch styles).
- Flutter: `flutter analyze` 0 issues; `flutter test` all green. Add tests:
  layout JSON round-trip vs firmware contract tokens, routeCommand mappings for
  new commands, designer smoke (drag doesn't throw), style-picker sends command.
- Update `BenzCluster/docs/widgets/*.png` gallery + README (already has the
  regeneration workflow).

### P8. Release (LAST — only when all above verified)
- Firmware: build `.bin` (`BenzCluster/build/...` from arduino-cli `--output-dir`),
  name `StarTrail-firmware-v2.0.0.bin`.
- App: `flutter build apk --release --build-name=2.0.0 --build-number=3`.
- `gh release create v2.0.0 --target V11` with BOTH artifacts + full notes.
  (gh auth works in this environment; transient DNS failures happen — retry.)
- Commit everything on `claude/compassionate-moser-ec21ce`, PR into `V11`.

## 5. Contracts & gotchas (memorise)

- **Layout JSON** (app↔firmware↔simulator): see `BenzCluster/custom_widget.h`
  + `flutter_app/lib/models/widget_layout.dart`. Types:
  gauge/readout/label/bar/icon/image/shape; binds: heading/pitch/roll/temp/
  altitude/pressure ('' = none). Colours `#RRGGBB`. Screen space 240×240.
- **BLE**: data notify char `00001111-...-7778` (JSON sensor blob); layout push
  char `00002222-...-8889` opcodes 'B' begin /'C' chunk /'F' finish; notify
  service `deadbeef-cafe-...` for text. Keep UUIDs.
- **LVGL heap**: firmware + simulator both use `LV_MEM_CUSTOM 1` (malloc). The
  sim once crashed from the 64KB pool — don't regress.
- **Simulator colours**: RGB565, SWAP=0. Don't "fix" to 32-bit.
- **`lv_conf.h`**: firmware's copy uses ESP32-only attributes → MSVC can't
  parse it; the simulator keeps its own portable copy in sync. Montserrat
  8–48 enabled in both.
- **Worktree**: all work happens in
  `.claude/worktrees/compassionate-moser-ec21ce`; push to
  `claude/compassionate-moser-ec21ce`, PR base `V11` (NOT master — unrelated
  history).
- **Windows notes**: bash tool is Git Bash; flutter at `C:\tools\flutter\bin`;
  MSVC vcvars script path is inside `build_sim.bat`. `%~dp0` trailing-slash
  quoting broke once — script now uses absolute paths.
- Real Mercedes emblem is trademarked — personal-use build, don't publish to
  Play Store.
- WiFi creds are placeholder-sensitive history (`config.h`) — never commit real
  secrets; a hardening spec exists in `docs/superpowers/specs/`.

## 6. Definition of "shipped"

1. All P1–P7 done, compile/tests/screenshots green.
2. User flashes firmware, reports heading/attitude behaviour; axis signs tuned
   if needed (expect one iteration).
3. v2.0.0 GitHub release with firmware .bin + APK.
4. README gallery + docs current.
