import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../theme/app_theme.dart';
import '../widgets/logo_widgets.dart';

class WelcomeScreen extends StatefulWidget {
  const WelcomeScreen({super.key});
  @override
  State<WelcomeScreen> createState() => _WelcomeScreenState();
}

class _WelcomeScreenState extends State<WelcomeScreen>
    with TickerProviderStateMixin {
  late final AnimationController _entranceController;
  late final AnimationController _pulseController;
  late final Animation<double> _fadeIn;
  late final Animation<double> _logoScale;
  late final Animation<Offset> _slideUp;
  late final Animation<double> _pulse;

  @override
  void initState() {
    super.initState();
    _entranceController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1200),
    )..forward();

    _pulseController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 2200),
    )..repeat(reverse: true);

    _fadeIn = Tween<double>(begin: 0, end: 1).animate(
      CurvedAnimation(
        parent: _entranceController,
        curve: const Interval(0, 0.5, curve: Curves.easeOut),
      ),
    );

    _logoScale = Tween<double>(begin: 0.3, end: 1).animate(
      CurvedAnimation(
        parent: _entranceController,
        curve: const Interval(0, 0.6, curve: Curves.easeOutBack),
      ),
    );

    _slideUp = Tween<Offset>(
      begin: const Offset(0, 0.4),
      end: Offset.zero,
    ).animate(
      CurvedAnimation(
        parent: _entranceController,
        curve: const Interval(0.2, 0.6, curve: Curves.easeOutCubic),
      ),
    );

    _pulse = Tween<double>(begin: 0.95, end: 1.05).animate(
      CurvedAnimation(parent: _pulseController, curve: Curves.easeInOutSine),
    );
  }

  @override
  void dispose() {
    _entranceController.dispose();
    _pulseController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [theme.gradientStart, theme.gradientEnd],
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              const Spacer(flex: 2),
              AnimatedBuilder(
                animation: _logoScale,
                builder: (_, child) => Transform.scale(
                  scale: _logoScale.value,
                  child: child,
                ),
                child: AnimatedBuilder(
                  animation: _pulse,
                  builder: (_, child) => Transform.scale(
                    scale: _pulse.value,
                    child: child,
                  ),
                  child: const ThemeLogo(size: 120, animate: true),
                ),
              ),
              const SizedBox(height: 28),
              FadeTransition(
                opacity: _fadeIn,
                child: SlideTransition(
                  position: _slideUp,
                  child: Column(
                    children: [
                      Text(
                        'Welcome',
                        style: TextStyle(
                          color: Colors.white.withAlpha(200),
                          fontSize: 26,
                          fontWeight: FontWeight.w300,
                          letterSpacing: 6,
                        ),
                      ),
                      const SizedBox(height: 4),
                      Text(
                        'STAR TRAIL',
                        style: TextStyle(
                          color: Colors.white,
                          fontSize: 40,
                          fontWeight: FontWeight.w700,
                          letterSpacing: 4,
                          height: 1.1,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 14),
              FadeTransition(
                opacity: _fadeIn,
                child: Column(
                  children: [
                    Text(
                      theme.name,
                      style: TextStyle(
                        color: theme.primary,
                        fontSize: 15,
                        fontWeight: FontWeight.w600,
                        letterSpacing: 1,
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      theme.welcomeSubtitle,
                      style: TextStyle(
                        color: Colors.white.withAlpha(140),
                        fontSize: 12,
                        letterSpacing: 1,
                      ),
                    ),
                  ],
                ),
              ),
              const Spacer(flex: 2),
              Consumer<ThemeProvider>(
                builder: (_, tp, _) => Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 20),
                  child: Row(
                    children: AppThemeMode.values.map((mode) {
                      final t = AppTheme.fromMode(mode);
                      final selected = tp.mode == mode;
                      return Expanded(
                        child: GestureDetector(
                          onTap: () => tp.setTheme(mode),
                          child: AnimatedContainer(
                            duration: const Duration(milliseconds: 350),
                            curve: Curves.easeOut,
                            margin: const EdgeInsets.symmetric(horizontal: 5),
                            padding: const EdgeInsets.symmetric(
                              vertical: 14,
                              horizontal: 4,
                            ),
                            decoration: BoxDecoration(
                              color: selected
                                  ? Colors.white.withAlpha(15)
                                  : Colors.white.withAlpha(4),
                              borderRadius: BorderRadius.circular(18),
                              border: Border.all(
                                color: selected
                                    ? t.primary
                                    : Colors.white.withAlpha(20),
                                width: selected ? 2 : 1,
                              ),
                              boxShadow: selected
                                  ? [
                                      BoxShadow(
                                        color: t.primary.withAlpha(40),
                                        blurRadius: 12,
                                        spreadRadius: 0,
                                      ),
                                    ]
                                  : null,
                            ),
                            child: Column(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                SizedBox(
                                  width: 40,
                                  height: 40,
                                  child: CustomPaint(
                                    size: const Size(40, 40),
                                    painter: _logoPainter(mode, t.primary),
                                  ),
                                ),
                                const SizedBox(height: 8),
                                Container(
                                  width: 8,
                                  height: 8,
                                  decoration: BoxDecoration(
                                    shape: BoxShape.circle,
                                    color: t.primary,
                                    boxShadow: selected
                                        ? [
                                            BoxShadow(
                                              color: t.primary.withAlpha(160),
                                              blurRadius: 8,
                                            ),
                                          ]
                                        : null,
                                  ),
                                ),
                                const SizedBox(height: 6),
                                Text(
                                  _themeLabel(mode),
                                  style: TextStyle(
                                    color: selected
                                        ? t.primary
                                        : Colors.white.withAlpha(140),
                                    fontSize: 10,
                                    fontWeight: selected
                                        ? FontWeight.w600
                                        : FontWeight.w400,
                                    letterSpacing: 0.5,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                      );
                    }).toList(),
                  ),
                ),
              ),
              const SizedBox(height: 28),
              Consumer<ThemeProvider>(
                builder: (_, tp, _) => Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 40),
                  child: SizedBox(
                    width: double.infinity,
                    height: 54,
                    child: ElevatedButton(
                      onPressed: () =>
                          Navigator.pushReplacementNamed(context, '/home'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: tp.theme.primary,
                        foregroundColor: tp.theme.onPrimary,
                        elevation: 0,
                        shadowColor: tp.theme.glow?.withAlpha(80),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(16),
                        ),
                      ),
                      child: const Text(
                        'Get Started',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.w600,
                          letterSpacing: 1,
                        ),
                      ),
                    ),
                  ),
                ),
              ),
              const SizedBox(height: 40),
            ],
          ),
        ),
      ),
    );
  }

  String _themeLabel(AppThemeMode mode) {
    switch (mode) {
      case AppThemeMode.benz:
        return 'Benz';
      case AppThemeMode.starTrail:
        return 'Star Trail';
      case AppThemeMode.illuminati:
        return 'Illuminati';
    }
  }
}

CustomPainter _logoPainter(AppThemeMode mode, Color color) {
  switch (mode) {
    case AppThemeMode.benz:
      return BenzStarPainter(color: color, glowColor: color);
    case AppThemeMode.starTrail:
      return StarTrailLogoPainter(color: color, glowColor: color);
    case AppThemeMode.illuminati:
      return IlluminatiLogoPainter(color: color, glowColor: color);
  }
}
