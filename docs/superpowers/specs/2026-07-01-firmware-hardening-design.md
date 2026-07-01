# Star Trail Firmware Hardening — Design

## Goal
Make the existing ESP32-S3 "Star Trail" instrument cluster firmware (`BenzCluster/`) production-grade: secure, better organized, and covered by automated checks — without a rewrite. Hardware (physical CrowPanel board) is not available for this work, so all verification is compile-time / host-side.

## Context
- ~13.7k lines across `BenzCluster/`, organized one `.cpp/.h` pair per widget — structure is already reasonable, not a rewrite candidate.
- `config.h` contains a **real WiFi password in plaintext**, committed across multiple commits, and pushed to a public GitHub remote (`Star-Trail-Cluster`). Treat as compromised.
- The web dashboard (`ota_update.cpp`) exposes `/reboot`, `/calibrate`, `/api/gyro_reset`, and firmware upload with **no authentication**, and several state-changing actions are GET requests (CSRF-able).
- No automated tests, no CI.

## Scope (in priority order)

### 1. Secrets & credential hygiene
- Introduce `BenzCluster/secrets.h` (gitignored) holding `WIFI_SSID`, `WIFI_PASSWORD`, `WEATHER_API_KEY`. Commit `secrets.h.example` as a template with placeholder values.
- `config.h` keeps only non-sensitive pin/hardware/location constants and `#include "secrets.h"`.
- Update `.gitignore` to exclude `secrets.h`.
- Purge the real credential from git history (all commits) using `git filter-repo`, then force-push to `origin`. **This step requires explicit user confirmation at execution time** since it rewrites shared history on a public remote.
- User must rotate the real WiFi password out-of-band — the repo history rewrite does not undo prior exposure.

### 2. Web dashboard / OTA authentication
- Add HTTP Basic Auth to all `webServer->on(...)` routes in `ota_update.cpp`, gated by a credential stored in `secrets.h` (`DASHBOARD_USER` / `DASHBOARD_PASSWORD`).
- Convert state-changing routes currently on `HTTP_GET` (`/reboot`, `/calibrate`, `/api/gyro_reset`, `/api/ntp_sync`) to `HTTP_POST` to remove basic CSRF-via-link exposure. Update the dashboard's JS fetch calls accordingly.
- Leave read-only routes (`/api/status`, `/api/log`) as GET.

### 3. Config/architecture cleanup
- Keep the existing one-file-per-widget layout as-is (it's already a reasonable boundary).
- Move runtime-tunable values currently baked into `config.h` (sea-level pressure fallback, magnetic declination, lat/lon) into the existing SPIFFS-backed calibration/settings files where a runtime equivalent already exists, so redeploying to a new location doesn't require recompiling. Constants with no existing runtime path stay in `config.h` as documented defaults.
- No broader restructuring — not needed given current file boundaries are already coherent.

### 4. Host-side testing & CI
- Extract pure-logic calculations that don't touch hardware — heading/declination math, pressure→altitude conversion, calibration curve fitting — into headers with no Arduino/hardware dependencies, if not already separated.
- Add a PlatformIO `native` test environment (or a minimal standalone Makefile + a small C++ test runner if PlatformIO isn't desired) to unit-test that extracted logic on the host.
- Add a GitHub Actions workflow that runs `arduino-cli compile` for the existing board target on every push/PR, so compile breakage is caught automatically. This is the highest-value, fully-host-verifiable piece since hardware isn't available.

## Out of scope
- Physical hardware testing/flashing (no board available this session).
- Companion Flutter app changes.
- Hardware/PCB (Eagle/KiCad) changes.
- Any new features or widgets.

## Verification
- `arduino-cli compile` succeeds for the existing target after all changes.
- New host-side unit tests pass locally (`pio test -e native` or equivalent).
- `git log -p -- BenzCluster/config.h` (and secrets.h if ever added) shows no plaintext credentials after history rewrite.
- Manual review of `ota_update.cpp` confirms every route requires auth and state-changing routes are POST.
