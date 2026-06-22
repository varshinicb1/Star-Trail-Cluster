import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import 'controls/widget_screen.dart';
import 'controls/led_screen.dart';
import 'controls/system_screen.dart';

class _MenuCard {
  final String title;
  final String description;
  final IconData icon;
  final Widget screen;
  const _MenuCard(this.title, this.description, this.icon, this.screen);
}

final _menuCards = [
  _MenuCard('Widget Settings', 'Arrange, toggle & configure widgets', Icons.grid_view, const WidgetScreen()),
  _MenuCard('LED Settings', 'LED colors, patterns & brightness', Icons.lightbulb, const LEDScreen()),
  _MenuCard('System Settings', 'Display, WiFi, reboot & info', Icons.settings, const SystemScreen()),
];

class DeviceControlsScreen extends StatelessWidget {
  const DeviceControlsScreen({super.key});
  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    final isConnected = context.watch<DeviceService>().isConnected;
    return Scaffold(
      appBar: AppBar(
        title: const Text('Device Controls'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Padding(
              padding: const EdgeInsets.only(left: 4, bottom: 4),
              child: Text(
                isConnected ? 'CONTROLS' : 'CONTROLS • CONNECT TO UNLOCK',
                style: TextStyle(
                  color: theme.textMuted,
                  fontSize: 10,
                  letterSpacing: 2,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ),
            const SizedBox(height: 8),
            ...List.generate(_menuCards.length, (i) {
              final card = _menuCards[i];
              return Padding(
                padding: const EdgeInsets.only(bottom: 12),
                child: _MenuCardWidget(
                  card: card,
                  isConnected: isConnected,
                  onTap: () {
                    if (isConnected) {
                      Navigator.push(
                        context,
                        MaterialPageRoute(builder: (_) => card.screen),
                      );
                    }
                  },
                ),
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

  const _MenuCardWidget({
    required this.card,
    required this.isConnected,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        padding: const EdgeInsets.all(18),
        decoration: BoxDecoration(
          color: isConnected ? theme.card : theme.card.withAlpha(120),
          borderRadius: BorderRadius.circular(20),
          border: Border.all(
            color: isConnected ? theme.cardBorder : theme.cardBorder.withAlpha(80),
          ),
        ),
        child: Stack(
          children: [
            Row(
              children: [
                Container(
                  width: 50,
                  height: 50,
                  decoration: BoxDecoration(
                    color: theme.primary.withAlpha(20),
                    borderRadius: BorderRadius.circular(14),
                  ),
                  child: Icon(
                    card.icon,
                    color: isConnected ? theme.primary : theme.textMuted,
                    size: 26,
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        card.title,
                        style: TextStyle(
                          color: isConnected
                              ? theme.textPrimary
                              : theme.textMuted,
                          fontSize: 15,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                      const SizedBox(height: 4),
                      Text(
                        card.description,
                        style: TextStyle(
                          color: isConnected
                              ? theme.textSecondary
                              : theme.textMuted.withAlpha(120),
                          fontSize: 12,
                        ),
                      ),
                    ],
                  ),
                ),
                Icon(
                  Icons.chevron_right,
                  color: isConnected ? theme.textMuted : theme.textMuted.withAlpha(60),
                  size: 22,
                ),
              ],
            ),
            if (!isConnected)
              Positioned.fill(
                child: Container(
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Center(
                    child: Container(
                      padding: const EdgeInsets.all(8),
                      decoration: BoxDecoration(
                        color: theme.surface.withAlpha(180),
                        borderRadius: BorderRadius.circular(30),
                      ),
                      child: Icon(
                        Icons.lock,
                        color: theme.textMuted,
                        size: 20,
                      ),
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
