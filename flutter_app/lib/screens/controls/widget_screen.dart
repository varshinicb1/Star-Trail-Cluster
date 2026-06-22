import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/theme_provider.dart';
import '../../services/device_service.dart';
import '../../theme/app_theme.dart';

class _WidgetItem {
  final String name;
  final IconData icon;
  final String key;
  _WidgetItem(this.name, this.icon, this.key);
}

final _allWidgets = [
  _WidgetItem('Clock', Icons.access_time, 'clock'),
  _WidgetItem('Compass', Icons.explore, 'compass'),
  _WidgetItem('Attitude', Icons.flight, 'attitude'),
  _WidgetItem('Alt / Temp', Icons.thermostat, 'alttemp'),
  _WidgetItem('G-Force', Icons.speed, 'gforce'),
  _WidgetItem('Music', Icons.music_note, 'music'),
  _WidgetItem('Airplane', Icons.airplanemode_active, 'airplane'),
];

const _swipeOptions = ['Up', 'Down', 'Left', 'Right'];
const _swipeIcons = [
  Icons.swipe_up,
  Icons.swipe_down,
  Icons.swipe_left,
  Icons.swipe_right,
];

const _knobOptions = [
  'Accelerometer',
  'Accel-Controlled',
  'Speed-Control',
  'Custom',
];
const _knobIcons = [
  Icons.sensors,
  Icons.alt_route,
  Icons.speed,
  Icons.tune,
];

class WidgetScreen extends StatefulWidget {
  const WidgetScreen({super.key});
  @override
  State<WidgetScreen> createState() => _WidgetScreenState();
}

class _WidgetScreenState extends State<WidgetScreen> {
  late Map<String, bool> _enabled;
  late List<String> _order;
  int _swipeIndex = 0;
  int _knobIndex = 0;
  bool _sending = false;

  @override
  void initState() {
    super.initState();
    _enabled = {
      for (final w in _allWidgets) w.key: true,
    };
    _order = _allWidgets.map((w) => w.key).toList();
  }

  void _onReorder(int oldIndex, int newIndex) {
    setState(() {
      final item = _order.removeAt(oldIndex);
      _order.insert(newIndex, item);
    });
  }

