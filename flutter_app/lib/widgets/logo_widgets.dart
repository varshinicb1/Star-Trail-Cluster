import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../theme/app_theme.dart';
import '../providers/theme_provider.dart';

class IlluminatiLogoPainter extends CustomPainter {
  final Color color;
  final Color? glowColor;

  IlluminatiLogoPainter({required this.color, this.glowColor});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final r = math.min(size.width, size.height) / 2;

    if (glowColor != null) {
      canvas.drawCircle(
        center,
        r * 0.75,
        Paint()
          ..color = glowColor!.withAlpha(25)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 10),
      );
    }

    canvas.drawCircle(
      center,
      r * 0.88,
      Paint()
        ..color = color
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2.5,
    );

    final letterPaint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2.5
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round;

    final h = r * 0.65;
    final w = r * 0.55;

    final v = Path()
      ..moveTo(center.dx - w, center.dy - h * 0.15)
      ..lineTo(center.dx, center.dy + h * 0.1)
      ..lineTo(center.dx + w, center.dy - h * 0.15);
    canvas.drawPath(v, letterPaint);

    final wShape = Path()
      ..moveTo(center.dx, center.dy + h * 0.1)
      ..lineTo(center.dx - w * 0.7, center.dy + h * 0.55)
      ..lineTo(center.dx, center.dy + h * 0.2)
      ..lineTo(center.dx + w * 0.7, center.dy + h * 0.55);
    canvas.drawPath(wShape, letterPaint);
  }

  @override
  bool shouldRepaint(covariant IlluminatiLogoPainter old) =>
      old.color != color || old.glowColor != glowColor;
}

class ThemeLogo extends StatelessWidget {
  final double size;
  final Color? color;
  final bool animate;

  const ThemeLogo({super.key, this.size = 80, this.color, this.animate = true});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    final mode = context.watch<ThemeProvider>().mode;
    final logoColor = color ?? theme.primary;
    final glow = animate ? theme.glow : null;

    // Star Trail (the merged premium theme) uses the real chrome emblem
    // asset; Illuminati uses its vector painter.
    Widget child;
    if (mode == AppThemeMode.starTrail) {
      child = SizedBox(
        key: const ValueKey('star-trail-emblem'),
        width: size,
        height: size,
        child: DecoratedBox(
          decoration: glow != null
              ? BoxDecoration(
                  shape: BoxShape.circle,
                  boxShadow: [BoxShadow(color: glow.withAlpha(60), blurRadius: size * 0.18)],
                )
              : const BoxDecoration(),
          child: Image.asset('assets/branding/emblem.png', width: size, height: size),
        ),
      );
    } else {
      child = CustomPaint(
        key: ValueKey(mode),
        size: Size(size, size),
        painter: IlluminatiLogoPainter(color: logoColor, glowColor: glow),
      );
    }

    return SizedBox(
      width: size,
      height: size,
      child: AnimatedSwitcher(
        duration: animate ? const Duration(milliseconds: 400) : Duration.zero,
        child: child,
      ),
    );
  }
}
