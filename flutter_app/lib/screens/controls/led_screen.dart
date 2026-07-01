import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/theme_provider.dart';
import '../../services/device_service.dart';
import '../../widgets/glass_container.dart';
import '../../theme/app_theme.dart';

const _presetColors = [
  Color(0xFFFF0000), Color(0xFFFF8800), Color(0xFFFFDD00), Color(0xFF00FF44),
  Color(0xFF00DDDD), Color(0xFF0088FF), Color(0xFF8800FF), Color(0xFFFF00FF),
  Color(0xFFFF4488), Color(0xFFFFFFDD), Color(0xFFFFEECC), Color(0xFFFFAA33),
];

const _presetLabels = ['Red', 'Orange', 'Yellow', 'Green', 'Cyan', 'Blue', 'Purple', 'Magenta', 'Pink', 'White', 'Warm White', 'Amber'];

const _patterns = <(String, IconData)>[
  ('Solid', Icons.circle), ('Pulse', Icons.blur_circular), ('Wave', Icons.waves),
  ('Police', Icons.local_police), ('Rainbow', Icons.palette), ('Fade', Icons.blur_on),
  ('Strobe', Icons.flash_on), ('Heartbeat', Icons.favorite), ('Breathe', Icons.spa),
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
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('LED settings applied'), backgroundColor: context.read<ThemeProvider>().theme.success));
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      backgroundColor: theme.surface,
      appBar: AppBar(
        title: Text('LED SETTINGS', style: TextStyle(fontSize: 13, letterSpacing: 3, fontWeight: FontWeight.w300, color: theme.textMuted)),
        leading: IconButton(icon: Icon(Icons.arrow_back), onPressed: () => Navigator.pop(context)),
      ),
      body: ListView(
        padding: EdgeInsets.all(16),
        children: [
          _previewCard(theme),
          SizedBox(height: 20),
          _sectionLabel(theme, 'COLOR'),
          SizedBox(height: 10),
          _colorGrid(theme),
          SizedBox(height: 20),
          _sectionLabel(theme, 'BRIGHTNESS'),
          SizedBox(height: 10),
          _brightnessSlider(theme),
          SizedBox(height: 20),
          _sectionLabel(theme, 'PATTERN'),
          SizedBox(height: 10),
          _patternPicker(theme),
          SizedBox(height: 20),
          _sectionLabel(theme, 'SPEED'),
          SizedBox(height: 10),
          _speedSlider(theme),
          SizedBox(height: 28),
          _applyButton(theme),
          SizedBox(height: 20),
        ],
      ),
    );
  }

  Widget _previewCard(AppTheme theme) {
    return GlassContainer(
      padding: EdgeInsets.all(24),
      child: Column(
        children: [
          AnimatedContainer(
            duration: Duration(milliseconds: 300),
            width: 80,
            height: 80,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: _ledOn ? _selectedColor : theme.textMuted.withAlpha(40),
              boxShadow: _ledOn ? [BoxShadow(color: _selectedColor.withAlpha(100), blurRadius: 32, spreadRadius: 6)] : null,
            ),
            child: Center(child: Icon(_ledOn ? Icons.lightbulb : Icons.lightbulb_outline, color: _ledOn ? Colors.white : theme.textMuted, size: 36)),
          ),
          SizedBox(height: 16),
          Row(mainAxisAlignment: MainAxisAlignment.center, children: [
            Text('LED', style: TextStyle(color: theme.textSecondary, fontSize: 14, letterSpacing: 0.5)),
            SizedBox(width: 12),
            SizedBox(height: 28, child: Switch(value: _ledOn, onChanged: (v) => setState(() => _ledOn = v))),
          ]),
        ],
      ),
    );
  }

  Widget _colorGrid(AppTheme theme) {
    return GlassContainer(
      padding: EdgeInsets.all(16),
      child: GridView.builder(
        shrinkWrap: true,
        physics: NeverScrollableScrollPhysics(),
        gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(crossAxisCount: 6, crossAxisSpacing: 12, mainAxisSpacing: 12),
        itemCount: _presetColors.length,
        itemBuilder: (_, i) {
          final selected = _selectedColorIndex == i;
          return Tooltip(
            message: _presetLabels[i],
            child: GestureDetector(
              onTap: () => setState(() => _selectedColorIndex = i),
              child: AnimatedContainer(
                duration: Duration(milliseconds: 250),
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: _presetColors[i],
                  boxShadow: selected ? [BoxShadow(color: _presetColors[i].withAlpha(120), blurRadius: 12)] : null,
                  border: Border.all(color: selected ? Colors.white : Colors.transparent, width: selected ? 2.5 : 0),
                ),
                child: selected ? Center(child: Icon(Icons.check, color: Colors.white, size: 16)) : null,
              ),
            ),
          );
        },
      ),
    );
  }

  Widget _brightnessSlider(AppTheme theme) {
    return GlassContainer(
      padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(children: [
        Icon(Icons.brightness_low, color: theme.textMuted, size: 18),
        Expanded(child: Slider(value: _brightness, min: 0, max: 255, divisions: 255, onChanged: (v) => setState(() => _brightness = v))),
        Icon(Icons.brightness_high, color: theme.textMuted, size: 18),
        SizedBox(width: 36, child: Text('${_brightness.toInt()}', textAlign: TextAlign.right, style: TextStyle(color: theme.primary, fontSize: 12, fontWeight: FontWeight.w600, fontFamily: 'monospace'))),
      ]),
    );
  }

  Widget _patternPicker(AppTheme theme) {
    return SizedBox(
      height: 88,
      child: ListView.separated(
        scrollDirection: Axis.horizontal, itemCount: _patterns.length,
        separatorBuilder: (_, _) => SizedBox(width: 8),
        itemBuilder: (_, i) {
          final selected = _selectedPatternIndex == i;
          return GestureDetector(
            onTap: () => setState(() => _selectedPatternIndex = i),
            child: AnimatedContainer(
              duration: Duration(milliseconds: 250),
              width: 76,
              decoration: BoxDecoration(
                color: theme.card,
                borderRadius: BorderRadius.circular(14),
                border: Border.all(color: selected ? theme.primary : theme.cardBorder, width: selected ? 2 : 1),
                boxShadow: selected ? [BoxShadow(color: theme.primary.withAlpha(30), blurRadius: 8)] : null,
              ),
              child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
                Icon(_patterns[i].$2, color: selected ? theme.primary : theme.textMuted, size: 24),
                SizedBox(height: 4),
                Text(_patterns[i].$1, style: TextStyle(color: selected ? theme.primary : theme.textSecondary, fontSize: 9, fontWeight: selected ? FontWeight.w600 : FontWeight.w400)),
              ]),
            ),
          );
        },
      ),
    );
  }

  Widget _speedSlider(AppTheme theme) {
    return GlassContainer(
      padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(children: [
        Text('Slow', style: TextStyle(color: theme.textMuted, fontSize: 11)),
        Expanded(child: Slider(value: _speed, min: 0, max: 255, divisions: 255, onChanged: (v) => setState(() => _speed = v))),
        Text('Fast', style: TextStyle(color: theme.textMuted, fontSize: 11)),
      ]),
    );
  }

  Widget _applyButton(AppTheme theme) {
    return GlassButton(
      width: double.infinity,
      height: 52,
      onTap: _sending ? null : _apply,
      child: _sending
        ? SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: theme.primary))
        : Text('APPLY', style: TextStyle(color: theme.primary, fontSize: 14, fontWeight: FontWeight.w600, letterSpacing: 2)),
    );
  }

  Widget _sectionLabel(AppTheme theme, String label) {
    return Text(label, style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600));
  }
}
