import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/theme_provider.dart';
import '../../services/device_service.dart';

const _presetColors = [
  Color(0xFFFF0000),
  Color(0xFFFF8800),
  Color(0xFFFFDD00),
  Color(0xFF00FF44),
  Color(0xFF00DDDD),
  Color(0xFF0088FF),
  Color(0xFF8800FF),
  Color(0xFFFF00FF),
  Color(0xFFFF4488),
  Color(0xFFFFFFDD),
  Color(0xFFFFEECC),
  Color(0xFFFFAA33),
];

final _presetLabels = [
  'Red',
  'Orange',
  'Yellow',
  'Green',
  'Cyan',
  'Blue',
  'Purple',
  'Magenta',
  'Pink',
  'White',
  'Warm White',
  'Amber',
];

final _patterns = <(String, IconData)>[
  ('Solid', Icons.circle),
  ('Pulse', Icons.blur_circular),
  ('Wave', Icons.waves),
  ('Police', Icons.local_police),
  ('Rainbow', Icons.palette),
  ('Fade', Icons.blur_on),
  ('Strobe', Icons.flash_on),
  ('Heartbeat', Icons.favorite),
  ('Breathe', Icons.spa),
  ('Marquee', Icons.swap_horiz),
];

class LEDScreen extends StatefulWidget {
  const LEDScreen({super.key});
  @override
  State<LEDScreen> createState() => _LEDScreenState();
}

class _LEDScreenState extends State<LEDScreen> {
  bool _ledOn = true;
  int _selectedColorIndex = 0;
  double _brightness = 128;
  int _selectedPatternIndex = 0;
  double _speed = 128;
  bool _sending = false;

  Color get _selectedColor => _presetColors[_selectedColorIndex];

