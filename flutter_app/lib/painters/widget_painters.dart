import 'dart:math';
import 'package:flutter/material.dart';

const double cx = 120, cy = 120;

class ClockPainter extends CustomPainter {
  final int hours, minutes, seconds;
  final int face; // 0=analog, 1=digital, 2=minimal
  ClockPainter(this.hours, this.minutes, this.seconds, this.face);

  @override
  void paint(Canvas canvas, Size size) {
    if (face == 0) {
      _drawAnalog(canvas);
    } else if (face == 1) { _drawDigital(canvas); }
    else { _drawMinimal(canvas); }
  }

  void _drawAnalog(Canvas canvas) {
    var bg = Paint()..color = Color(0xFF0A0A0A);
    canvas.drawCircle(Offset(cx, cy), 115, bg);
    for (int i = 0; i < 12; i++) {
      double a = (i * 30 - 90) * pi / 180;
      int r1 = i % 3 == 0 ? 100 : 108;
      int r2 = 115;
      canvas.drawLine(
        Offset(cx + cos(a) * r1, cy + sin(a) * r1),
        Offset(cx + cos(a) * r2, cy + sin(a) * r2),
        Paint()..color = Colors.white..strokeWidth = i % 3 == 0 ? 3 : 1,
      );
    }
    double ha = ((hours % 12) * 30 + minutes * 0.5 - 90) * pi / 180;
    double ma = (minutes * 6 - 90) * pi / 180;
    double sa = (seconds * 6 - 90) * pi / 180;
    canvas.drawLine(Offset(cx, cy), Offset(cx + cos(ha) * 50, cy + sin(ha) * 50),
        Paint()..color = Colors.white..strokeWidth = 4..strokeCap = StrokeCap.round);
    canvas.drawLine(Offset(cx, cy), Offset(cx + cos(ma) * 75, cy + sin(ma) * 75),
        Paint()..color = Colors.white..strokeWidth = 2..strokeCap = StrokeCap.round);
    canvas.drawLine(Offset(cx, cy), Offset(cx + cos(sa) * 85, cy + sin(sa) * 85),
        Paint()..color = Colors.red..strokeWidth = 1);
    canvas.drawCircle(Offset(cx, cy), 4, Paint()..color = Colors.white);
  }

  void _drawDigital(Canvas canvas) {
    var bg = Paint()..color = Color(0xFF0A0A0A);
    canvas.drawCircle(Offset(cx, cy), 115, bg);
    String t = '${hours.toString().padLeft(2, '0')}:${minutes.toString().padLeft(2, '0')}';
    _drawSegText(canvas, t, Offset(cx, cy - 10), 3, Colors.cyan);
    String s = seconds.toString().padLeft(2, '0');
    _drawSegText(canvas, s, Offset(cx, cy + 35), 1.5, Colors.grey);
  }

  void _drawMinimal(Canvas canvas) {
    var bg = Paint()..color = Color(0xFF0A0A0A);
    canvas.drawCircle(Offset(cx, cy), 115, bg);
    String t = '${hours.toString().padLeft(2, '0')}:${minutes.toString().padLeft(2, '0')}';
    _drawSegText(canvas, t, Offset(cx, cy), 4, Colors.white.withAlpha(180));
  }

  void _drawSegText(Canvas c, String txt, Offset off, double scale, Color col) {
    int x0 = off.dx.toInt() - (txt.length * 14 * scale / 2).toInt();
    int y0 = off.dy.toInt() - (9 * scale / 2).toInt();
    for (int i = 0; i < txt.length; i++) {
      if (txt[i] == ':') {
        c.drawCircle(Offset(x0 + i * 14 * scale + 3 * scale, y0 + 3 * scale), 2 * scale, Paint()..color = col);
        c.drawCircle(Offset(x0 + i * 14 * scale + 3 * scale, y0 + 9 * scale - 3 * scale), 2 * scale, Paint()..color = col);
      } else {
        _drawSegDigit(c, int.parse(txt[i]), Offset(x0 + i * 14.0 * scale, y0.toDouble()), scale, col);
      }
    }
  }

