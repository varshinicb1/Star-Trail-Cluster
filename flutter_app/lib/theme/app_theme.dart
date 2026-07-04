import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

enum AppThemeMode { starTrail, illuminati }

class AppTheme {
  final String name;
  final String welcomeSubtitle;
  final Color primary;
  final Color secondary;
  final Color accent;
  final Color surface;
  final Color surfaceLight;
  final Color card;
  final Color cardBorder;
  final Color textPrimary;
  final Color textSecondary;
  final Color textMuted;
  final Color success;
  final Color warning;
  final Color error;
  final Color gradientStart;
  final Color gradientEnd;
  final Color? glow;
  final Brightness brightness;

  const AppTheme({
    required this.name,
    required this.welcomeSubtitle,
    required this.primary,
    required this.secondary,
    required this.accent,
    required this.surface,
    required this.surfaceLight,
    required this.card,
    required this.cardBorder,
    required this.textPrimary,
    required this.textSecondary,
    required this.textMuted,
    required this.success,
    required this.warning,
    required this.error,
    required this.gradientStart,
    required this.gradientEnd,
    this.glow,
    required this.brightness,
  });

  /// Brushed-chrome gradient used for premium accent surfaces (buttons, badges).
  /// Runs bright highlight → accent → gunmetal low, evoking polished metal.
  /// Uses [primary]/[secondary] so it stays bright even when the full-screen
  /// [gradientStart]/[gradientEnd] background gradient is dark.
  LinearGradient get chromeGradient => LinearGradient(
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
    colors: [
      Color.lerp(primary, Colors.white, 0.35)!,
      primary,
      secondary,
    ],
    stops: const [0.0, 0.5, 1.0],
  );

  /// Foreground colour that reads cleanly on top of [primary] (dark text on a
  /// light silver primary, white on a saturated one).
  Color get onPrimary =>
      primary.computeLuminance() > 0.5 ? const Color(0xFF0A0A0C) : Colors.white;

  static const _defaultTextTheme = TextTheme(
    headlineLarge: TextStyle(fontWeight: FontWeight.w300, letterSpacing: -0.5),
    headlineMedium: TextStyle(fontWeight: FontWeight.w400, letterSpacing: -0.25),
    titleLarge: TextStyle(fontWeight: FontWeight.w600, letterSpacing: 0.15),
    titleMedium: TextStyle(fontWeight: FontWeight.w500, letterSpacing: 0.1),
    bodyLarge: TextStyle(letterSpacing: 0.3),
    bodyMedium: TextStyle(letterSpacing: 0.2),
    bodySmall: TextStyle(letterSpacing: 0.3),
    labelLarge: TextStyle(fontWeight: FontWeight.w600, letterSpacing: 0.5),
    labelSmall: TextStyle(letterSpacing: 0.5),
  );