  Future<void> _apply() async {
    setState(() => _sending = true);
    final svc = context.read<DeviceService>();
    final argb = _selectedColor.toARGB32();
    final hex = '#${argb.toRadixString(16).padLeft(8, '0').substring(2)}';
    final commands = [
      _ledOn ? 'led_on' : 'led_off',
      'led_color=$hex',
      'led_brightness=${_brightness.toInt()}',
      'led_pattern=${_patterns[_selectedPatternIndex].$1.toLowerCase()}',
      'led_speed=${_speed.toInt()}',
    ];
    for (final cmd in commands) {
      await svc.sendCommand(cmd);
    }
    if (mounted) {
      setState(() => _sending = false);
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: const Text('LED settings applied'),
          backgroundColor: context.read<ThemeProvider>().theme.success,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      appBar: AppBar(
        title: const Text('LED Settings'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => Navigator.pop(context),
        ),
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Container(
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: theme.cardBorder),
            ),
            child: Column(
              children: [
                AnimatedContainer(
                  duration: const Duration(milliseconds: 300),
                  width: 80,
                  height: 80,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: _ledOn
                        ? _selectedColor
                        : theme.textMuted.withAlpha(40),
                    boxShadow: _ledOn
                        ? [
                            BoxShadow(
                              color: _selectedColor.withAlpha(100),
                              blurRadius: 24,
                              spreadRadius: 4,
                            ),
                          ]
                        : null,
                  ),
                  child: Center(
                    child: Icon(
                      _ledOn ? Icons.lightbulb : Icons.lightbulb_outline,
                      color: _ledOn ? Colors.white : theme.textMuted,
                      size: 36,
                    ),
                  ),
                ),
                const SizedBox(height: 16),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      'LED',
                      style: TextStyle(
                        color: theme.textSecondary,
                        fontSize: 14,
                      ),
                    ),
                    const SizedBox(width: 12),
                    SizedBox(
                      height: 28,
                      child: Switch(
                        value: _ledOn,
                        onChanged: (v) => setState(() => _ledOn = v),
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
          const SizedBox(height: 20),
          Text(
            'COLOR',
            style: TextStyle(
              color: theme.textMuted,
              fontSize: 10,
              letterSpacing: 2,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 10),
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: theme.cardBorder),
            ),
            child: GridView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 6,
                crossAxisSpacing: 12,
                mainAxisSpacing: 12,
              ),
              itemCount: _presetColors.length,
              itemBuilder: (_, i) {
                final selected = _selectedColorIndex == i;
                return Tooltip(
                  message: _presetLabels[i],
                  child: GestureDetector(
                    onTap: () => setState(() => _selectedColorIndex = i),
                    child: AnimatedContainer(
                      duration: const Duration(milliseconds: 250),
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: _presetColors[i],
                        boxShadow: selected
                            ? [
                                BoxShadow(
                                  color: _presetColors[i].withAlpha(120),
                                  blurRadius: 12,
                                ),
                              ]
                            : null,
                        border: Border.all(
                          color: selected ? Colors.white : Colors.transparent,
                          width: selected ? 2.5 : 0,
                        ),
                      ),
                      child: selected
                          ? const Center(
                              child: Icon(
                                Icons.check,
                                color: Colors.white,
                                size: 16,
                              ),
                            )
                          : null,
                    ),
                  ),
                );
              },
            ),
          ),
          const SizedBox(height: 20),
          Text(
            'BRIGHTNESS',
            style: TextStyle(
              color: theme.textMuted,
              fontSize: 10,
              letterSpacing: 2,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 10),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: theme.cardBorder),
            ),
            child: Row(
              children: [
                Icon(Icons.brightness_low, color: theme.textMuted, size: 18),
                Expanded(
                  child: Slider(
                    value: _brightness,
                    min: 0,
                    max: 255,
                    divisions: 255,
                    onChanged: (v) => setState(() => _brightness = v),
                  ),
                ),
                Icon(Icons.brightness_high, color: theme.textMuted, size: 18),
                SizedBox(
                  width: 36,
                  child: Text(
                    '${_brightness.toInt()}',
                    textAlign: TextAlign.right,
                    style: TextStyle(
                      color: theme.primary,
                      fontSize: 12,
                      fontWeight: FontWeight.w600,
                      fontFamily: 'monospace',
                    ),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 20),
          Text(
            'PATTERN',
            style: TextStyle(
              color: theme.textMuted,
              fontSize: 10,
              letterSpacing: 2,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 10),
          SizedBox(
            height: 80,
            child: ListView.separated(
              scrollDirection: Axis.horizontal,
              itemCount: _patterns.length,
              separatorBuilder: (_, _) => const SizedBox(width: 8),
              itemBuilder: (_, i) {
                final selected = _selectedPatternIndex == i;
                return GestureDetector(
                  onTap: () => setState(() => _selectedPatternIndex = i),
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 250),
                    width: 72,
                    decoration: BoxDecoration(
                      color: theme.card,
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(
                        color: selected ? theme.primary : theme.cardBorder,
                        width: selected ? 2 : 1,
                      ),
                      boxShadow: selected
                          ? [
                              BoxShadow(
                                color: theme.primary.withAlpha(30),
                                blurRadius: 8,
                              ),
                            ]
                          : null,
                    ),
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(
                          _patterns[i].$2,
                          color: selected ? theme.primary : theme.textMuted,
                          size: 22,
                        ),
                        const SizedBox(height: 6),
                        Text(
                          _patterns[i].$1,
                          style: TextStyle(
                            color: selected
                                ? theme.primary
                                : theme.textSecondary,
                            fontSize: 9,
                            fontWeight: selected
                                ? FontWeight.w600
                                : FontWeight.w400,
                          ),
                        ),
                      ],
                    ),
                  ),
                );
              },
            ),
          ),
          const SizedBox(height: 20),
          Text(
            'SPEED',
            style: TextStyle(
              color: theme.textMuted,
              fontSize: 10,
              letterSpacing: 2,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 10),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: theme.cardBorder),
            ),
            child: Row(
              children: [
                Text(
                  'Slow',
                  style: TextStyle(color: theme.textMuted, fontSize: 11),
                ),
                Expanded(
                  child: Slider(
                    value: _speed,
                    min: 0,
                    max: 255,
                    divisions: 255,
                    onChanged: (v) => setState(() => _speed = v),
                  ),
                ),
                Text(
                  'Fast',
                  style: TextStyle(color: theme.textMuted, fontSize: 11),
                ),
              ],
            ),
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
}