  void _drawSegDigit(Canvas c, int d, Offset off, double s, Color col) {
    int seg(int n) => const [
      [1,1,1,1,1,1,0],[0,1,1,0,0,0,0],[1,1,0,1,1,0,1],[1,1,1,1,0,0,1],[0,1,1,0,0,1,1],
      [1,0,1,1,0,1,1],[1,0,1,1,1,1,1],[1,1,1,0,0,0,0],[1,1,1,1,1,1,1],[1,1,1,1,0,1,1],
    ][d][n];
    var p = Paint()..color = col..strokeWidth = s..strokeCap = StrokeCap.round;
    double x = off.dx, y = off.dy, w = 5 * s, h = 9 * s;
    if (seg(0) == 1) c.drawLine(Offset(x, y), Offset(x + w, y), p);
    if (seg(1) == 1) c.drawLine(Offset(x + w, y), Offset(x + w, y + h / 2), p);
    if (seg(2) == 1) c.drawLine(Offset(x + w, y + h / 2), Offset(x + w, y + h), p);
    if (seg(3) == 1) c.drawLine(Offset(x, y + h), Offset(x + w, y + h), p);
    if (seg(4) == 1) c.drawLine(Offset(x, y + h / 2), Offset(x, y + h), p);
    if (seg(5) == 1) c.drawLine(Offset(x, y), Offset(x, y + h / 2), p);
    if (seg(6) == 1) c.drawLine(Offset(x, y + h / 2), Offset(x + w, y + h / 2), p);
  }

  @override
  bool shouldRepaint(ClockPainter o) =>
      o.hours != hours || o.minutes != minutes || o.seconds != seconds || o.face != face;
}

class CompassPainter extends CustomPainter {
  final double heading;
  final String cardinal;
  CompassPainter(this.heading, this.cardinal);

  @override
  void paint(Canvas canvas, Size size) {
    _drawBezel(canvas);
    canvas.save(); canvas.rotate(-heading * pi / 180);
    _drawTicks(canvas);
    _drawLetters(canvas);
    canvas.restore();
    _drawAirplane(canvas);
  }

  void _drawBezel(Canvas c) {
    c.drawCircle(Offset(cx, cy), 117, Paint()..color = Color(0xFF2D3234));
    c.drawCircle(Offset(cx, cy), 111, Paint()..color = Color(0xFFAAAAAA)..style = PaintingStyle.stroke..strokeWidth = 10);
    c.drawCircle(Offset(cx, cy), 117, Paint()..color = Colors.white..style = PaintingStyle.stroke..strokeWidth = 1);
  }

  void _drawTicks(Canvas c) {
    for (int i = 0; i < 36; i++) {
      double a = (i * 10 - 90) * pi / 180;
      int r1 = 102, r2 = 110;
      c.drawLine(
        Offset(cx + cos(a) * r1, cy + sin(a) * r1),
        Offset(cx + cos(a) * r2, cy + sin(a) * r2),
        Paint()..color = Colors.white..strokeWidth = 2,
      );
    }
    for (int i = 0; i < 36; i++) {
      double a = (i * 10 + 5 - 90) * pi / 180;
      c.drawCircle(Offset(cx + cos(a) * 98, cy + sin(a) * 98), 1.5, Paint()..color = Colors.white38);
    }
  }

  void _drawLetters(Canvas c) {
    var marks = [
      (0, 'N', Colors.yellow.shade700, 82),
      (90, 'E', Colors.yellow.shade700, 82),
      (180, 'S', Colors.yellow.shade700, 82),
      (270, 'W', Colors.yellow.shade700, 82),
    ];
    for (var m in marks) {
      double a = (m.$1 - 90) * pi / 180;
      var tp = TextPainter(text: TextSpan(text: m.$2, style: TextStyle(color: m.$3, fontSize: 16, fontWeight: FontWeight.bold)), textDirection: TextDirection.ltr)..layout();
      tp.paint(c, Offset(cx + cos(a) * m.$4 - tp.width / 2, cy + sin(a) * m.$4 - tp.height / 2));
    }
    // Degree numbers at 30/60/120/150/210/240/300/330
    var nums = [30, 60, 120, 150, 210, 240, 300, 330];
    var ntxt = ['3', '6', '12', '15', '21', '24', '30', '33'];
    for (int i = 0; i < nums.length; i++) {
      double a = (nums[i] - 90) * pi / 180;
      var tp = TextPainter(text: TextSpan(text: ntxt[i], style: TextStyle(color: Color(0xFFBBBBBB), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
      tp.paint(c, Offset(cx + cos(a) * 78 - tp.width / 2, cy + sin(a) * 78 - tp.height / 2));
    }
  }

  void _drawAirplane(Canvas c) {
    c.drawLine(Offset(cx, cy - 45), Offset(cx, cy + 40),
        Paint()..color = Colors.orange..strokeWidth = 4..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx - 45, cy + 12), Offset(cx + 45, cy + 12),
        Paint()..color = Colors.orange..strokeWidth = 4..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx, cy + 42), Offset(cx, cy + 12),
        Paint()..color = Colors.orange.withAlpha(150)..strokeWidth = 3..strokeCap = StrokeCap.round);
    c.drawCircle(Offset(cx, cy + 12), 4, Paint()..color = Colors.white);
  }

  @override
  bool shouldRepaint(CompassPainter o) => o.heading != heading;
}

class AttitudePainter extends CustomPainter {
  final double pitch, roll;
  AttitudePainter(this.pitch, this.roll);

