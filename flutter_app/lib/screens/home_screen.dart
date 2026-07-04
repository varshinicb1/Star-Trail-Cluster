import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../widgets/logo_widgets.dart';
import '../widgets/glass_container.dart';
import '../theme/app_theme.dart';
import 'controls/led_screen.dart';
import 'controls/widget_screen.dart';
import 'controls/system_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});
  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> with TickerProviderStateMixin {
  late AnimationController _pulseController;

  @override
  void initState() {
    super.initState();
    _pulseController = AnimationController(vsync: this, duration: Duration(seconds: 2))..repeat(reverse: true);
  }

  @override
  void dispose() {
    _pulseController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<ThemeProvider>(
      builder: (_, tp, _) => Consumer<DeviceService>(
        builder: (_, svc, _) {
          final theme = tp.theme;
          if (!svc.isConnected) return _buildDisconnected(theme, svc);
          return _buildDashboard(theme, svc);
        },
      ),
    );
  }

  Widget _buildDisconnected(AppTheme theme, DeviceService svc) {
    return Scaffold(
      backgroundColor: theme.surface,
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            AnimatedBuilder(
              animation: _pulseController,
              builder: (_, child) => Transform.scale(
                scale: 0.88 + _pulseController.value * 0.12,
                child: child,
              ),
              child: Container(
                width: 100,
                height: 100,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  boxShadow: [
                    BoxShadow(
                      color: theme.primary.withAlpha(15),
                      blurRadius: 40,
                      spreadRadius: 4,
                    ),
                  ],
                ),
                child: ThemeLogo(size: 100),
              ),
            ),
            SizedBox(height: 32),
            GlassContainer(
              padding: EdgeInsets.symmetric(horizontal: 32, vertical: 24),
              borderRadius: BorderRadius.circular(20),
              child: Column(
                children: [
                  Text(
                    'DISCONNECTED',
                    style: TextStyle(
                      color: theme.textMuted,
                      fontSize: 11,
                      letterSpacing: 4,
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                  SizedBox(height: 8),
                  Text(
                    'Connect to your Star Trail cluster',
                    style: TextStyle(color: theme.textSecondary, fontSize: 13, letterSpacing: 0.3),
                  ),
                  SizedBox(height: 20),
                  GlassButton(
                    width: double.infinity,
                    onTap: () => Navigator.pushNamed(context, '/config'),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(Icons.link, size: 16, color: theme.primary),
                        SizedBox(width: 8),
                        Text(
                          'CONNECT',
                          style: TextStyle(color: theme.primary, fontSize: 13, fontWeight: FontWeight.w600, letterSpacing: 2),
                        ),
                      ],
                    ),
                  ),
                  SizedBox(height: 10),
                  GestureDetector(
                    onTap: () => svc.startSimulator(),
                    child: Text(
                      'Use Simulator',
                      style: TextStyle(color: theme.warning, fontSize: 12, letterSpacing: 1),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDashboard(AppTheme theme, DeviceService svc) {
    final d = svc.data;
    return Scaffold(
      backgroundColor: theme.surface,
      appBar: AppBar(
        leading: Container(
          margin: EdgeInsets.only(left: 12),
          child: ThemeLogo(size: 28),
        ),
        title: Text(
          theme.name.toUpperCase(),
          style: TextStyle(
            fontSize: 13,
            letterSpacing: 3,
            fontWeight: FontWeight.w300,
            color: theme.textMuted,
          ),
        ),
        actions: [
          _connectionBadge(theme, svc),
          SizedBox(width: 8),
          IconButton(
            icon: Icon(Icons.tune, size: 20),
            onPressed: () => Navigator.pushNamed(context, '/config'),
            tooltip: 'Settings',
          ),
          SizedBox(width: 4),
        ],
      ),
      body: SingleChildScrollView(
        padding: EdgeInsets.fromLTRB(16, 8, 16, 24),
        child: Column(
          children: [
            _connectionPill(theme, svc),
            SizedBox(height: 16),
            _compassCard(theme, d.heading),
            SizedBox(height: 14),
            _sensorRow(theme, d),
            SizedBox(height: 14),
            _gforcePanel(theme, d),
            SizedBox(height: 14),
            _quickControls(theme),
            SizedBox(height: 12),
            _sensorTrigger(theme, d, svc),
          ],
        ),
      ),
    );
  }

  Widget _connectionBadge(AppTheme theme, DeviceService svc) {
    final isSim = svc.mode == ConnectionMode.simulator;
    Color badgeColor;
    String label;
    if (svc.mode == ConnectionMode.ble) {
      badgeColor = theme.primary;
      label = 'BLE';
    } else if (svc.mode == ConnectionMode.wifi) {
      badgeColor = theme.success;
      label = 'WiFi';
    } else {
      badgeColor = theme.warning;
      label = 'SIM';
    }
    return Container(
      padding: EdgeInsets.symmetric(horizontal: 10, vertical: 4),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [badgeColor.withAlpha(15), badgeColor.withAlpha(5)],
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
        ),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: badgeColor.withAlpha(40), width: 0.5),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          GlowingDot(color: badgeColor, size: 6, animate: !isSim),
          SizedBox(width: 6),
          Text(
            label,
            style: TextStyle(
              color: badgeColor,
              fontSize: 10,
              fontWeight: FontWeight.w600,
              letterSpacing: 1,
            ),
          ),
        ],
      ),
    );
  }

  Widget _connectionPill(AppTheme theme, DeviceService svc) {
    final sim = svc.mode == ConnectionMode.simulator;
    final color = sim ? theme.warning : theme.success;
    final text = sim ? 'Simulator Mode' : 'Connected';
    final sub = sim ? 'Local simulation' : '${svc.data.ssid} • ${svc.data.ip}';
    return GlassContainer(
      padding: EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      borderColor: color.withAlpha(30),
      child: Row(
        children: [
          GlowingDot(color: color, animate: !sim),
          SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  text,
                  style: TextStyle(color: color, fontSize: 12, fontWeight: FontWeight.w600, letterSpacing: 0.5),
                ),
                SizedBox(height: 2),
                Text(
                  sub,
                  style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 0.3),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  // Mirrors the device's actual linear-scrolling compass widget
  // (BenzCluster/compass.cpp) exactly: pure-black background, a horizontal
  // white scale that scrolls under a FIXED RED centre needle, with the
  // heading value in white text below. This is deliberately NOT a rotating
  // dial — the app must show the same design the gadget shows, not a
  // decorative reinterpretation.
  Widget _compassCard(AppTheme theme, double heading) {
    return GlassContainer(
      padding: EdgeInsets.zero,
      borderRadius: BorderRadius.circular(20),
      bgColor: Colors.black,
      child: ClipRRect(
        borderRadius: BorderRadius.circular(20),
        child: SizedBox(
          height: 200,
          width: double.infinity,
          child: _LinearCompass(heading: heading),
        ),
      ),
    );
  }

  Widget _sensorRow(AppTheme theme, dynamic d) {
    return Row(
      children: [
        Expanded(child: _sensorCard(theme, 'ALT', '${d.altitude.toInt()}', 'ft', theme.secondary, Icons.height)),
        SizedBox(width: 8),
        Expanded(child: _sensorCard(theme, 'TEMP', '${d.temperature.toStringAsFixed(1)}', '°C', theme.warning, Icons.thermostat)),
        SizedBox(width: 8),
        Expanded(child: _sensorCard(theme, 'PRESS', '${d.pressure.toStringAsFixed(0)}', 'hPa', theme.primary, Icons.air)),
      ],
    );
  }

  Widget _sensorCard(AppTheme theme, String label, String value, String unit, Color accent, IconData icon) {
    return GlassContainer(
      padding: EdgeInsets.all(14),
      borderColor: accent.withAlpha(15),
      child: Column(
        children: [
          Container(
            width: 34,
            height: 34,
            decoration: BoxDecoration(
              color: accent.withAlpha(12),
              borderRadius: BorderRadius.circular(10),
            ),
            child: Icon(icon, color: accent, size: 16),
          ),
          SizedBox(height: 10),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.end,
            children: [
              Text(
                value,
                style: TextStyle(color: accent, fontSize: 16, fontWeight: FontWeight.bold, fontFamily: 'monospace', letterSpacing: 0.5),
              ),
              SizedBox(width: 2),
              Padding(
                padding: EdgeInsets.only(bottom: 2),
                child: Text(unit, style: TextStyle(color: accent.withAlpha(150), fontSize: 10, fontFamily: 'monospace')),
              ),
            ],
          ),
          SizedBox(height: 4),
          Text(
            label,
            style: TextStyle(color: theme.textMuted, fontSize: 9, letterSpacing: 1.5, fontWeight: FontWeight.w600),
          ),
        ],
      ),
    );
  }

  Widget _gforcePanel(AppTheme theme, dynamic d) {
    final totalG = math.sqrt(d.accelX * d.accelX + d.accelY * d.accelY + d.accelZ * d.accelZ);
    final gColor = totalG < 0.3 ? theme.success : totalG < 0.6 ? const Color(0xFFFFCC00) : totalG < 0.8 ? theme.warning : theme.error;
    return GlassContainer(
      padding: EdgeInsets.all(16),
      borderColor: gColor.withAlpha(20),
      child: Row(
        children: [
          SizedBox(
            width: 80,
            height: 80,
            child: CustomPaint(
              painter: _GForceDialPainter(totalG, gColor, theme),
              child: Center(
                child: Text(
                  totalG.toStringAsFixed(2),
                  style: TextStyle(color: gColor, fontSize: 16, fontWeight: FontWeight.bold, fontFamily: 'monospace'),
                ),
              ),
            ),
          ),
          SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'G-FORCE',
                  style: TextStyle(color: theme.textMuted, fontSize: 9, letterSpacing: 1.5, fontWeight: FontWeight.w600),
                ),
                SizedBox(height: 8),
                _gAxis(theme, 'X', d.accelX, theme.primary),
                SizedBox(height: 4),
                _gAxis(theme, 'Y', d.accelY, theme.secondary),
                SizedBox(height: 4),
                _gAxis(theme, 'Z', d.accelZ, theme.accent),
                SizedBox(height: 6),
                Text(
                  'Roll ${d.roll.toStringAsFixed(1)}°  Pitch ${d.pitch.toStringAsFixed(1)}°',
                  style: TextStyle(color: theme.textMuted, fontSize: 10, fontFamily: 'monospace', letterSpacing: 0.3),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _gAxis(AppTheme theme, String axis, double value, Color color) {
    final clamped = (value.abs() / 2.0).clamp(0.0, 1.0);
    return Row(
      children: [
        SizedBox(
          width: 14,
          child: Text(axis, style: TextStyle(color: color, fontSize: 10, fontWeight: FontWeight.bold, fontFamily: 'monospace')),
        ),
        SizedBox(width: 8),
        Expanded(
          child: ClipRRect(
            borderRadius: BorderRadius.circular(2),
            child: LinearProgressIndicator(
              value: clamped,
              backgroundColor: theme.cardBorder,
              valueColor: AlwaysStoppedAnimation<Color>(color),
              minHeight: 4,
            ),
          ),
        ),
        SizedBox(width: 6),
        SizedBox(
          width: 42,
          child: Text(
            value.toStringAsFixed(2),
            style: TextStyle(color: theme.textSecondary, fontSize: 10, fontFamily: 'monospace'),
          ),
        ),
      ],
    );
  }

  Widget _quickControls(AppTheme theme) {
    return Row(
      children: [
        Expanded(child: _quickBtn(theme, Icons.lightbulb, 'LED', () => Navigator.push(context, MaterialPageRoute(builder: (_) => const LEDScreen())))),
        SizedBox(width: 8),
        Expanded(child: _quickBtn(theme, Icons.dashboard, 'Widgets', () => Navigator.push(context, MaterialPageRoute(builder: (_) => const WidgetScreen())))),
        SizedBox(width: 8),
        Expanded(child: _quickBtn(theme, Icons.settings, 'System', () => Navigator.push(context, MaterialPageRoute(builder: (_) => const SystemScreen())))),
      ],
    );
  }

  Widget _quickBtn(AppTheme theme, IconData icon, String label, VoidCallback onTap) {
    return GlassContainer(
      padding: EdgeInsets.symmetric(vertical: 14),
      borderColor: theme.cardBorder,
      onTap: onTap,
      child: Column(
        children: [
          Icon(icon, color: theme.textSecondary, size: 22),
          SizedBox(height: 6),
          Text(label, style: TextStyle(color: theme.textMuted, fontSize: 10, fontWeight: FontWeight.w500, letterSpacing: 0.3)),
        ],
      ),
    );
  }

  Widget _sensorTrigger(AppTheme theme, dynamic d, DeviceService svc) {
    return GlassContainer(
      padding: EdgeInsets.all(14),
      borderColor: theme.cardBorder,
      onTap: () => _showSensorSheet(theme, svc),
      child: Row(
        children: [
          Container(
            width: 34,
            height: 34,
            decoration: BoxDecoration(color: theme.textMuted.withAlpha(12), borderRadius: BorderRadius.circular(10)),
            child: Icon(Icons.sensors, color: theme.textMuted, size: 16),
          ),
          SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('SENSORS', style: TextStyle(color: theme.textMuted, fontSize: 9, letterSpacing: 1.5, fontWeight: FontWeight.w600)),
                SizedBox(height: 4),
                Text(
                  'Mag ${d.magRawX}, ${d.magRawY}, ${d.magRawZ}  •  Up ${(d.uptime / 3600).floor()}h ${((d.uptime % 3600) / 60).floor()}m',
                  style: TextStyle(color: theme.textSecondary, fontSize: 10, fontFamily: 'monospace', letterSpacing: 0.2),
                ),
              ],
            ),
          ),
          Icon(Icons.chevron_right, color: theme.textMuted, size: 20),
        ],
      ),
    );
  }

  void _showSensorSheet(AppTheme theme, DeviceService svc) {
    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (_) => Consumer<DeviceService>(
        builder: (_, s, _) {
          final data = s.data;
          return GlassContainer(
            margin: EdgeInsets.all(8),
            padding: EdgeInsets.fromLTRB(20, 12, 20, 24),
            borderRadius: BorderRadius.circular(24),
            borderColor: theme.cardBorder,
            bgColor: theme.card,
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Container(width: 36, height: 3, decoration: BoxDecoration(color: theme.textMuted.withAlpha(60), borderRadius: BorderRadius.circular(2))),
                SizedBox(height: 16),
                Text('Live Sensor Data', style: TextStyle(color: theme.textPrimary, fontSize: 16, fontWeight: FontWeight.w600, letterSpacing: 0.5)),
                SizedBox(height: 20),
                _sensorRowSheet(theme, 'Heading', '${data.heading.toStringAsFixed(1)}°', theme.accent),
                _sensorRowSheet(theme, 'Pitch / Roll', '${data.pitch.toStringAsFixed(1)}° / ${data.roll.toStringAsFixed(1)}°', theme.primary),
                _sensorRowSheet(theme, 'Altitude', '${data.altitude.toInt()} ft', theme.secondary),
                _sensorRowSheet(theme, 'Temperature', '${data.temperature.toStringAsFixed(1)} °C', theme.warning),
                _sensorRowSheet(theme, 'Pressure', '${data.pressure.toStringAsFixed(1)} hPa', theme.primary),
                _sensorRowSheet(theme, 'Magnetometer', '${data.magRawX}, ${data.magRawY}, ${data.magRawZ}', const Color(0xFFFFCC00)),
                _sensorRowSheet(theme, 'Accelerometer', '${data.accelX.toStringAsFixed(2)}, ${data.accelY.toStringAsFixed(2)}, ${data.accelZ.toStringAsFixed(2)}', theme.primary),
                _sensorRowSheet(theme, 'RSSI', '${data.rssi} dBm', data.rssi > -70 ? theme.success : theme.warning),
                _sensorRowSheet(theme, 'Uptime', '${(data.uptime / 3600).floor()}h ${((data.uptime % 3600) / 60).floor()}m ${(data.uptime % 60).floor()}s', theme.textMuted),
                _sensorRowSheet(theme, 'Free Heap', '${(data.heap / 1024).floor()} KB', theme.textMuted),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _sensorRowSheet(AppTheme theme, String label, String value, Color valueColor) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 5),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: TextStyle(color: theme.textMuted, fontSize: 12, letterSpacing: 0.3)),
          Text(value, style: TextStyle(color: valueColor, fontSize: 12, fontFamily: 'monospace', fontWeight: FontWeight.w500, letterSpacing: 0.3)),
        ],
      ),
    );
  }

}

/// Faithful app-side reproduction of the device's linear scrolling compass
/// (BenzCluster/compass.cpp): pure black bg, a horizontal WHITE scale with
/// minor ticks every 15deg and cardinal/intercardinal labels every 45deg
/// scrolling under a FIXED RED centre needle, plus a large white heading
/// readout below. Deliberately black/white/red only — no theme colours — so
/// this always looks exactly like what's on the physical gadget.
class _LinearCompass extends StatefulWidget {
  final double heading;
  const _LinearCompass({required this.heading});

  @override
  State<_LinearCompass> createState() => _LinearCompassState();
}

class _LinearCompassState extends State<_LinearCompass> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _animation;
  double _currentHeading = 0;

  @override
  void initState() {
    super.initState();
    _currentHeading = widget.heading;
    _controller = AnimationController(vsync: this, duration: const Duration(milliseconds: 350));
    _animation = _controller.drive(Tween(begin: 0, end: 0));
    _controller.addListener(() => setState(() {}));
  }

  @override
  void didUpdateWidget(_LinearCompass old) {
    super.didUpdateWidget(old);
    if ((old.heading - widget.heading).abs() > 0.3) {
      final diff = widget.heading - _currentHeading;
      final wrapped = diff > 180 ? diff - 360 : (diff < -180 ? diff + 360 : diff);
      final target = _currentHeading + wrapped;
      _animation = _controller.drive(Tween(begin: _currentHeading, end: target));
      _currentHeading = target;
      _controller.reset();
      _controller.forward();
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final heading = _controller.isAnimating ? _animation.value : widget.heading;
    var h = heading % 360;
    if (h < 0) h += 360;
    return CustomPaint(
      size: Size.infinite,
      painter: _LinearCompassPainter(h),
    );
  }
}

class _LinearCompassPainter extends CustomPainter {
  static const _labels = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
  final double heading;
  _LinearCompassPainter(this.heading);

  double _angDiff(double a, double b) {
    var d = a - b;
    while (d > 180) d -= 360;
    while (d <= -180) d += 360;
    return d;
  }

  @override
  void paint(Canvas canvas, Size size) {
    canvas.drawRect(Offset.zero & size, Paint()..color = Colors.black);
    final cx = size.width / 2;
    final baseY = size.height * 0.32;
    const pxPerDeg = 3.6; // wider on-screen than the 240px gadget, same ratio
    final halfVis = size.width * 0.55;

    // Minor ticks every 15deg, major (labelled) every 45deg.
    for (int i = 0; i < 24; i++) {
      final tickHdg = i * 15.0;
      final d = _angDiff(tickHdg, heading);
      final x = cx + d * pxPerDeg;
      if ((x - cx).abs() > halfVis) continue;
      final major = i % 3 == 0;
      final h = major ? 26.0 : 14.0;
      canvas.drawLine(
        Offset(x, baseY),
        Offset(x, baseY + h),
        Paint()
          ..color = Colors.white
          ..strokeWidth = major ? 2.5 : 1.5,
      );
    }
    for (int i = 0; i < 8; i++) {
      final lblHdg = i * 45.0;
      final d = _angDiff(lblHdg, heading);
      final x = cx + d * pxPerDeg;
      if ((x - cx).abs() > halfVis) continue;
      final tp = TextPainter(
        text: TextSpan(text: _labels[i], style: const TextStyle(color: Colors.white, fontSize: 15, fontWeight: FontWeight.w600)),
        textDirection: TextDirection.ltr,
      )..layout();
      tp.paint(canvas, Offset(x - tp.width / 2, baseY + 32));
    }

    // Fixed red centre needle.
    canvas.drawLine(
      Offset(cx, baseY - 10),
      Offset(cx, baseY + 46),
      Paint()
        ..color = const Color(0xFFE01020)
        ..strokeWidth = 3
        ..strokeCap = StrokeCap.round,
    );

    // Heading value + caption.
    final valTp = TextPainter(
      text: TextSpan(text: '${heading.round() % 360}', style: const TextStyle(color: Colors.white, fontSize: 44, fontWeight: FontWeight.w700)),
      textDirection: TextDirection.ltr,
    )..layout();
    valTp.paint(canvas, Offset(cx - valTp.width / 2, baseY + 68));

    final capTp = TextPainter(
      text: const TextSpan(text: 'HEADING', style: TextStyle(color: Color(0xFF777777), fontSize: 12, letterSpacing: 1)),
      textDirection: TextDirection.ltr,
    )..layout();
    capTp.paint(canvas, Offset(cx - capTp.width / 2, baseY + 68 + valTp.height + 4));
  }

  @override
  bool shouldRepaint(_LinearCompassPainter old) => old.heading != heading;
}

class _GForceDialPainter extends CustomPainter {
  final double gValue;
  final Color color;
  final AppTheme theme;
  _GForceDialPainter(this.gValue, this.color, this.theme);

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = math.min(size.width, size.height) / 2 - 4;
    const startAngle = -math.pi * 0.75;
    const sweepAngle = math.pi * 1.5;
    final fraction = (gValue / 2.0).clamp(0.0, 1.0);

    final bgArc = Paint()..color = theme.cardBorder..style = PaintingStyle.stroke..strokeWidth = 6..strokeCap = StrokeCap.round;
    canvas.drawArc(Rect.fromCircle(center: center, radius: radius), startAngle, sweepAngle, false, bgArc);

    final fillArc = Paint()..color = color..style = PaintingStyle.stroke..strokeWidth = 6..strokeCap = StrokeCap.round;
    canvas.drawArc(Rect.fromCircle(center: center, radius: radius), startAngle, sweepAngle * fraction, false, fillArc);
  }

  @override
  bool shouldRepaint(_GForceDialPainter o) => o.gValue != gValue || o.color != color || o.theme != theme;
}