  Future<void> _apply() async {
    setState(() => _sending = true);
    final svc = context.read<DeviceService>();
    final enabled = _order.where((k) => _enabled[k] ?? true).join(',');
    final swipe = _swipeOptions[_swipeIndex].toLowerCase();
    final knob = _knobOptions[_knobIndex].toLowerCase().replaceAll('-', '_');
    final cmds = [
      'widgets=$enabled',
      'widget_order=${_order.join(',')}',
      'swipe_dir=$swipe',
      'knob_mode=$knob',
    ];
    for (final cmd in cmds) {
      await svc.sendCommand(cmd);
    }
    if (mounted) {
      setState(() => _sending = false);
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: const Text('Widget settings applied'),
          backgroundColor: context.read<ThemeProvider>().theme.success,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    final displayNames = {
      for (final w in _allWidgets) w.key: w.name,
    };
    final displayIcons = {
      for (final w in _allWidgets) w.key: w.icon,
    };

    return Scaffold(
      appBar: AppBar(
        title: const Text('Widget Settings'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => Navigator.pop(context),
        ),
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _sectionLabel(theme, 'VISIBLE WIDGETS'),
          const SizedBox(height: 8),
          Container(
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(16),
              border: Border.all(color: theme.cardBorder),
            ),
            child: Column(
              children: _allWidgets.map((w) {
                final enabled = _enabled[w.key] ?? true;
                return Container(
                  decoration: BoxDecoration(
                    border: w != _allWidgets.last
                        ? Border(
                            bottom: BorderSide(color: theme.cardBorder),
                          )
                        : null,
                  ),
                  child: ListTile(
                    leading: Icon(
                      w.icon,
                      color: enabled ? theme.primary : theme.textMuted,
                      size: 22,
                    ),
                    title: Text(
                      w.name,
                      style: TextStyle(
                        color: enabled
                            ? theme.textPrimary
                            : theme.textMuted,
                        fontSize: 14,
                      ),
                    ),
                    trailing: Switch(
                      value: enabled,
                      onChanged: (v) =>
                          setState(() => _enabled[w.key] = v),
                    ),
                  ),
                );
              }).toList(),
            ),
          ),
          const SizedBox(height: 24),
          _sectionLabel(theme, 'WIDGET ORDER'),
          const SizedBox(height: 8),
          Container(
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(16),
              border: Border.all(color: theme.cardBorder),
            ),
            child: ReorderableListView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              itemCount: _order.length,
              onReorderItem: _onReorder,
              proxyDecorator: (child, i, anim) => AnimatedBuilder(
                animation: anim,
                builder: (_, c) => Material(
                  color: Colors.transparent,
                  elevation: 4,
                  shadowColor: theme.primary.withAlpha(40),
                  borderRadius: BorderRadius.circular(12),
                  child: c,
                ),
                child: child,
              ),
              itemBuilder: (_, i) {
                final key = _order[i];
                final name = displayNames[key] ?? key;
                final icon = displayIcons[key] ?? Icons.widgets;
                final enabled = _enabled[key] ?? true;
                return Container(
                  key: ValueKey(key),
                  decoration: BoxDecoration(
                    border: i != _order.length - 1
                        ? Border(
                            bottom: BorderSide(color: theme.cardBorder),
                          )
                        : null,
                  ),
                  child: ListTile(
                    leading: Icon(
                      Icons.drag_handle,
                      color: theme.textMuted,
                      size: 20,
                    ),
                    title: Row(
                      children: [
                        Icon(
                          icon,
                          color: enabled
                              ? theme.textSecondary
                              : theme.textMuted,
                          size: 18,
                        ),
                        const SizedBox(width: 10),
                        Text(
                          name,
                          style: TextStyle(
                            color: enabled
                                ? theme.textPrimary
                                : theme.textMuted,
                            fontSize: 14,
                          ),
                        ),
                      ],
                    ),
                    trailing: Text(
                      '${i + 1}',
                      style: TextStyle(
                        color: theme.textMuted,
                        fontSize: 12,
                        fontFamily: 'monospace',
                      ),
                    ),
                  ),
                );
              },
            ),
          ),
          const SizedBox(height: 24),
          _sectionLabel(theme, 'SWIPE DIRECTION'),
          const SizedBox(height: 8),
          Row(
            children: List.generate(4, (i) {
              final selected = _swipeIndex == i;
              return Expanded(
                child: GestureDetector(
                  onTap: () => setState(() => _swipeIndex = i),
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 250),
                    margin: const EdgeInsets.symmetric(horizontal: 4),
                    padding: const EdgeInsets.symmetric(vertical: 16),
                    decoration: BoxDecoration(
                      color: selected
                          ? theme.primary.withAlpha(25)
                          : theme.card,
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(
                        color: selected ? theme.primary : theme.cardBorder,
                        width: selected ? 2 : 1,
                      ),
                    ),
                    child: Column(
                      children: [
                        Icon(
                          _swipeIcons[i],
                          color: selected
                              ? theme.primary
                              : theme.textMuted,
                          size: 24,
                        ),
                        const SizedBox(height: 6),
                        Text(
                          _swipeOptions[i],
                          style: TextStyle(
                            color: selected
                                ? theme.primary
                                : theme.textSecondary,
                            fontSize: 11,
                            fontWeight: selected
                                ? FontWeight.w600
                                : FontWeight.w400,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              );
            }),
          ),
          const SizedBox(height: 24),
          _sectionLabel(theme, 'ROTARY KNOB'),
          const SizedBox(height: 8),
          Row(
            children: List.generate(4, (i) {
              final selected = _knobIndex == i;
              return Expanded(
                child: GestureDetector(
                  onTap: () => setState(() => _knobIndex = i),
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 250),
                    margin: const EdgeInsets.symmetric(horizontal: 4),
                    padding: const EdgeInsets.symmetric(vertical: 16),
                    decoration: BoxDecoration(
                      color: selected
                          ? theme.primary.withAlpha(25)
                          : theme.card,
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(
                        color: selected ? theme.primary : theme.cardBorder,
                        width: selected ? 2 : 1,
                      ),
                    ),
                    child: Column(
                      children: [
                        Icon(
                          _knobIcons[i],
                          color: selected
                              ? theme.primary
                              : theme.textMuted,
                          size: 24,
                        ),
                        const SizedBox(height: 6),
                        Text(
                          _knobOptions[i],
                          style: TextStyle(
                            color: selected
                                ? theme.primary
                                : theme.textSecondary,
                            fontSize: 9,
                            fontWeight: selected
                                ? FontWeight.w600
                                : FontWeight.w400,
                          ),
                          textAlign: TextAlign.center,
                        ),
                      ],
                    ),
                  ),
                ),
              );
            }),
          ),
          const SizedBox(height: 28),
          SizedBox(
            width: double.infinity,
            height: 52,
            child: ElevatedButton(
              onPressed: _sending ? null : _apply,
              style: ElevatedButton.styleFrom(
                backgroundColor: theme.primary,
                foregroundColor: Colors.white,
                elevation: 0,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(16),
                ),
              ),
              child: _sending
                  ? SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(
                        strokeWidth: 2,
                        color: Colors.white,
                      ),
                    )
                  : const Text(
                      'Apply',
                      style: TextStyle(
                        fontSize: 15,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
            ),
          ),
          const SizedBox(height: 20),
        ],
      ),
    );
  }

  Widget _sectionLabel(AppTheme theme, String label) {
    return Text(
      label,
      style: TextStyle(
        color: theme.textMuted,
        fontSize: 10,
        letterSpacing: 2,
        fontWeight: FontWeight.w600,
      ),
    );
  }
}
