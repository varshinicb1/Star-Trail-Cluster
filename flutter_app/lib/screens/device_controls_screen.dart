import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../widgets/glass_container.dart';
import 'controls/widget_screen.dart';
import 'controls/led_screen.dart';
import 'controls/system_screen.dart';
import 'controls/attitude_style_screen.dart';
import 'controls/factory_calibrate_screen.dart';

class _MenuCard {
  final String title;
  final String description;
  final IconData icon;
  final Widget screen;
  final Color accent;
  const _MenuCard(this.title, this.description, this.icon, this.screen, this.accent);
}

final _menuCards = [
  _MenuCard('Widget Settings', 'Arrange, toggle & configure widgets', Icons.grid_view, WidgetScreen(), Color(0xFF00CCFF)),
  _MenuCard('Attitude Style', 'Pick your attitude-indicator face', Icons.flight, AttitudeStyleScreen(), Color(0xFF6FA8C7)),
  _MenuCard('LED Settings', 'LED colors, patterns & brightness', Icons.lightbulb, LEDScreen(), Color(0xFFFFAA00)),
  _MenuCard('System Settings', 'Display, WiFi, reboot & info', Icons.settings, SystemScreen(), Color(0xFF00FF88)),
  _MenuCard('Factory Calibration', 'One-time mount + magnetometer setup', Icons.build_circle_outlined, FactoryCalibrateScreen(), Color(0xFFE01020)),
];

class DeviceControlsScreen extends StatelessWidget {
  const DeviceControlsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    final isConnected = context.watch<DeviceService>().isConnected;
    return Scaffold(
      backgroundColor: theme.surface,
      appBar: AppBar(
        title: Text(
          'CONTROLS',
          style: TextStyle(fontSize: 13, letterSpacing: 3, fontWeight: FontWeight.w300, color: theme.textMuted),
        ),
      ),
      body: Padding(
        padding: EdgeInsets.fromLTRB(16, 8, 16, 16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Padding(
              padding: EdgeInsets.only(left: 4, bottom: 12),
              child: Text(
                isConnected ? 'DEVICE CONFIGURATION' : 'CONNECT TO UNLOCK',
                style: TextStyle(color: isConnected ? theme.primary.withAlpha(150) : theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600),
              ),
            ),
            ...List.generate(_menuCards.length, (i) {
              final card = _menuCards[i];
              return Padding(
                padding: EdgeInsets.only(bottom: 12),
                child: _MenuCardWidget(card: card, isConnected: isConnected, onTap: () {
                  if (isConnected) Navigator.push(context, MaterialPageRoute(builder: (_) => card.screen));
                }),
              );
            }),
          ],
        ),
      ),
    );
  }
}

class _MenuCardWidget extends StatelessWidget {
  final _MenuCard card;
  final bool isConnected;
  final VoidCallback onTap;
  const _MenuCardWidget({required this.card, required this.isConnected, required this.onTap});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return GlassContainer(
      padding: EdgeInsets.all(18),
      borderColor: isConnected ? card.accent.withAlpha(25) : theme.cardBorder.withAlpha(80),
      bgColor: isConnected ? theme.card : theme.card.withAlpha(120),
      onTap: onTap,
      child: Stack(
        children: [
          Row(
            children: [
              Container(
                width: 50,
                height: 50,
                decoration: BoxDecoration(
                  gradient: LinearGradient(
                    colors: [card.accent.withAlpha(25), card.accent.withAlpha(10)],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                  borderRadius: BorderRadius.circular(14),
                ),
                child: Icon(card.icon, color: isConnected ? card.accent : theme.textMuted, size: 24),
              ),
              SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(card.title, style: TextStyle(color: isConnected ? theme.textPrimary : theme.textMuted, fontSize: 15, fontWeight: FontWeight.w600, letterSpacing: 0.3)),
                    SizedBox(height: 4),
                    Text(card.description, style: TextStyle(color: isConnected ? theme.textSecondary : theme.textMuted.withAlpha(120), fontSize: 11, letterSpacing: 0.2)),
                  ],
                ),
              ),
              Icon(Icons.chevron_right, color: isConnected ? theme.textMuted : theme.textMuted.withAlpha(60), size: 20),
            ],
          ),
          if (!isConnected)
            Positioned.fill(
              child: Container(
                decoration: BoxDecoration(borderRadius: BorderRadius.circular(16)),
                child: Center(
                  child: Container(
                    padding: EdgeInsets.all(8),
                    decoration: BoxDecoration(color: theme.surface.withAlpha(180), borderRadius: BorderRadius.circular(30)),
                    child: Icon(Icons.lock, color: theme.textMuted, size: 20),
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }
}
