# Custom Widget Design Pipeline (Sub-project 2)

## Goal
Let the user design custom cluster widgets (gauges, readouts, text, bars, icons, images, shapes) on the phone, on a free-form canvas, and push the finished layout to the physical device — analogous to a smartwatch face designer. This is the largest piece of work in the overall effort and is built in four ordered stages because each depends on the previous one.

## Context
- Today, every widget (`attitude.cpp`, `compass.cpp`, `gforce.cpp`, etc.) is hand-written C++ LVGL code, compiled into firmware. There is no runtime-configurable widget concept at all — this is new capability, not a refactor.
- `simulator/` already has a minimal LVGL PC-simulator scaffold (`main.cpp`, `lv_conf.h`, CMake build) — currently not wired to any real widget code, but it's the right foundation to extend for visual verification without hardware.
- Firmware already runs NimBLE with a custom data-characteristic pattern (`ble_notify.cpp`: service/characteristic UUIDs, `WRITE`/`NOTIFY` properties) — the widget-push channel follows the same pattern rather than inventing new BLE plumbing.
- The existing web dashboard (`ota_update.cpp`) has authenticated HTTP POST routes (post-hardening) — the WiFi fallback path reuses this server rather than standing up a second one.

## Stage 1 — Widget definition format + on-device dynamic renderer (firmware)
- Define a compact binary widget-description format: a flat list of elements, each with `type` (gauge | readout | text | bar | icon | image | shape), position, size, data-source binding (which sensor/value feeds it, or static), color, and type-specific params (e.g. gauge min/max/arc range). JSON is the authoring format app-side; firmware receives/stores a compiled binary form to keep flash/RAM usage low on-device.
- Write a new `custom_widget.cpp/h` module: parses the binary format and drives LVGL primitives (`lv_arc`, `lv_label`, `lv_bar`, `lv_img`, `lv_line`/`lv_poly` for shapes) to render it — one generic interpreter, not per-design generated code.
- Data-source binding maps to the existing sensor read functions already exposed by `sensors.cpp` (IMU, BME280, etc.) — no new sensor plumbing, just a lookup table from binding name to existing getter.
- Store the active custom layout in SPIFFS so it survives reboot, alongside the existing calibration files.
- v1 element set (per user's choice): circular gauge, numeric readout, text label, bar/progress, icon (from a small bundled icon set), static image (small bitmap, size-capped for flash), simple shape (line/rect/circle).

## Stage 2 — LVGL PC simulator extended for visual verification
- Wire `simulator/main.cpp` to load the same `custom_widget.cpp` interpreter used on-device (shared source, host-compiled against LVGL's SDL/PC driver instead of the ST7789V driver), so a widget definition renders identically on desktop and hardware.
- Add a way to load a widget-definition JSON/binary file into the simulator and render it in a window.
- This becomes the verification loop: every new element type or layout gets rendered here and screenshotted for the user to visually approve before Stage 3/4 work depends on it — directly satisfies the "run an LVGL emulator and visually verify" requirement.

## Stage 3 — App-side drag/drop canvas designer
- New Flutter screen: a canvas sized to the device's 240×240 round display, with a palette of the Stage-1 element types.
- Drag to place, resize/rotate handles, tap to select and edit an element's properties panel (data-source binding, color, gauge range, etc.).
- Live preview renders using Flutter's own canvas (`CustomPainter`, similar to existing `widget_painters.dart`) — does not depend on the LVGL simulator; that's for firmware-side verification, this is for in-app authoring feedback.
- Serializes the design to the same JSON format Stage 1 consumes, stored locally (multiple saved layouts, one "active" at a time).

## Stage 4 — Push pipeline (BLE primary, WiFi fallback)
- BLE: new characteristic on the existing NimBLE server (`ble_notify.cpp` pattern) accepting chunked binary writes of the compiled widget definition (definitions are small — well within BLE MTU with basic chunking, no need for a complex transfer protocol).
- WiFi fallback: new authenticated POST route on the existing web dashboard server accepting the same binary payload, used automatically when BLE write fails or the device isn't in range but is on WiFi.
- App-side: single "Push to Device" action triggers BLE attempt first, falls back to WiFi HTTP if BLE unavailable/fails, surfaces clear success/failure state to the user.

## Out of scope
- Cloud sync / sharing designs between users.
- More than one active custom layout on-device at a time (switching between saved designs is an app-side concept; device only ever holds the currently pushed one).
- Animations/transitions within custom widgets (static/live-data-bound only in v1).
- Physical hardware testing (no board available this session) — Stage 1/2 verified via the PC simulator; Stage 4's on-device BLE/WiFi paths are compile-verified and logically reviewed only until hardware is available.

## Verification
- Stage 1: `arduino-cli compile` succeeds with the new module linked in; a hand-written sample widget-definition binary renders correctly in the Stage 2 simulator.
- Stage 2: user visually approves rendered output for each element type before Stage 3 begins consuming the format.
- Stage 3: Flutter canvas produces JSON that round-trips through the same parser used in Stage 1/2 without errors.
- Stage 4: BLE and WiFi push paths reviewed for correctness (chunking, auth, failure handling); full end-to-end push-to-real-device verification deferred until hardware is available.
