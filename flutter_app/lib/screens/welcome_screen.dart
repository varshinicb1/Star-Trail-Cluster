import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../theme/app_theme.dart';
import '../widgets/logo_widgets.dart';
import '../widgets/glass_container.dart';

class WelcomeScreen extends StatefulWidget {
  const WelcomeScreen({super.key});
  @override
  State<WelcomeScreen> createState() => _WelcomeScreenState();
}

class _WelcomeScreenState extends State<WelcomeScreen> with TickerProviderStateMixin {
  late AnimationController _fadeController;
  late AnimationController _pulseController;
  late AnimationController _slideController;
  late Animation<double> _fadeAnim;
  late Animation<Offset> _slideAnim;
  bool _entered = false;

  @override
  void initState() {
    super.initState();
    _fadeController = AnimationController(vsync: this, duration: Duration(milliseconds: 1200));
    _pulseController = AnimationController(vsync: this, duration: Duration(seconds: 3));
    _slideController = AnimationController(vsync: this, duration: Duration(milliseconds: 800));
    _fadeAnim = CurvedAnimation(parent: _fadeController, curve: Curves.easeOut);
    _slideAnim = Tween<Offset>(begin: Offset(0, 0.3), end: Offset.zero).animate(
      CurvedAnimation(parent: _slideController, curve: Curves.easeOutCubic),
    );
    _fadeController.forward();
    _slideController.forward();
    _pulseController.repeat(reverse: true);
  }

  @override
  void dispose() {
    _fadeController.dispose();
    _pulseController.dispose();
    _slideController.dispose();
    super.dispose();
  }

