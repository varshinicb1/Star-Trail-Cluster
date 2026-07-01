import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

enum AppThemeMode { benz, starTrail, illuminati }

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

  ThemeData get themeData => ThemeData(
    brightness: brightness,
    useMaterial3: true,
    colorSchemeSeed: primary,
    scaffoldBackgroundColor: surface,
    appBarTheme: AppBarTheme(
      backgroundColor: surfaceLight,
      elevation: 0,
      centerTitle: true,
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
    ),
    cardTheme: CardThemeData(
      color: card,
      elevation: 0,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16), side: BorderSide(color: cardBorder)),
    ),
    dividerTheme: DividerThemeData(color: cardBorder, thickness: 1),
    textTheme: GoogleFonts.interTextTheme(
      TextTheme(
        headlineLarge: TextStyle(color: textPrimary, fontWeight: FontWeight.w200, letterSpacing: 1.5),
        headlineMedium: TextStyle(color: textPrimary, fontWeight: FontWeight.w300, letterSpacing: 1.0),
        titleLarge: TextStyle(color: textPrimary, fontWeight: FontWeight.w600, letterSpacing: 0.5),
        titleMedium: TextStyle(color: textPrimary, fontWeight: FontWeight.w500),
        bodyLarge: TextStyle(color: textPrimary),
        bodyMedium: TextStyle(color: textSecondary),
        bodySmall: TextStyle(color: textMuted),
      ),
    ),
    switchTheme: SwitchThemeData(
      thumbColor: WidgetStateProperty.resolveWith((states) => states.contains(WidgetState.selected) ? primary : textMuted),
      trackColor: WidgetStateProperty.resolveWith((states) => states.contains(WidgetState.selected) ? primary.withAlpha(60) : cardBorder),
    ),
    sliderTheme: SliderThemeData(
      activeTrackColor: primary,
      inactiveTrackColor: cardBorder,
      thumbColor: primary,
      overlayColor: primary.withAlpha(30),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: surface,
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: cardBorder),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: cardBorder),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(12),
        borderSide: BorderSide(color: primary, width: 1.5),
      ),
      labelStyle: TextStyle(color: textMuted, fontSize: 12),
      hintStyle: TextStyle(color: textMuted.withAlpha(150), fontSize: 13),
    ),
    chipTheme: ChipThemeData(
      backgroundColor: card,
      selectedColor: primary.withAlpha(40),
      labelStyle: TextStyle(color: textSecondary, fontSize: 12),
      side: BorderSide(color: cardBorder),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
    ),
    snackBarTheme: SnackBarThemeData(
      backgroundColor: card,
      contentTextStyle: TextStyle(color: textPrimary),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      behavior: SnackBarBehavior.floating,
    ),
    dialogTheme: DialogThemeData(
      backgroundColor: card,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
    ),
    progressIndicatorTheme: ProgressIndicatorThemeData(color: primary),
  );

  static const starTrail = AppTheme(
    name: 'Star Trail',
    welcomeSubtitle: 'Star Trail • Mercedes',
    primary: Color(0xFF00CCFF),
    secondary: Color(0xFF0088CC),
    accent: Color(0xFF00FF88),
    surface: Color(0xFF080810),
    surfaceLight: Color(0xFF0D0D1A),
    card: Color(0xFF111122),
    cardBorder: Color(0xFF1A1A33),
    textPrimary: Color(0xFFEEEEFF),
    textSecondary: Color(0xFF9999BB),
    textMuted: Color(0xFF555577),
    success: Color(0xFF00FF88),
    warning: Color(0xFFFFAA00),
    error: Color(0xFFFF4444),
    gradientStart: Color(0xFF00CCFF),
    gradientEnd: Color(0xFF0066AA),
    glow: Color(0xFF00CCFF),
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

  // Premium black / gunmetal / brushed-chrome — the flagship automotive theme.
  static const benz = AppTheme(
    name: 'Benz',
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

  static const themes = [benz, starTrail, illuminati];

  static AppTheme fromMode(AppThemeMode mode) {
    switch (mode) {
      case AppThemeMode.benz: return benz;
      case AppThemeMode.starTrail: return starTrail;
      case AppThemeMode.illuminati: return illuminati;
    }
  }
}
