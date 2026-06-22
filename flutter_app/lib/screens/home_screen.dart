import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../widgets/logo_widgets.dart';
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
    _pulseController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 2),
    )..repeat(reverse: true);
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
                scale: 0.85 + _pulseController.value * 0.15,
                child: child,
              ),
              child: ThemeLogo(size: 100),
            ),
            const SizedBox(height: 28),
            Text(
              'Not Connected',
              style: TextStyle(
                color: theme.textPrimary,
                fontSize: 22,
                fontWeight: FontWeight.w500,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Connect to your Star Trail cluster',
              style: TextStyle(color: theme.textMuted, fontSize: 14),
            ),
            const SizedBox(height: 36),
            SizedBox(
              width: 180,
              child: ElevatedButton(
                onPressed: () => Navigator.pushNamed(context, '/config'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: theme.primary,
                  foregroundColor: theme.surface,
                  padding: const EdgeInsets.symmetric(vertical: 14),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(12),
                  ),
                  elevation: 0,
                ),
                child: const Text(
                  'Connect',
                  style: TextStyle(fontSize: 15, fontWeight: FontWeight.w600),
                ),
              ),
            ),
            const SizedBox(height: 16),
            TextButton(
              onPressed: () => svc.startSimulator(),
              child: Text(
                'Use Simulator',
                style: TextStyle(color: theme.warning, fontSize: 13),
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
        leading: const ThemeLogo(size: 28),
        title: Text(theme.name),
        actions: [
          _connectBadge(theme, svc),
          const SizedBox(width: 4),
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: () => Navigator.pushNamed(context, '/config'),
          ),
          const SizedBox(width: 4),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            _connectionPill(theme, svc),
            const SizedBox(height: 16),
            _compassCard(theme, d.heading),
            const SizedBox(height: 20),
            _dataCards(theme, d),
            const SizedBox(height: 16),
            _gforcePanel(theme, d),
            const SizedBox(height: 16),
            _quickControls(theme),
            const SizedBox(height: 16),
            _sensorTrigger(theme, d, svc),
            const SizedBox(height: 16),
          ],
        ),
      ),
    );
  }

  Widget _connectBadge(AppTheme theme, DeviceService svc) {
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
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
      decoration: BoxDecoration(
        color: badgeColor.withAlpha(25),
        borderRadius: BorderRadius.circular(6),
        border: Border.all(color: badgeColor.withAlpha(80), width: 1),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 6,
            height: 6,
            decoration: BoxDecoration(
              color: badgeColor,
              shape: BoxShape.circle,
              boxShadow: isSim
                  ? []
                  : [BoxShadow(color: badgeColor.withAlpha(120), blurRadius: 4)],
            ),
          ),
          const SizedBox(width: 5),
          Text(
            label,
            style: TextStyle(
              color: badgeColor,
              fontSize: 10,
              fontWeight: FontWeight.w600,
              letterSpacing: 0.5,
            ),
          ),
        ],
      ),
    );
  }

  Widget _connectionPill(AppTheme theme, DeviceService svc) {
    final connected = svc.mode == ConnectionMode.ble || svc.mode == ConnectionMode.wifi;
    final sim = svc.mode == ConnectionMode.simulator;
    final color = sim ? theme.warning : (connected ? theme.success : theme.error);
    final text = sim ? 'Simulator Mode' : 'Connected';
    final sub = sim ? 'Using local simulator' : svc.data.ssid;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(
        color: color.withAlpha(12),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color.withAlpha(40), width: 1),
      ),
      child: Row(
        children: [
          Container(
            width: 8,
            height: 8,
            decoration: BoxDecoration(
              color: color,
              shape: BoxShape.circle,
              boxShadow: [BoxShadow(color: color.withAlpha(100), blurRadius: 6)],
            ),
          ),
          const SizedBox(width: 10),
          Text(
            text,
            style: TextStyle(
              color: color,
              fontSize: 13,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(width: 6),
          Text(
            sub,
            style: TextStyle(color: theme.textMuted, fontSize: 11),
          ),
        ],
      ),
    );
  }

  Widget _compassCard(AppTheme theme, double heading) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: theme.card,
        borderRadius: BorderRadius.circular(24),
        border: Border.all(color: theme.cardBorder),
      ),
      child: Column(
        children: [
          _CompassWidget(heading: heading, theme: theme),
          const SizedBox(height: 14),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.navigation, color: theme.accent, size: 20),
              const SizedBox(width: 8),
              Text(
                '${_cardinal(heading)} • ${heading.toStringAsFixed(0)}°',
                style: TextStyle(
                  color: theme.accent,
                  fontSize: 20,
                  fontWeight: FontWeight.w500,
                  fontFamily: 'monospace',
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _dataCards(AppTheme theme, dynamic d) {
    return Row(
      children: [
        Expanded(child: _dataCard(
          theme,
          Icons.height,
          '${d.altitude.toInt()} ft',
          'ALTITUDE',
          theme.secondary,
        )),
        const SizedBox(width: 8),
        Expanded(child: _dataCard(
          theme,
          Icons.thermostat,
          '${d.temperature.toStringAsFixed(1)}°',
          'TEMP',
          theme.warning,
        )),
        const SizedBox(width: 8),
        Expanded(child: _dataCard(
          theme,
          Icons.air,
          '${d.pressure.toStringAsFixed(0)} hPa',
          'PRESSURE',
          theme.primary,
        )),
      ],
    );
  }

  Widget _dataCard(AppTheme theme, IconData icon, String value, String label, Color accent) {
    return Container(
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [theme.card, theme.card.withAlpha(180)],
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
        ),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: theme.cardBorder),
      ),
      child: Column(
        children: [
          Container(
            width: 36,
            height: 36,
            decoration: BoxDecoration(
              color: accent.withAlpha(20),
              borderRadius: BorderRadius.circular(10),
            ),
            child: Icon(icon, color: accent, size: 18),
          ),
          const SizedBox(height: 8),
          Text(
            value,
            style: TextStyle(
              color: accent,
              fontSize: 14,
              fontWeight: FontWeight.bold,
              fontFamily: 'monospace',
            ),
          ),
          const SizedBox(height: 3),
          Text(
            label,
            style: TextStyle(
              color: theme.textMuted,
              fontSize: 9,
              letterSpacing: 1.2,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  Widget _gforcePanel(AppTheme theme, dynamic d) {
    final totalG = math.sqrt(
      d.accelX * d.accelX + d.accelY * d.accelY + d.accelZ * d.accelZ,
    );
    final gColor = totalG < 0.3
        ? theme.success
        : totalG < 0.6
            ? const Color(0xFFFFCC00)
            : totalG < 0.8
                ? theme.warning
                : theme.error;

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [theme.card, theme.card.withAlpha(180)],
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
        ),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: theme.cardBorder),
      ),
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
                  style: TextStyle(
                    color: gColor,
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                    fontFamily: 'monospace',
                  ),
                ),
              ),
            ),
          ),
          const SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'G-FORCE',
                  style: TextStyle(
                    color: theme.textMuted,
                    fontSize: 9,
                    letterSpacing: 1.2,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const SizedBox(height: 6),
                _gAxisRow(theme, 'X', d.accelX, theme.primary),
                const SizedBox(height: 2),
                _gAxisRow(theme, 'Y', d.accelY, theme.secondary),
                const SizedBox(height: 2),
                _gAxisRow(theme, 'Z', d.accelZ, theme.accent),
                const SizedBox(height: 6),
                Text(
                  'Roll ${d.roll.toStringAsFixed(1)}°  Pitch ${d.pitch.toStringAsFixed(1)}°',
                  style: TextStyle(
                    color: theme.textMuted,
                    fontSize: 11,
                    fontFamily: 'monospace',
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _gAxisRow(AppTheme theme, String axis, double value, Color color) {
    final clamped = (value.abs() / 2.0).clamp(0.0, 1.0);
    return Row(
      children: [
        SizedBox(
          width: 14,
          child: Text(
            axis,
            style: TextStyle(
              color: color,
              fontSize: 10,
              fontWeight: FontWeight.bold,
              fontFamily: 'monospace',
            ),
          ),
        ),
        const SizedBox(width: 6),
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
        const SizedBox(width: 6),
        SizedBox(
          width: 40,
          child: Text(
            value.toStringAsFixed(2),
            style: TextStyle(
              color: theme.textSecondary,
              fontSize: 10,
              fontFamily: 'monospace',
            ),
          ),
        ),
      ],
    );
  }

  Widget _quickControls(AppTheme theme) {
    return Row(
      children: [
            Expanded(child: _quickBtn(theme, Icons.lightbulb, 'LED', () {
          Navigator.push(context, MaterialPageRoute(builder: (_) => const LEDScreen()));
        })),
        const SizedBox(width: 8),
        Expanded(child: _quickBtn(theme, Icons.dashboard, 'Widgets', () {
          Navigator.push(context, MaterialPageRoute(builder: (_) => const WidgetScreen()));
        })),
        const SizedBox(width: 8),
        Expanded(child: _quickBtn(theme, Icons.settings, 'System', () {
          Navigator.push(context, MaterialPageRoute(builder: (_) => const SystemScreen()));
        })),
      ],
    );
  }

  Widget _quickBtn(AppTheme theme, IconData icon, String label, VoidCallback onTap) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 12),
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: [theme.card, theme.card.withAlpha(180)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
          borderRadius: BorderRadius.circular(14),
          border: Border.all(color: theme.cardBorder),
        ),
        child: Column(
          children: [
            Icon(icon, color: theme.textSecondary, size: 22),
            const SizedBox(height: 4),
            Text(
              label,
              style: TextStyle(
                color: theme.textMuted,
                fontSize: 10,
                fontWeight: FontWeight.w500,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _sensorTrigger(AppTheme theme, dynamic d, DeviceService svc) {
    return GestureDetector(
      onTap: () => _showSensorSheet(theme, svc),
      child: Container(
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: [theme.card, theme.card.withAlpha(180)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: theme.cardBorder),
        ),
        child: Row(
          children: [
            Container(
              width: 36,
              height: 36,
              decoration: BoxDecoration(
                color: theme.textMuted.withAlpha(20),
                borderRadius: BorderRadius.circular(10),
              ),
              child: Icon(Icons.sensors, color: theme.textMuted, size: 18),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'SENSORS',
                    style: TextStyle(
                      color: theme.textMuted,
                      fontSize: 9,
                      letterSpacing: 1.2,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'Mag ${d.magRawX}, ${d.magRawY}, ${d.magRawZ}  •  '
                    'Up ${(d.uptime / 3600).floor()}h ${((d.uptime % 3600) / 60).floor()}m  •  '
                    'Heap ${(d.heap / 1024).floor()} KB',
                    style: TextStyle(
                      color: theme.textSecondary,
                      fontSize: 10,
                      fontFamily: 'monospace',
                    ),
                  ),
                ],
              ),
            ),
            Icon(Icons.chevron_right, color: theme.textMuted, size: 20),
          ],
        ),
      ),
    );
  }

  void _showSensorSheet(AppTheme theme, DeviceService svc) {
    showModalBottomSheet(
      context: context,
      backgroundColor: theme.card,
      shape: RoundedRectangleBorder(
        borderRadius: const BorderRadius.vertical(top: Radius.circular(24)),
        side: BorderSide(color: theme.cardBorder),
      ),
      builder: (_) => Consumer<DeviceService>(
        builder: (_, s, _) {
          final data = s.data;
          return Padding(
            padding: const EdgeInsets.fromLTRB(20, 12, 20, 24),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Container(
                  width: 36,
                  height: 3,
                  decoration: BoxDecoration(
                    color: theme.textMuted.withAlpha(80),
                    borderRadius: BorderRadius.circular(2),
                  ),
                ),
                const SizedBox(height: 16),
                Text(
                  'Live Sensor Data',
                  style: TextStyle(
                    color: theme.textPrimary,
                    fontSize: 16,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const SizedBox(height: 20),
                _sensorRow(theme, 'Heading', '${data.heading.toStringAsFixed(1)}°', theme.accent),
                _sensorRow(theme, 'Pitch / Roll', '${data.pitch.toStringAsFixed(1)}° / ${data.roll.toStringAsFixed(1)}°', theme.primary),
                _sensorRow(theme, 'Altitude', '${data.altitude.toInt()} ft', theme.secondary),
                _sensorRow(theme, 'Temperature', '${data.temperature.toStringAsFixed(1)} °C', theme.warning),
                _sensorRow(theme, 'Pressure', '${data.pressure.toStringAsFixed(1)} hPa', theme.primary),
                _sensorRow(theme, 'Magnetometer', '${data.magRawX}, ${data.magRawY}, ${data.magRawZ}', const Color(0xFFFFCC00)),
                _sensorRow(theme, 'Accelerometer', '${data.accelX.toStringAsFixed(2)}, ${data.accelY.toStringAsFixed(2)}, ${data.accelZ.toStringAsFixed(2)}', theme.primary),
                _sensorRow(theme, 'RSSI', '${data.rssi} dBm', data.rssi > -70 ? theme.success : theme.warning),
                _sensorRow(theme, 'Uptime', '${(data.uptime / 3600).floor()}h ${((data.uptime % 3600) / 60).floor()}m ${(data.uptime % 60).floor()}s', theme.textMuted),
                _sensorRow(theme, 'Free Heap', '${(data.heap / 1024).floor()} KB', theme.textMuted),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _sensorRow(AppTheme theme, String label, String value, Color valueColor) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(color: theme.textMuted, fontSize: 12),
          ),
          Text(
            value,
            style: TextStyle(
              color: valueColor,
              fontSize: 12,
              fontFamily: 'monospace',
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  static String _cardinal(double h) {
    if (h < 22 || h >= 337) return 'N';
    if (h < 67) return 'NE';
    if (h < 112) return 'E';
    if (h < 157) return 'SE';
    if (h < 202) return 'S';
    if (h < 247) return 'SW';
    if (h < 292) return 'W';
    return 'NW';
  }
}

class _CompassWidget extends StatefulWidget {
  final double heading;
  final AppTheme theme;
  const _CompassWidget({required this.heading, required this.theme});

  @override
  State<_CompassWidget> createState() => _CompassWidgetState();
}

class _CompassWidgetState extends State<_CompassWidget>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _animation;
  double _currentHeading = 0;

  @override
  void initState() {
    super.initState();
    _currentHeading = widget.heading;
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 300),
    );
    _animation = _controller.drive(Tween(begin: 0, end: 0));
    _controller.addListener(() => setState(() {}));
  }

  @override
  void didUpdateWidget(_CompassWidget old) {
    super.didUpdateWidget(old);
    if ((old.heading - widget.heading).abs() > 0.5) {
      final diff = widget.heading - _currentHeading;
      _currentHeading += diff > 180 ? diff - 360 : (diff < -180 ? diff + 360 : diff);
      _animation = _controller.drive(Tween(
        begin: _currentHeading - diff,
        end: _currentHeading,
      ));
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
    return SizedBox(
      width: 200,
      height: 200,
      child: Transform.rotate(
        angle: heading * math.pi / 180,
        child: CustomPaint(
          size: const Size(200, 200),
          painter: _CompassFacePainter(widget.theme),
        ),
      ),
    );
  }
}

class _CompassFacePainter extends CustomPainter {
  final AppTheme theme;
  _CompassFacePainter(this.theme);

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = math.min(size.width, size.height) / 2;
    final outerR = radius * 0.92;
    final innerR = radius * 0.82;

    if (theme.glow != null) {
      canvas.drawCircle(
        center,
        outerR * 0.9,
        Paint()
          ..color = theme.glow!.withAlpha(12)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 24),
      );
    }

    final bgPaint = Paint()..color = theme.surface;
    canvas.drawCircle(center, outerR, bgPaint);

    canvas.drawCircle(
      center,
      outerR,
      Paint()
        ..color = theme.cardBorder
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2,
    );

    canvas.drawCircle(
      center,
      innerR,
      Paint()
        ..color = theme.cardBorder.withAlpha(60)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1,
    );

    for (int deg = 0; deg < 360; deg += 15) {
      final angle = deg * math.pi / 180;
      final isCardinal = deg % 90 == 0;
      final isMajor = deg % 45 == 0;
      final tickLen = isCardinal ? outerR * 0.18 : (isMajor ? outerR * 0.12 : outerR * 0.07);
      final outer = Offset(
        center.dx + math.cos(angle) * innerR,
        center.dy + math.sin(angle) * innerR,
      );
      final inner = Offset(
        center.dx + math.cos(angle) * (innerR - tickLen),
        center.dy + math.sin(angle) * (innerR - tickLen),
      );
      canvas.drawLine(
        outer,
        inner,
        Paint()
          ..color = isCardinal
              ? theme.textPrimary
              : isMajor
                  ? theme.textSecondary.withAlpha(150)
                  : theme.textMuted.withAlpha(60)
          ..strokeWidth = isCardinal ? 2.5 : (isMajor ? 1.5 : 1),
      );

      if (isCardinal) {
        final label = deg == 0 ? 'N' : deg == 90 ? 'E' : deg == 180 ? 'S' : 'W';
        final lr = innerR - tickLen - 18;
        final lp = Offset(
          center.dx + math.cos(angle) * lr,
          center.dy + math.sin(angle) * lr,
        );
        final tp = TextPainter(
          text: TextSpan(
            text: label,
            style: TextStyle(
              color: theme.primary,
              fontSize: 15,
              fontWeight: FontWeight.bold,
            ),
          ),
          textDirection: TextDirection.ltr,
        )..layout();
        tp.paint(canvas, Offset(lp.dx - tp.width / 2, lp.dy - tp.height / 2));
      }
    }

    final needlePaint = Paint()
      ..color = theme.accent
      ..strokeWidth = 3
      ..strokeCap = StrokeCap.round;
    canvas.drawLine(
      center,
      Offset(center.dx, center.dy - innerR * 0.75),
      needlePaint,
    );

    canvas.drawCircle(
      center,
      innerR * 0.25,
      Paint()
        ..color = theme.accent.withAlpha(15)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 10),
    );
    canvas.drawCircle(center, 5, Paint()..color = theme.accent);
    canvas.drawCircle(center, 2.5, Paint()..color = theme.surface);
  }

  @override
  bool shouldRepaint(_CompassFacePainter o) => o.theme != theme;
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

    final bgArc = Paint()
      ..color = theme.cardBorder
      ..style = PaintingStyle.stroke
      ..strokeWidth = 6
      ..strokeCap = StrokeCap.round;
    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      startAngle,
      sweepAngle,
      false,
      bgArc,
    );

    final fillArc = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = 6
      ..strokeCap = StrokeCap.round;
    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      startAngle,
      sweepAngle * fraction,
      false,
      fillArc,
    );
  }

  @override
  bool shouldRepaint(_GForceDialPainter o) =>
      o.gValue != gValue || o.color != color || o.theme != theme;
}