  @override
  void paint(Canvas canvas, Size size) {
    _drawBezel(canvas);
    canvas.save(); canvas.translate(cx, cy); canvas.rotate(roll * pi / 180);
    _drawHorizon(canvas);
    canvas.restore();
    _drawAircraft(canvas);
    _drawRollScale(canvas);
    _drawLabels(canvas);
  }

  void _drawBezel(Canvas c) {
    c.drawCircle(Offset(cx, cy), 109, Paint()..color = Color(0xFF222244)..style = PaintingStyle.stroke..strokeWidth = 10);
    c.drawCircle(Offset(cx, cy), 114, Paint()..style = PaintingStyle.stroke..strokeWidth = 2);
    c.drawCircle(Offset(cx, cy), 115, Paint()..color = Colors.white..style = PaintingStyle.stroke..strokeWidth = 1);
  }

  void _drawHorizon(Canvas c) {
    double pitchPx = pitch * 3;
    var clip = Path()..addOval(Rect.fromCircle(center: Offset.zero, radius: 103));
    c.clipPath(clip);
    c.drawRect(Rect.fromLTWH(-120, -120 + pitchPx, 240, 240), Paint()..color = Color(0xFF1177CC));
    c.drawRect(Rect.fromLTWH(-120, pitchPx, 240, 240), Paint()..color = Color(0xFF885522));
    c.drawLine(Offset(-120, pitchPx), Offset(120, pitchPx), Paint()..color = Colors.white..strokeWidth = 3);

    // Pitch ladder
    for (int i = -4; i <= 4; i++) {
      if (i == 0) continue;
      double y = i * 32 + pitchPx;
      if (y < -115 || y > 115) continue;
      int val = i.abs() * 5;
      double len = (val == 10 || val == 20) ? 60 : 30;
      c.drawLine(Offset(-len, y), Offset(len, y), Paint()..color = Colors.white..strokeWidth = 2);
    }

    // Virtual runway
    var rwy = Path()
      ..moveTo(0, 10 + pitchPx)
      ..lineTo(-48, 60 + pitchPx)
      ..lineTo(48, 60 + pitchPx)
      ..close();
    c.drawPath(rwy, Paint()..color = Color(0xFF502616)..style = PaintingStyle.fill);
    c.drawLine(Offset(0, 10 + pitchPx), Offset(-16, 60 + pitchPx), Paint()..color = Colors.orange..strokeWidth = 2);
    c.drawLine(Offset(0, 10 + pitchPx), Offset(16, 60 + pitchPx), Paint()..color = Colors.orange..strokeWidth = 2);
    c.drawLine(Offset(-16, 60 + pitchPx), Offset(16, 60 + pitchPx), Paint()..color = Colors.orange..strokeWidth = 2);
  }

