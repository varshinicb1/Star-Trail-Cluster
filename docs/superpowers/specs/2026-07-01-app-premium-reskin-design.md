# Star Trail App — Premium Mercedes Reskin (Sub-project 1)

## Goal
Replace the current template-feeling visual identity of the Flutter companion app (`flutter_app/`) with a premium, automotive-grade black/gunmetal/chrome aesthetic evocative of a Mercedes-Benz cockpit — without using the actual protected Mercedes-Benz logo. Functional scope of the app is unchanged; this is purely visual/brand.

## Context
- App already has a working 3-theme system (`lib/theme/app_theme.dart`: Star Trail cyan, Illuminati red, DR purple) with a clean `ThemeData` builder — the mechanism is sound, colors/assets are the problem.
- Android manifest currently uses `android:label="star_trail"` and the stock Flutter launcher icon (`mipmap/ic_launcher.png` — default Flutter logo), which is the main source of the "kid's app" impression.
- No custom fonts, no custom app icon, no splash screen branding on the Flutter side (the ESP32 firmware has its own splash themes — unrelated, out of scope here).
- User will supply an actual logo/wordmark asset file later; until provided, the app ships with a brand-inspired abstract mark, not a placeholder.

## Scope

### 1. New theme: `AppThemeMode.premiumBlack` (or replace `neutral`)
- Palette: near-black surface (`#0A0A0C`), gunmetal cards (`#1C1C1F`), chrome/silver gradient accents (`#C8C9CC` → `#8E9195`), sparing use of a cool steel-blue highlight for interactive/success states, muted red only for warnings/errors — restrained, not neon.
- Typography: swap default Material font for a geometric/technical sans (e.g. `Inter` or `Roboto Condensed` via `google_fonts` or bundled font asset) with tighter letter-spacing on headers to read as "instrument cluster," not "consumer app."
- Chrome-gradient treatment reusable as a `BoxDecoration` helper for key surfaces (app bar, primary buttons, connect badge) — implemented once, applied consistently, not one-off per screen.
- This becomes the default theme; existing Star Trail / Illuminati themes remain selectable in `config_screen.dart`'s theme picker, DR/neutral renamed or retired since premiumBlack supersedes its intent.

### 2. App icon & branding
- Replace `android/app/src/main/res/mipmap-*/ic_launcher.png` (all densities) with a generated icon using the brand-inspired abstract mark (silver tri-point/star motif on black), produced via `flutter_launcher_icons` package for correct multi-density generation rather than hand-placed PNGs.
- Update `android:label` in `AndroidManifest.xml` from `star_trail` to a proper display name (e.g. "Star Trail").
- Add a native splash screen (via `flutter_native_splash` package) matching the new dark theme instead of the default white Flutter splash flash.
- Logo file: placeholder abstract mark ships now; swapped for user-supplied asset when provided (single asset-replacement step, no code change needed if same file name/format).

### 3. Screen-level polish pass
- Apply new theme across all existing screens (home, device controls, LED, system, widget, OTA, config, welcome) — no new screens, no removed screens (per earlier decision to keep all current screens).
- Audit each screen for leftover default Material widgets/spacing that read as "unstyled" (default `ElevatedButton` shapes, default `SnackBar`, etc.) and bring them in line with the theme's card/border/radius conventions already defined in `AppTheme.themeData`.

## Out of scope
- Any new functionality, screens, or navigation changes.
- Widget designer / custom widget pipeline (Sub-project 2).
- Firmware changes.
- Actual Mercedes-Benz trademarked logo — not sourced or embedded until user provides their own asset file.

## Verification
- `flutter analyze` passes with no new warnings.
- App builds for Android (`flutter build apk --debug`) once Flutter SDK is installed.
- Visual review: screenshot each of the 8 screens in the new theme and walk through with user for approval (no physical device required — Android emulator or `flutter run -d chrome` for quick iteration).