  void _enter() {
    if (_entered) return;
    _entered = true;
    Navigator.pushReplacementNamed(context, '/home');
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
            colors: [
              theme.surface,
              theme.surfaceLight,
              theme.card,
            ],
          ),
        ),
        child: Stack(
          children: [
            _Starfield(theme: theme),
            SafeArea(
              child: FadeTransition(
                opacity: _fadeAnim,
                child: SlideTransition(
                  position: _slideAnim,
                  child: Column(
                    children: [
                      Spacer(flex: 2),
                      // Logo
                      AnimatedBuilder(
                        animation: _pulseController,
                        builder: (_, child) => Transform.scale(
                          scale: 0.92 + _pulseController.value * 0.08,
                          child: child,
                        ),
                        child: Container(
                          width: 120,
                          height: 120,
                          decoration: BoxDecoration(
                            shape: BoxShape.circle,
                            boxShadow: [
                              BoxShadow(
                                color: theme.glow?.withAlpha(25) ?? Colors.transparent,
                                blurRadius: 40,
                                spreadRadius: 8,
                              ),
                            ],
                          ),
                          child: ThemeLogo(size: 120),
                        ),
                      ),
                      SizedBox(height: 28),
                      Text(
                        'STAR TRAIL',
                        style: TextStyle(
                          fontSize: 32,
                          fontWeight: FontWeight.w200,
                          color: theme.textPrimary,
                          letterSpacing: 12,
                        ),
                      ),
                      SizedBox(height: 8),
                      Text(
                        theme.welcomeSubtitle,
                        style: TextStyle(
                          fontSize: 12,
                          color: theme.textMuted,
                          letterSpacing: 4,
                          fontWeight: FontWeight.w300,
                        ),
                      ),
                      SizedBox(height: 8),
                      Container(
                        width: 40,
                        height: 1,
                        color: theme.primary.withAlpha(80),
                      ),
                      Spacer(flex: 1),
                      // Theme selector
                      _ThemeSelector(theme: theme),
                      SizedBox(height: 40),
                      // Enter button
                      Padding(
                        padding: EdgeInsets.symmetric(horizontal: 48),
                        child: GlassButton(
                          width: double.infinity,
                          height: 56,
                          accentColor: theme.primary,
                          onTap: _enter,
                          child: Container(
                            decoration: BoxDecoration(
                              gradient: LinearGradient(
                                colors: [theme.primary.withAlpha(40), theme.primary.withAlpha(15)],
                                begin: Alignment.topLeft,
                                end: Alignment.bottomRight,
                              ),
                              borderRadius: BorderRadius.circular(14),
                            ),
                            child: Row(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Text(
                                  'ENTER',
                                  style: TextStyle(
                                    color: theme.primary,
                                    fontSize: 14,
                                    fontWeight: FontWeight.w600,
                                    letterSpacing: 4,
                                  ),
                                ),
                                SizedBox(width: 12),
                                Icon(Icons.arrow_forward, color: theme.primary, size: 18),
                              ],
                            ),
                          ),
                        ),
                      ),
                      Spacer(flex: 2),
                    ],
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _ThemeSelector extends StatelessWidget {
  final AppTheme theme;
  const _ThemeSelector({required this.theme});

  @override
  Widget build(BuildContext context) {
    final tp = context.watch<ThemeProvider>();
    return GlassContainer(
      padding: EdgeInsets.all(6),
      margin: EdgeInsets.symmetric(horizontal: 40),
      borderRadius: BorderRadius.circular(30),
      borderColor: theme.cardBorder,
      bgColor: theme.card.withAlpha(180),
      child: Row(
        children: List.generate(AppThemeMode.values.length, (i) {
          final mode = AppThemeMode.values[i];
          final t = AppTheme.fromMode(mode);
          final active = tp.mode == mode;
          return Expanded(
            child: GestureDetector(
              onTap: () => tp.setTheme(mode),
              child: AnimatedContainer(
                duration: Duration(milliseconds: 300),
                curve: Curves.easeOutCubic,
                padding: EdgeInsets.symmetric(vertical: 10),
                decoration: BoxDecoration(
                  color: active ? t.primary.withAlpha(25) : Colors.transparent,
                  borderRadius: BorderRadius.circular(24),
                  border: Border.all(
                    color: active ? t.primary.withAlpha(60) : Colors.transparent,
                    width: 1,
                  ),
                ),
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Container(
                      width: 20,
                      height: 20,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        gradient: LinearGradient(
                          colors: [t.primary, t.accent],
                        ),
                      ),
                    ),
                    SizedBox(height: 4),
                    Text(
                      t.name,
                      style: TextStyle(
                        fontSize: 10,
                        fontWeight: active ? FontWeight.w600 : FontWeight.w400,
                        color: active ? t.primary : theme.textMuted,
                        letterSpacing: 0.5,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          );
        }),
      ),
    );
  }
}

class _Starfield extends StatefulWidget {
  final AppTheme theme;
  const _Starfield({required this.theme});

  @override
  State<_Starfield> createState() => _StarfieldState();
}

class _StarfieldState extends State<_Starfield> with SingleTickerProviderStateMixin {
  late AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: Duration(seconds: 20));
    _controller.repeat();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _controller,
      builder: (context, _) => CustomPaint(
        size: Size.infinite,
        painter: _StarfieldPainter(_controller.value, widget.theme),
      ),
    );
  }
}

class _StarfieldPainter extends CustomPainter {
  final double phase;
  final AppTheme theme;
  _StarfieldPainter(this.phase, this.theme);

  @override
  void paint(Canvas canvas, Size size) {
    final rng = math.Random(42);
    for (int i = 0; i < 60; i++) {
      final x = rng.nextDouble() * size.width;
      final y = rng.nextDouble() * size.height;
      final r = rng.nextDouble() * 1.5 + 0.3;
      final alpha = ((math.sin(phase * math.pi * 2 + i * 1.7) + 1) / 2 * 0.5 + 0.2) * 255;
      canvas.drawCircle(
        Offset(x, y),
        r,
        Paint()..color = theme.textMuted.withAlpha(alpha.toInt()),
      );
    }
  }

  @override
  bool shouldRepaint(_StarfieldPainter o) => o.phase != phase;
}
