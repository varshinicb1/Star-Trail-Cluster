# Car Dashboard Widget Redesign + Sensor Rework — Design

## Goal
Redesign the instrument cluster widgets for a real car dashboard (black, minimalist,
premium) and rebuild the sensor/heading foundation so it is **accurate and fully
hands-off for end customers**. The unit mounts in a 25–30° angled enclosure; a
one-time factory calibration (done by the builder, not the customer) makes every
shipped unit correct out of the box.

Build order (user decision): spec everything → build the compass first (verify in the
LVGL simulator) → then the rest.

## Hardware (confirmed)
External I2C module on `Wire` (SDA 38 / SCL 39):
- **MPU6050** @ 0x68 — 6-axis accel + gyro (NO internal magnetometer).
- **QMC5883L** @ 0x0D — magnetometer.
- **BME280** @ 0x76 — pressure/temp/humidity (altitude, temperature).

The current firmware targets MPU9250 + internal AK8963 — this is wrong for the actual
hardware and must be replaced.

## 1. Sensor & fusion layer (foundation — every widget depends on it)
New `sensors.cpp/h` implementation:
- **MPU6050 driver**: raw accel (g) + gyro (°/s), gyro bias auto-zeroed at boot (device
  still for ~2 s).
- **Pitch/roll fusion**: complementary filter (accel for absolute reference, gyro for
  smoothness), ~100 Hz, tunable alpha. Output is *vehicle* pitch/roll after the mount
  offset is removed (see calibration).
- **QMC5883L driver**: raw magnetometer, continuous mode.
- **Heading pipeline**:
  1. Apply hard-iron offset + soft-iron scale (from factory cal).
  2. **Tilt-compensate** using the fused gravity vector so heading is correct while the
     vehicle pitches/rolls (standard tilt-comp: rotate mag by −pitch/−roll, then
     `atan2`).
  3. Add magnetic **declination** (configurable, default Bangalore −1.09°) → **true
     north** heading, 0–360°.
  4. Light low-pass + wrap-aware smoothing so the compass scale glides, no jitter/jumps
     across the 359°→0° seam.

## 2. One-time factory calibration (builder does once per unit; customers never touch)
Stored in SPIFFS/flash, survives reboot. Triggered from the **companion app**
("Factory Calibrate" action) which walks through:
1. **Orientation zero** — car on level ground, device in its final mount. Capture accel
   → derive the fixed mount rotation (the 25–30° tilt). Saved as `mountPitchOffset` /
   `mountRollOffset`. Subtracted from all pitch/roll and used in heading tilt-comp, so
   attitude reads true vehicle attitude and heading is correct despite the angled mount.
2. **Figure-8 magnetometer cal** — collect samples while the unit is rotated through all
   orientations; compute hard-iron (offset) + soft-iron (scale). Saved.
Clear on-screen + in-app guidance during each step. After calibration the unit is
perfect with zero customer interaction. (Realistic note: a car has ferrous/electrical
interference; the figure-8 cal at install is what makes heading reliable — a purely
passive "no cal ever" approach would drift, so the one-time factory step is the design.)

## 3. Widgets (all pure-black background, minimalist colours)

### 3a. Compass — linear scrolling (BUILD FIRST)
- Horizontal **white** scale that scrolls; a **fixed red needle** always centered.
- Precise ticks: major at N/NE/E/SE/S/SW/W/NW (cardinal/intercardinal labels) + minor
  degree marks; numbers where appropriate. Wraps seamlessly 0↔360.
- Default visible span ≈ 90° across the screen width; scale moves smoothly with heading.
- **Pure black** background, **white** scale/labels, **red** center needle.
- Heading value in **white** text below the scale (e.g. `142°`), from the fused
  true-north heading.

### 3b. Alt / Temp — minimalist
- Black screen. Altitude icon + altitude value (ft), thermometer icon + temperature (°C),
  both from BME280. Clean white/minimal styling.

### 3c. Attitude indicator — multiple selectable styles
Four styles, all accurate and smooth, **Classic round ICAO = default**. User selects
which style like a watch face **from the companion app**:
- **Classic round ICAO** (default) — circular horizon, blue sky / brown ground, pitch
  ladder, bank arc with 10/20/30/45/60° ticks, fixed sky pointer + moving roll pointer.
- **Full-screen horizon** — sky/ground fills the whole 240×240, no bezel.
- **Minimalist line** — black bg, white horizon line + pitch ladder + roll arc, no fill.
- **Tape / EFIS** — pitch tape + roll scale at top, fixed aircraft symbol center.
Selection mechanism: app picker → BLE/WiFi command (e.g. `attitude_style=N`) → firmware
stores choice in flash, applied on the attitude screen.

### 3d. Remaining existing widgets
Restyle Clock, G-Force, Music, Airplane to the same black/minimal aesthetic; keep
functionality.

## 4. Companion app changes
- **Factory Calibrate** flow (orientation zero + figure-8, with progress/guidance).
- **Attitude style picker** (watch-face-style selector) that pushes the choice to the
  device.
- Both reuse the existing BLE-primary / WiFi-fallback command channel.

## 5. Verification
- Each widget rendered in the **LVGL PC simulator** (`simulator/ --shots`) with
  representative sensor values, visually reviewed before hardware.
- Sensor math unit-checked where possible off-device (tilt-comp, wrap smoothing).
- On-hardware behaviour (real heading/attitude) validated by the user after flashing —
  not possible in this environment.

## Out of scope
- Physical flashing/on-car validation this session (no board here).
- GPS-based heading (no GPS on this hardware).

## Notes / related
- Also pending from earlier: merge the redundant "Benz" and "Star Trail" app themes into
  a single premium "Star Trail" theme (silver/chrome + real emblem), leaving Star Trail
  (Mercedes) + Illuminati (VW). Small, tracked separately from this widget work.
