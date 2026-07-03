import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/theme_provider.dart';
import '../../services/device_service.dart';
import '../../widgets/glass_container.dart';

/// Attitude-indicator style names/descriptions — index MUST match the
/// firmware's AttitudeStyle enum in BenzCluster/attitude.h exactly.
const _styles = [
  (name: 'Classic', desc: 'ICAO round: sky/ground, pitch ladder, bank arc', icon: Icons.flight),
  (name: 'Fullscreen', desc: 'Immersive horizon, no bezel', icon: Icons.crop_free),
  (name: 'Minimal', desc: 'Black + white line horizon', icon: Icons.horizontal_rule),
  (name: 'Tape / EFIS', desc: 'Glass-cockpit pitch tape', icon: Icons.view_agenda),
];

/// Lets the user pick which attitude-indicator style renders on the device —
/// like choosing a watch face. Sends `attitude_style=N` over BLE/WiFi.
class AttitudeStyleScreen extends StatefulWidget {
  const AttitudeStyleScreen({super.key});
  @override
  State<AttitudeStyleScreen> createState() => _AttitudeStyleScreenState();
}

class _AttitudeStyleScreenState extends State<AttitudeStyleScreen> {
  int _selected = 0;
  bool _sending = false;

  Future<void> _select(int i) async {
    setState(() { _selected = i; _sending = true; });
    final svc = context.read<DeviceService>();
    final reply = await svc.sendCommandWithReply('attitude_style=$i');
    if (mounted) {
      setState(() => _sending = false);
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(
        content: Text(reply != null
            ? 'Attitude style set to ${_styles[i].name}'
            : 'Could not reach device — check connection'),
      ));
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    final connected = context.watch<DeviceService>().isConnected;
    return Scaffold(
      appBar: AppBar(title: const Text('Attitude Style')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Text('Choose the attitude-indicator style shown on the gadget — like '
              'picking a watch face. Applies instantly.',
              style: TextStyle(color: theme.textSecondary, fontSize: 13, height: 1.4)),
          const SizedBox(height: 20),
          for (int i = 0; i < _styles.length; i++) ...[
            _StyleCard(
              index: i,
              name: _styles[i].name,
              desc: _styles[i].desc,
              icon: _styles[i].icon,
              selected: _selected == i,
              enabled: connected && !_sending,
              onTap: () => _select(i),
            ),
            const SizedBox(height: 12),
          ],
          if (!connected)
            Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Text('Connect to the device to change the style.',
                  style: TextStyle(color: theme.warning, fontSize: 12)),
            ),
        ],
      ),
    );
  }
}

class _StyleCard extends StatelessWidget {
  final int index;
  final String name;
  final String desc;
  final IconData icon;
  final bool selected;
  final bool enabled;
  final VoidCallback onTap;
  const _StyleCard({
    required this.index,
    required this.name,
    required this.desc,
    required this.icon,
    required this.selected,
    required this.enabled,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return GlassContainer(
      onTap: enabled ? onTap : null,
      borderColor: selected ? theme.primary : theme.cardBorder,
      bgColor: selected ? theme.primary.withAlpha(15) : null,
      padding: const EdgeInsets.all(16),
      child: Row(
        children: [
          Container(
            width: 46,
            height: 46,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: (selected ? theme.primary : theme.textMuted).withAlpha(30),
            ),
            child: Icon(icon, color: selected ? theme.primary : theme.textMuted, size: 22),
          ),
          const SizedBox(width: 14),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(name, style: TextStyle(color: theme.textPrimary, fontSize: 15, fontWeight: FontWeight.w600)),
                const SizedBox(height: 2),
                Text(desc, style: TextStyle(color: theme.textMuted, fontSize: 12)),
              ],
            ),
          ),
          if (selected) Icon(Icons.check_circle, color: theme.primary, size: 22)
          else Icon(Icons.circle_outlined, color: theme.textMuted.withAlpha(120), size: 22),
        ],
      ),
    );
  }
}