  void _drawAircraft(Canvas c) {
    c.drawLine(Offset(cx - 70, cy), Offset(cx - 18, cy),
        Paint()..color = Colors.yellow.shade700..strokeWidth = 5..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx + 18, cy), Offset(cx + 70, cy),
        Paint()..color = Colors.yellow.shade700..strokeWidth = 5..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx - 18, cy), Offset(cx, cy + 14), Paint()..color = Colors.yellow.shade700..strokeWidth = 5..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx, cy + 14), Offset(cx + 18, cy), Paint()..color = Colors.yellow.shade700..strokeWidth = 5..strokeCap = StrokeCap.round);
    c.drawCircle(Offset(cx, cy), 4, Paint()..color = Colors.yellow.shade700);
  }

  void _drawRollScale(Canvas c) {
    for (int a = -50; a <= 50; a += 10) {
      double rad = (a - 90) * pi / 180;
      int r1 = a % 30 == 0 ? 106 : 108;
      c.drawLine(
        Offset(cx + cos(rad) * r1, cy + sin(rad) * r1),
        Offset(cx + cos(rad) * 112, cy + sin(rad) * 112),
        Paint()..color = Colors.white..strokeWidth = 2,
      );
    }
    c.drawLine(Offset(cx - 8, cy - 110), Offset(cx + 8, cy - 110),
        Paint()..color = Colors.orange..strokeWidth = 3..strokeCap = StrokeCap.round);
    double ra = (roll - 90) * pi / 180;
    var ptr = Path()
      ..moveTo(cx + cos(ra) * 106, cy + sin(ra) * 106)
      ..lineTo(cx + cos(ra - 0.07) * 100, cy + sin(ra - 0.07) * 100)
      ..lineTo(cx + cos(ra + 0.07) * 100, cy + sin(ra + 0.07) * 100)
      ..close();
    c.drawPath(ptr, Paint()..color = Colors.yellow);
  }

  void _drawLabels(Canvas c) {
    var tp = TextPainter(text: TextSpan(text: 'P ${pitch.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFF00FFAA), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(c, Offset(5, 215));
    tp = TextPainter(text: TextSpan(text: 'R ${roll.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFF00FFAA), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(c, Offset(240 - tp.width - 5, 215));
  }

  @override
  bool shouldRepaint(AttitudePainter o) => o.pitch != pitch || o.roll != roll;
}

class AltTempPainter extends CustomPainter {
  final double altitude, temperature;
  AltTempPainter(this.altitude, this.temperature);

  @override
  void paint(Canvas canvas, Size size) {
    _drawBezel(canvas);
    _drawTicks(canvas);
    _drawHands(canvas);
    _drawLabels(canvas);
  }

  void _drawBezel(Canvas c) {
    c.drawCircle(Offset(cx, cy), 117, Paint()..color = Color(0xFF191C1E));
    c.drawCircle(Offset(cx, cy), 111, Paint()..color = Color(0xFFAAAAAA)..style = PaintingStyle.stroke..strokeWidth = 10);
    c.drawCircle(Offset(cx, cy), 117, Paint()..color = Colors.white..style = PaintingStyle.stroke..strokeWidth = 1);
    c.drawCircle(Offset(cx, cy), 55, Paint()..color = Color(0xFF24282A));
  }

  void _drawTicks(Canvas c) {
    for (int i = 0; i < 100; i++) {
      double a = (i * 3.6 - 90) * pi / 180;
      bool major = i % 10 == 0;
      int r1 = major ? 96 : 100;
      int r2 = 108;
      c.drawLine(
        Offset(cx + cos(a) * r1, cy + sin(a) * r1),
        Offset(cx + cos(a) * r2, cy + sin(a) * r2),
        Paint()..color = Colors.white..strokeWidth = major ? 2 : 1,
      );
    }
    for (int n = 0; n < 10; n++) {
      double a = (n * 36 - 90) * pi / 180;
      var tp = TextPainter(text: TextSpan(text: '$n', style: TextStyle(color: Colors.white, fontSize: 12)), textDirection: TextDirection.ltr)..layout();
      tp.paint(c, Offset(cx + cos(a) * 72 - tp.width / 2, cy + sin(a) * 72 - tp.height / 2));
    }
  }

  void _drawHands(Canvas c) {
    int alt = altitude.toInt();
    double longAngle = ((alt % 1000) / 1000.0) * 360;
    double shortAngle = ((alt % 10000) / 10000.0) * 360;
    double la = (longAngle - 90) * pi / 180;
    c.drawLine(Offset(cx, cy), Offset(cx + cos(la) * 85, cy + sin(la) * 85),
        Paint()..color = Colors.white..strokeWidth = 2..strokeCap = StrokeCap.round);
    double sa = (shortAngle - 90) * pi / 180;
    c.drawLine(Offset(cx, cy), Offset(cx + cos(sa) * 50, cy + sin(sa) * 50),
        Paint()..color = Colors.white..strokeWidth = 5..strokeCap = StrokeCap.round);
    c.drawCircle(Offset(cx, cy), 6, Paint()..color = Color(0xFF888888));
    c.drawCircle(Offset(cx, cy), 3, Paint()..color = Colors.white);
  }

  void _drawLabels(Canvas c) {
    var tp = TextPainter(text: TextSpan(text: 'ALT', style: TextStyle(color: Color(0xFF888888), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(c, Offset(cx - tp.width / 2, cy - 62));
    tp = TextPainter(text: TextSpan(text: '${altitude.toInt()} ft', style: TextStyle(color: Color(0xFF00FFAA), fontSize: 14)), textDirection: TextDirection.ltr)..layout();
    tp.paint(c, Offset(cx - tp.width / 2, 90));
    tp = TextPainter(text: TextSpan(text: '${temperature.toStringAsFixed(1)} °C', style: TextStyle(color: Color(0xFFFF6644), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(c, Offset(cx - tp.width / 2, 108));
    // x1000 ft reference window
    c.drawRRect(RRect.fromRectAndRadius(Rect.fromCenter(center: Offset(cx, cy + 65), width: 42, height: 16), Radius.circular(2)),
        Paint()..color = Color(0xFF000000));
    c.drawRRect(RRect.fromRectAndRadius(Rect.fromCenter(center: Offset(cx, cy + 65), width: 42, height: 16), Radius.circular(2)),
        Paint()..color = Color(0xFF555555)..style = PaintingStyle.stroke..strokeWidth = 1);
    tp = TextPainter(text: TextSpan(text: 'x1000', style: TextStyle(color: Color(0xFF888888), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(c, Offset(cx - tp.width / 2, cy + 65 - tp.height / 2));
  }

  @override
  bool shouldRepaint(AltTempPainter o) => o.altitude != altitude || o.temperature != temperature;
}

class GForcePainter extends CustomPainter {
  final double ax, ay, az;
  GForcePainter(this.ax, this.ay, this.az);

  @override
  void paint(Canvas canvas, Size size) {
    double gScale = 60;
    double latG = ax, lonG = ay;
    double totalG = sqrt(latG * latG + lonG * lonG);

    canvas.drawCircle(Offset(cx, cy), 110, Paint()..color = Color(0xFF0A0A0A));
    for (int i = 0; i < 4; i++) {
      double r = [0.25, 0.5, 0.75, 1.0][i] * gScale;
      canvas.drawCircle(Offset(cx, cy), r, Paint()..color = Colors.white10..style = PaintingStyle.stroke..strokeWidth = 1);
    }
    canvas.drawLine(Offset(cx - 80, cy), Offset(cx + 80, cy), Paint()..color = Color(0xFF222222));
    canvas.drawLine(Offset(cx, cy - 80), Offset(cx, cy + 80), Paint()..color = Color(0xFF222222));

    double dotX = cx + latG * gScale;
    double dotY = cy - lonG * gScale;
    Color col = totalG < 0.3 ? Colors.green : totalG < 0.6 ? Colors.yellow : totalG < 0.8 ? Colors.orange : Colors.red;
    canvas.drawCircle(Offset(dotX, dotY), 6, Paint()..color = col);
    canvas.drawCircle(Offset(dotX, dotY), 8, Paint()..color = col.withAlpha(80));

    var tp = TextPainter(text: TextSpan(text: '${totalG.toStringAsFixed(2)}g', style: TextStyle(color: Colors.white, fontSize: 14)), textDirection: TextDirection.ltr)..layout();
    tp.paint(canvas, Offset(cx - tp.width / 2, 90));
    tp = TextPainter(text: TextSpan(text: 'G-FORCE', style: TextStyle(color: Color(0xFF44FF44), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(canvas, Offset(cx - tp.width / 2, 20));
  }

  @override
  bool shouldRepaint(GForcePainter o) => o.ax != ax || o.ay != ay;
}

class MusicPainter extends CustomPainter {
  final String title, artist;
  final bool playing;
  MusicPainter(this.title, this.artist, this.playing);

  @override
  void paint(Canvas canvas, Size size) {
    canvas.drawCircle(Offset(cx, cy), 115, Paint()..color = Color(0xFF0A0A0A));
    // Speaker icon
    canvas.drawRect(Rect.fromLTWH(cx - 20, cy - 15, 15, 30), Paint()..color = Colors.white70);
    canvas.drawPath(
      Path()..moveTo(cx - 5, cy - 15)..lineTo(cx + 15, cy - 30)..lineTo(cx + 15, cy + 30)..lineTo(cx - 5, cy + 15)..close(),
      Paint()..color = Colors.white70,
    );
    // Sound waves
    canvas.drawArc(Rect.fromCircle(center: Offset(cx + 20, cy), radius: 15), -0.5, 1, false, Paint()..color = Colors.white38..style = PaintingStyle.stroke..strokeWidth = 2);
    canvas.drawArc(Rect.fromCircle(center: Offset(cx + 20, cy), radius: 25), -0.5, 1, false, Paint()..color = Colors.white24..style = PaintingStyle.stroke..strokeWidth = 2);

    var tp = TextPainter(text: TextSpan(text: title, style: TextStyle(color: Colors.white, fontSize: 14)), textDirection: TextDirection.ltr, maxLines: 1, ellipsis: '...')..layout(maxWidth: 200);
    tp.paint(canvas, Offset(cx - tp.width / 2, cy + 40));
    tp = TextPainter(text: TextSpan(text: artist, style: TextStyle(color: Colors.grey, fontSize: 10)), textDirection: TextDirection.ltr, maxLines: 1, ellipsis: '...')..layout(maxWidth: 200);
    tp.paint(canvas, Offset(cx - tp.width / 2, cy + 58));
  }

  @override
  bool shouldRepaint(MusicPainter o) => o.playing != playing || o.title != title;
}

class AirplanePainter extends CustomPainter {
  final double pitch, roll, heading;
  AirplanePainter(this.pitch, this.roll, this.heading);

  @override
  void paint(Canvas canvas, Size size) {
    canvas.drawCircle(Offset(cx, cy), 115, Paint()..color = Color(0xFF0A121C));
    canvas.drawCircle(Offset(cx, cy), 109, Paint()..color = Color(0xFFAAAAAA)..style = PaintingStyle.stroke..strokeWidth = 10);
    canvas.drawCircle(Offset(cx, cy), 115, Paint()..color = Colors.white..style = PaintingStyle.stroke..strokeWidth = 1);
    // Grid
    canvas.drawCircle(Offset(cx, cy), 105, Paint()..color = Color(0xFF505050)..style = PaintingStyle.stroke..strokeWidth = 1);
    canvas.drawCircle(Offset(cx, cy), 70, Paint()..color = Color(0xFF323232)..style = PaintingStyle.stroke..strokeWidth = 1);
    canvas.drawLine(Offset(cx - 105, cy), Offset(cx + 105, cy), Paint()..color = Color(0xFF3C3C3C));
    canvas.drawLine(Offset(cx, cy - 105), Offset(cx, cy + 105), Paint()..color = Color(0xFF3C3C3C));

    int pitchMove = pitch.toInt() * 2;
    canvas.save(); canvas.translate(0, pitchMove.toDouble()); canvas.rotate(roll * pi / 180);
    _drawPlane(canvas);
    canvas.restore();

    var tp = TextPainter(text: TextSpan(text: 'P ${pitch.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFF00FFAA), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(canvas, Offset(5, 215));
    tp = TextPainter(text: TextSpan(text: 'R ${roll.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFF00FFAA), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(canvas, Offset(240 - tp.width - 5, 215));
    tp = TextPainter(text: TextSpan(text: 'HDG ${heading.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFF88CCFF), fontSize: 10)), textDirection: TextDirection.ltr)..layout();
    tp.paint(canvas, Offset(cx - tp.width / 2, 8));
  }

  void _drawPlane(Canvas c) {
    c.drawLine(Offset(cx, cy - 25), Offset(cx, cy + 30),
        Paint()..color = Color(0xFFFFB428)..strokeWidth = 5..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx - 35, cy + 10), Offset(cx + 35, cy + 10),
        Paint()..color = Color(0xFF28DCFF)..strokeWidth = 4..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx - 35, cy + 25), Offset(cx - 45, cy + 25),
        Paint()..color = Color(0xFF28DCFF).withAlpha(200)..strokeWidth = 3..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx + 35, cy + 25), Offset(cx + 45, cy + 25),
        Paint()..color = Color(0xFF28DCFF).withAlpha(200)..strokeWidth = 3..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx - 14, cy + 40), Offset(cx + 14, cy + 40),
        Paint()..color = Color(0xFFFF5050)..strokeWidth = 3..strokeCap = StrokeCap.round);
    c.drawLine(Offset(cx, cy - 25), Offset(cx - 12, cy), Paint()..color = Color(0xFFFFB428)..strokeWidth = 3);
    c.drawLine(Offset(cx, cy - 25), Offset(cx + 12, cy), Paint()..color = Color(0xFFFFB428)..strokeWidth = 3);
    c.drawCircle(Offset(cx, cy + 2), 4, Paint()..color = Colors.white);
  }

  @override
  bool shouldRepaint(AirplanePainter o) => o.pitch != pitch || o.roll != roll || o.heading != heading;
}
