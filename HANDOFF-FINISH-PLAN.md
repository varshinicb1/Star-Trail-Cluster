# Star Trail Cluster — v2.0.0 shipped

Superseded: the P1–P8 plan this file used to track is complete and released.
See the [v2.0.0 release notes](https://github.com/varshinicb1/Star-Trail-Cluster/releases/tag/v2.0.0)
for the full changelog. Kept as a historical record only.

## What shipped in v2.0.0
- Firmware: MPU6050+QMC5883L sensor rewrite, true-north heading, factory
  calibration, linear compass, minimal alt/temp, 4 attitude styles, unified
  BLE+HTTP command API, black/minimal widget restyle.
- App: fixed broken auto-update (missing Android network permissions), fixed
  a real BLE characteristic-matching bug, fixed BLE control commands being
  silent no-ops, fixed the designer's drag-pans-screen bug, added Attitude
  Style picker + Factory Calibration wizard screens, dynamic version display.
- 23/23 tests pass, firmware compiles 2.03MB (64%), all widgets verified in
  the LVGL simulator.

## Still open (not blocking the v2.0.0 release, tracked for a future pass)
- **On-hardware validation**: tilt-comp axis signs for the MPU6050 fusion are
  compile-verified but unvalidated on a physical board (none available in this
  build environment). Flash + report; expect a possible one-line sign flip in
  `sensors.cpp` `fusePitchRoll()`/`computeHeading()`.
- **Theme dedupe**: the app still ships both "Benz" and "Star Trail" premium
  themes (overlapping Mercedes styling) — low priority cosmetic cleanup.
- **Designer polish**: resize handles, snap-to-grid, z-order, multi-layout
  management are still basic; core drag/property-edit works and the
  screen-pan bug is fixed.
- LED pattern/speed commands (`led_pattern=`, `led_speed=`) are accepted by
  the app's route table but have no firmware-side effect — no LED pattern
  engine exists yet in `leds.cpp`.
