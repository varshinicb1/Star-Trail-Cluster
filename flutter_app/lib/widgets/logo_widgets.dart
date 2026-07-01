import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../theme/app_theme.dart';
import '../providers/theme_provider.dart';

class StarTrailLogoPainter extends CustomPainter {
  final Color color;
  final Color? glowColor;

  StarTrailLogoPainter({required this.color, this.glowColor});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final r = math.min(size.width, size.height) / 2;
    final outerR = r * 0.88;
    final innerR = r * 0.50;

    if (glowColor != null) {
      canvas.drawCircle(
        center,
        outerR * 0.8,
        Paint()
          ..color = glowColor!.withAlpha(25)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 12),
      );
    }

    canvas.drawCircle(
      center,
      outerR,
      Paint()
        ..color = color
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.8,
    );

    canvas.drawCircle(
      center,
      innerR,
      Paint()
        ..color = color.withAlpha(160)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.2,
    );

    final starPaint = Paint()
      ..color = color
      ..strokeWidth = 2.0
      ..strokeCap = StrokeCap.round;

    for (int i = 0; i < 3; i++) {
      final angle = -math.pi / 2 + i * 2 * math.pi / 3;
      final dx = math.cos(angle) * innerR;
      final dy = math.sin(angle) * innerR;
      canvas.drawLine(center, Offset(center.dx + dx, center.dy + dy), starPaint);
    }
  }

  @override
  bool shouldRepaint(covariant StarTrailLogoPainter old) =>
      old.color != color || old.glowColor != glowColor;
}

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

/// Premium brushed-chrome three-point star inside a dual ring — the flagship
/// "Benz" emblem, brand-inspired (not the trademarked Mercedes logo).
class BenzStarPainter extends CustomPainter {
  final Color color;
  final Color? glowColor;

  BenzStarPainter({required this.color, this.glowColor});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final r = math.min(size.width, size.height) / 2;
    final rOuter = r * 0.90;
    final rInner = r * 0.62;

    if (glowColor != null) {
      canvas.drawCircle(
        center,
        rOuter,
        Paint()
          ..color = glowColor!.withAlpha(30)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 14),
      );
    }

    // dual concentric rings
    canvas.drawCircle(
      center,
      rOuter,
      Paint()
        ..color = color
        ..style = PaintingStyle.stroke
        ..strokeWidth = r * 0.05,
    );
    canvas.drawCircle(
      center,
      rInner,
      Paint()
        ..color = color.withAlpha(150)
        ..style = PaintingStyle.stroke
        ..strokeWidth = r * 0.03,
    );

    // three tapered wedges forming the tri-star
    final fill = Paint()
      ..color = color
      ..style = PaintingStyle.fill;
    final half = 12 * math.pi / 180;
    for (int i = 0; i < 3; i++) {
      final a = -math.pi / 2 + i * 2 * math.pi / 3;
      final tip = Offset(center.dx + math.cos(a) * rInner, center.dy + math.sin(a) * rInner);
      final l = Offset(
        center.dx + math.cos(a - half) * rInner * 0.30,
        center.dy + math.sin(a - half) * rInner * 0.30,
      );
      final rr = Offset(
        center.dx + math.cos(a + half) * rInner * 0.30,
        center.dy + math.sin(a + half) * rInner * 0.30,
      );
      final wedge = Path()
        ..moveTo(tip.dx, tip.dy)
        ..lineTo(l.dx, l.dy)
        ..lineTo(center.dx, center.dy)
        ..lineTo(rr.dx, rr.dy)
        ..close();
      canvas.drawPath(wedge, fill);
    }
    // centre hub
    canvas.drawCircle(center, rInner * 0.14, fill);
  }

  @override
  bool shouldRepaint(covariant BenzStarPainter old) =>
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

    // The Benz theme uses the real chrome emblem asset; the other themes use
    // their vector painters.
    Widget child;
    if (mode == AppThemeMode.benz) {
      child = SizedBox(
        key: const ValueKey('benz-emblem'),
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
      final CustomPainter painter = mode == AppThemeMode.starTrail
          ? StarTrailLogoPainter(color: logoColor, glowColor: glow)
          : IlluminatiLogoPainter(color: logoColor, glowColor: glow);
      child = CustomPaint(
        key: ValueKey(mode),
        size: Size(size, size),
        painter: painter,
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