  ThemeData get themeData => ThemeData(
    brightness: brightness,
    useMaterial3: true,
    colorSchemeSeed: primary,
    scaffoldBackgroundColor: surface,
    appBarTheme: AppBarTheme(
      backgroundColor: Colors.transparent,
      elevation: 0,
      centerTitle: true,
      scrolledUnderElevation: 0,
      titleTextStyle: GoogleFonts.inter(
        color: textPrimary,
        fontSize: 16,
        fontWeight: FontWeight.w600,
        letterSpacing: 1.5,
      ),
      iconTheme: IconThemeData(color: textPrimary),
    ),
    bottomNavigationBarTheme: BottomNavigationBarThemeData(
      backgroundColor: surfaceLight,
      selectedItemColor: primary,
      unselectedItemColor: textMuted,
      type: BottomNavigationBarType.fixed,
      elevation: 0,
      selectedLabelStyle: TextStyle(fontSize: 11, fontWeight: FontWeight.w600, letterSpacing: 0.5),
      unselectedLabelStyle: TextStyle(fontSize: 10, fontWeight: FontWeight.w400, letterSpacing: 0.3),
    ),
    cardTheme: CardThemeData(
      color: card,
      elevation: 0,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(16),
        side: BorderSide(color: cardBorder),
      ),
    ),
    dividerTheme: DividerThemeData(color: cardBorder, thickness: 1),
    // V11's richer style set, colourised, then wrapped in Inter for the
    // premium cluster typography.
    textTheme: GoogleFonts.interTextTheme(
      _defaultTextTheme.apply(
        bodyColor: textPrimary,
        displayColor: textPrimary,
      ),
    ),
    switchTheme: SwitchThemeData(
      thumbColor: WidgetStateProperty.resolveWith((s) => s.contains(WidgetState.selected) ? primary : textMuted),
      trackColor: WidgetStateProperty.resolveWith((s) => s.contains(WidgetState.selected) ? primary.withAlpha(60) : cardBorder),
    ),
    sliderTheme: SliderThemeData(
      activeTrackColor: primary,
      inactiveTrackColor: cardBorder,
      thumbColor: primary,
      overlayColor: primary.withAlpha(30),
      trackHeight: 4,
      thumbShape: RoundSliderThumbShape(enabledThumbRadius: 8),
      overlayShape: RoundSliderOverlayShape(overlayRadius: 16),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: surface,
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: cardBorder),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: cardBorder),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: primary, width: 1.5),
      ),
      labelStyle: TextStyle(color: textMuted, fontSize: 12, letterSpacing: 0.3),
      hintStyle: TextStyle(color: textMuted.withAlpha(120), fontSize: 13, letterSpacing: 0.2),
      contentPadding: EdgeInsets.symmetric(horizontal: 16, vertical: 14),
    ),
    chipTheme: ChipThemeData(
      backgroundColor: card,
      selectedColor: primary.withAlpha(40),
      labelStyle: TextStyle(color: textSecondary, fontSize: 12, letterSpacing: 0.3),
      side: BorderSide(color: cardBorder),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: card,
      contentTextStyle: TextStyle(color: textPrimary),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
      behavior: SnackBarBehavior.floating,
    ),
    dialogTheme: DialogThemeData(
      backgroundColor: card,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(24)),
    ),
    progressIndicatorTheme: ProgressIndicatorThemeData(color: primary),
    popupMenuTheme: PopupMenuThemeData(
      color: card,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
    ),
    timePickerTheme: TimePickerThemeData(
      backgroundColor: card,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
    ),
  );

  // Premium black / gunmetal / brushed-chrome — the flagship automotive theme,
  // paired with the real Mercedes emblem (see ThemeLogo). Formerly split
  // across two overlapping "Benz" and "Star Trail" themes; merged into one.
  static const starTrail = AppTheme(
    name: 'Star Trail',
    welcomeSubtitle: 'Star Trail • Premium Cluster',
    primary: Color(0xFFC8C9CC),       // brushed chrome / steel silver
    secondary: Color(0xFF8E9195),     // gunmetal
    accent: Color(0xFF6FA8C7),        // cool steel-blue highlight
    surface: Color(0xFF0A0A0C),       // near-black cockpit
    surfaceLight: Color(0xFF121216),  // raised panel
    card: Color(0xFF17171B),          // gunmetal card
    cardBorder: Color(0xFF2A2A30),    // subtle metal edge
    textPrimary: Color(0xFFF2F3F5),   // polished white
    textSecondary: Color(0xFFA9ABB0), // silver text
    textMuted: Color(0xFF6A6C72),     // muted steel
    success: Color(0xFF7FD1A6),       // restrained mint
    warning: Color(0xFFE0B15A),       // amber
    error: Color(0xFFD46A6A),         // muted red
    gradientStart: Color(0xFF1D1D22), // dark metallic (full-screen bg top)
    gradientEnd: Color(0xFF050506),   // near-black (full-screen bg bottom)
    glow: Color(0xFF8FB4CC),          // faint steel-blue glow
    brightness: Brightness.dark,
  );

  static const illuminati = AppTheme(
    name: 'Illuminati',
    welcomeSubtitle: 'Illuminati • Volkswagen',
    primary: Color(0xFFFF2200),
    secondary: Color(0xFFCC1100),
    accent: Color(0xFFFF8800),
    surface: Color(0xFF0A0808),
    surfaceLight: Color(0xFF140D0D),
    card: Color(0xFF1A0F0F),
    cardBorder: Color(0xFF331A1A),
    textPrimary: Color(0xFFFFEEEE),
    textSecondary: Color(0xFFBB9999),
    textMuted: Color(0xFF775555),
    success: Color(0xFFFF4444),
    warning: Color(0xFFFF8800),
    error: Color(0xFFFF0000),
    gradientStart: Color(0xFFFF2200),
    gradientEnd: Color(0xFFCC3300),
    glow: Color(0xFFFF4400),
    brightness: Brightness.dark,
  );

  static const themes = [starTrail, illuminati];

  static AppTheme fromMode(AppThemeMode mode) {
    switch (mode) {
      case AppThemeMode.starTrail: return starTrail;
      case AppThemeMode.illuminati: return illuminati;
    }
  }
}
