import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../models/widget_layout.dart';

/// Renders a [CwLayout] onto a 240x240 canvas, mirroring how the firmware's
/// `cw_render` draws each element with LVGL primitives. Used for the designer's
/// live preview so what the user arranges matches the device output.
class CustomWidgetPainter extends CustomPainter {
  final CwLayout layout;
  final Map<CwBind, double> values; // live/sample values per binding
  final CwElement? selected;

  CustomWidgetPainter({required this.layout, required this.values, this.selected});

  Color _c(int rgb) => Color(0xFF000000 | rgb);

  double _value(CwElement e) => values[e.bind] ?? e.min;

  @override
  void paint(Canvas canvas, Size size) {
    final scale = size.width / kCwScreen;
    canvas.save();
    canvas.scale(scale);

    // background disc (round display)
    final bgPaint = Paint()..color = _c(layout.bg);
    canvas.drawCircle(const Offset(kCwScreen / 2, kCwScreen / 2), kCwScreen / 2, bgPaint);

    for (final e in layout.elements) {
      _paintElement(canvas, e);
      if (identical(e, selected)) _paintSelection(canvas, e);
    }
    canvas.restore();
  }

  void _paintElement(Canvas canvas, CwElement e) {
    switch (e.type) {
      case CwType.gauge:
        _paintGauge(canvas, e);
        break;
      case CwType.bar:
        _paintBar(canvas, e);
        break;
      case CwType.readout:
        _paintText(canvas, e, _formatValue(e, _value(e)));
        break;
      case CwType.label:
        _paintText(canvas, e, e.text.isEmpty ? 'LABEL' : e.text);
        break;
      case CwType.icon:
        _paintIcon(canvas, e);
        break;
      case CwType.shape:
        _paintShape(canvas, e);
        break;
      case CwType.image:
        _paintImage(canvas, e);
        break;
    }
  }

  String _formatValue(CwElement e, double v) {
    if (e.bind == CwBind.heading || e.bind == CwBind.altitude) {
      return '${v.round()}${e.text}';
    }
    return '${v.toStringAsFixed(1)}${e.text}';
  }

  void _paintGauge(Canvas canvas, CwElement e) {
    final rect = Rect.fromLTWH(e.x, e.y, e.w, e.h).deflate(6);
    const start = 135 * math.pi / 180;
    const sweep = 270 * math.pi / 180;
    final track = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 8
      ..strokeCap = StrokeCap.round
      ..color = const Color(0xFF2A2A30);
    canvas.drawArc(rect, start, sweep, false, track);

    final frac = ((_value(e) - e.min) / (e.max - e.min)).clamp(0.0, 1.0);
    final ind = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 8
      ..strokeCap = StrokeCap.round
      ..color = _c(e.color);
    canvas.drawArc(rect, start, sweep * frac, false, ind);
  }

  void _paintBar(Canvas canvas, CwElement e) {
    final r = RRect.fromRectAndRadius(
        Rect.fromLTWH(e.x, e.y, e.w, e.h), const Radius.circular(4));
    canvas.drawRRect(r, Paint()..color = const Color(0xFF2A2A30));
    final frac = ((_value(e) - e.min) / (e.max - e.min)).clamp(0.0, 1.0);
    final fill = RRect.fromRectAndRadius(
        Rect.fromLTWH(e.x, e.y, e.w * frac, e.h), const Radius.circular(4));
    canvas.drawRRect(fill, Paint()..color = _c(e.color));
  }

  double _fontSize(int f) => f == 0 ? 14 : (f == 1 ? 22 : 34);

  void _paintText(Canvas canvas, CwElement e, String text) {
    final tp = TextPainter(
      text: TextSpan(
        text: text,
        style: TextStyle(
          color: _c(e.color),
          fontSize: _fontSize(e.font),
          fontWeight: FontWeight.w600,
        ),
      ),
      textDirection: TextDirection.ltr,
    )..layout(maxWidth: kCwScreen);
    tp.paint(canvas, Offset(e.x, e.y));
  }

  void _paintIcon(Canvas canvas, CwElement e) {
    // The device draws a font glyph; in the preview we use a Material icon that
    // maps to the same token, so users recognise it while arranging.
    final iconData = _iconFor(e.icon);
    final tp = TextPainter(
      text: TextSpan(
        text: String.fromCharCode(iconData.codePoint),
        style: TextStyle(
          color: _c(e.color),
          fontSize: _fontSize(e.font) + 6,
          fontFamily: iconData.fontFamily,
          package: iconData.fontPackage,
        ),
      ),
      textDirection: TextDirection.ltr,
    )..layout();
    tp.paint(canvas, Offset(e.x, e.y));
  }

  void _paintShape(Canvas canvas, CwElement e) {
    final paint = Paint()..color = _c(e.color);
    if (!e.filled) {
      paint
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2;
    }
    switch (e.shape) {
      case CwShape.circle:
        canvas.drawCircle(Offset(e.x + e.w / 2, e.y + e.h / 2), math.min(e.w, e.h) / 2, paint);
        break;
      case CwShape.line:
        canvas.drawLine(Offset(e.x, e.y + e.h / 2), Offset(e.x + e.w, e.y + e.h / 2),
            paint..strokeWidth = 2);
        break;
      case CwShape.rect:
        canvas.drawRRect(
            RRect.fromRectAndRadius(Rect.fromLTWH(e.x, e.y, e.w, e.h), const Radius.circular(6)),
            paint);
        break;
    }
  }

  void _paintImage(Canvas canvas, CwElement e) {
    // Matches the device placeholder: a framed box (bitmaps deferred).
    final paint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2
      ..color = _c(e.color);
    canvas.drawRRect(
        RRect.fromRectAndRadius(Rect.fromLTWH(e.x, e.y, e.w, e.h), const Radius.circular(6)),
        paint);
    final ic = _iconFor('IMAGE');
    final tp = TextPainter(
      text: TextSpan(
          text: String.fromCharCode(ic.codePoint),
          style: TextStyle(
              color: _c(e.color).withAlpha(120),
              fontSize: math.min(e.w, e.h) * 0.4,
              fontFamily: ic.fontFamily)),
      textDirection: TextDirection.ltr,
    )..layout();
    tp.paint(canvas, Offset(e.x + (e.w - tp.width) / 2, e.y + (e.h - tp.height) / 2));
  }

  void _paintSelection(Canvas canvas, CwElement e) {
    final rect = Rect.fromLTWH(e.x, e.y, e.w, e.h).inflate(3);
    canvas.drawRect(
      rect,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.5
        ..color = const Color(0xFF6FA8C7),
    );
  }

  @override
  bool shouldRepaint(covariant CustomWidgetPainter old) => true;
}

IconData iconFor(String token) => _iconFor(token);

IconData _iconFor(String token) {
  switch (token) {
    case 'GPS':
      return Icons.gps_fixed;
    case 'WIFI':
      return Icons.wifi;
    case 'BLUETOOTH':
      return Icons.bluetooth;
    case 'BATTERY':
      return Icons.battery_full;
    case 'CHARGE':
      return Icons.bolt;
    case 'BELL':
      return Icons.notifications;
    case 'SETTINGS':
      return Icons.settings;
    case 'POWER':
      return Icons.power_settings_new;
    case 'PLAY':
      return Icons.play_arrow;
    case 'WARNING':
      return Icons.warning;
    case 'IMAGE':
      return Icons.image;
    default:
      return Icons.gps_fixed;
  }
}
