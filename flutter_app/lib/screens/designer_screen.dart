import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../models/widget_layout.dart';
import '../painters/custom_widget_painter.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../theme/app_theme.dart';

/// Smartwatch-style custom widget designer: drag/drop elements onto a 240x240
/// round canvas, edit their properties, then push the layout to the device
/// (BLE primary, WiFi fallback). Serializes to the shared JSON contract.
class DesignerScreen extends StatefulWidget {
  const DesignerScreen({super.key});
  @override
  State<DesignerScreen> createState() => _DesignerScreenState();
}

class _DesignerScreenState extends State<DesignerScreen>
    with SingleTickerProviderStateMixin {
  static const _prefsKey = 'custom_layouts';
  CwLayout _layout = CwLayout(elements: [CwElement.defaultFor(CwType.gauge)]);
  CwElement? _selected;
  bool _pushing = false;

  // Animated sample values so gauges/readouts move in the preview. Driven by a
  // vsync ticker (not a Timer) so it stays smooth and doesn't block the frame.
  final Map<CwBind, double> _values = {};
  late final AnimationController _ctrl;

  @override
  void initState() {
    super.initState();
    _selected = _layout.elements.first;
    _loadSaved();
    _ctrl = AnimationController(vsync: this, duration: const Duration(seconds: 30))
      ..addListener(_tick)
      ..repeat();
  }

  void _tick() {
    final t = _ctrl.value * 30; // seconds of phase
    setState(() {
      _values[CwBind.heading] = (t * 20) % 360;
      _values[CwBind.pitch] = 5 * math.sin(t * 0.7);
      _values[CwBind.roll] = 5 * math.cos(t * 0.6);
      _values[CwBind.temperature] = 25 + 2 * math.sin(t * 0.2);
      _values[CwBind.altitude] = 6300 + 500 * math.sin(t * 0.3);
      _values[CwBind.pressure] = 1013 + 3 * math.sin(t * 0.25);
    });
  }

  @override
  void dispose() {
    _ctrl.dispose();
    super.dispose();
  }

  Future<void> _loadSaved() async {
    final prefs = await SharedPreferences.getInstance();
    final s = prefs.getString('${_prefsKey}_active');
    if (s != null && mounted) {
      try {
        setState(() {
          _layout = CwLayout.fromJsonString(s);
          _selected = _layout.elements.isNotEmpty ? _layout.elements.first : null;
        });
      } catch (_) {}
    }
  }

  Future<void> _save() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('${_prefsKey}_active', _layout.toJsonString());
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Layout saved')),
      );
    }
  }

  void _addElement(CwType type) {
    setState(() {
      final e = CwElement.defaultFor(type);
      _layout.elements.add(e);
      _selected = e;
    });
  }

  void _deleteSelected() {
    if (_selected == null) return;
    setState(() {
      _layout.elements.remove(_selected);
      _selected = _layout.elements.isNotEmpty ? _layout.elements.last : null;
    });
  }

  Future<void> _push() async {
    final svc = context.read<DeviceService>();
    setState(() => _pushing = true);
    final res = await svc.pushLayout(_layout);
    if (!mounted) return;
    setState(() => _pushing = false);
    final msg = switch (res) {
      PushResult.ok => 'Pushed to device (BLE)',
      PushResult.okViaWifi => 'Pushed to device (WiFi)',
      PushResult.notConnected => 'Not connected — connect first',
      PushResult.failed => 'Push failed',
    };
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(msg)));
  }

  // Hit-test topmost element containing the canvas-space point.
  CwElement? _hitTest(Offset p) {
    for (final e in _layout.elements.reversed) {
      if (Rect.fromLTWH(e.x, e.y, e.w, e.h).inflate(6).contains(p)) return e;
    }
    return null;
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      backgroundColor: theme.surface,
      appBar: AppBar(
        title: const Text('Widget Designer'),
        actions: [
          IconButton(icon: const Icon(Icons.save_outlined), onPressed: _save),
          IconButton(
            icon: _pushing
                ? const SizedBox(width: 18, height: 18, child: CircularProgressIndicator(strokeWidth: 2))
                : const Icon(Icons.upload),
            onPressed: _pushing ? null : _push,
          ),
        ],
      ),
      body: LayoutBuilder(
        builder: (context, constraints) {
          // Size the round canvas to the space available so it never overflows
          // on short viewports (leaves room for palette + properties).
          double canvasSize = constraints.maxWidth - 32;
          final maxByHeight = constraints.maxHeight * 0.5;
          if (canvasSize > 300) canvasSize = 300;
          if (canvasSize > maxByHeight) canvasSize = maxByHeight;
          if (canvasSize < 100) canvasSize = 100;
          return Column(
            children: [
              const SizedBox(height: 16),
              _buildCanvas(theme, canvasSize),
              const SizedBox(height: 12),
              _buildPalette(theme),
              const Divider(height: 1),
              Expanded(child: _buildProperties(theme)),
            ],
          );
        },
      ),
    );
  }

  Widget _buildCanvas(AppTheme theme, double size) {
    return Center(
      child: Builder(builder: (context) {
        final scale = size / kCwScreen;
        return GestureDetector(
          onTapDown: (d) {
            final p = d.localPosition / scale;
            setState(() => _selected = _hitTest(p));
          },
          onPanStart: (d) {
            final p = d.localPosition / scale;
            final hit = _hitTest(p);
            if (hit != null) setState(() => _selected = hit);
          },
          onPanUpdate: (d) {
            if (_selected == null) return;
            setState(() {
              _selected!.x = (_selected!.x + d.delta.dx / scale).clamp(0, kCwScreen - 8);
              _selected!.y = (_selected!.y + d.delta.dy / scale).clamp(0, kCwScreen - 8);
            });
          },
          child: Container(
            width: size,
            height: size,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              border: Border.all(color: theme.cardBorder, width: 2),
              boxShadow: [
                BoxShadow(color: Colors.black.withAlpha(120), blurRadius: 18, spreadRadius: 2),
              ],
            ),
            child: ClipOval(
              child: CustomPaint(
                painter: CustomWidgetPainter(
                    layout: _layout, values: _values, selected: _selected),
              ),
            ),
          ),
        );
      }),
    );
  }

  Widget _buildPalette(AppTheme theme) {
    final items = [
      (CwType.gauge, Icons.speed, 'Gauge'),
      (CwType.readout, Icons.pin, 'Value'),
      (CwType.label, Icons.text_fields, 'Text'),
      (CwType.bar, Icons.linear_scale, 'Bar'),
      (CwType.icon, Icons.star, 'Icon'),
      (CwType.image, Icons.image, 'Image'),
      (CwType.shape, Icons.category, 'Shape'),
    ];
    return SizedBox(
      height: 74,
      child: ListView(
        scrollDirection: Axis.horizontal,
        padding: const EdgeInsets.symmetric(horizontal: 12),
        children: [
          for (final it in items)
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 8),
              child: InkWell(
                onTap: () => _addElement(it.$1),
                borderRadius: BorderRadius.circular(12),
                child: Container(
                  width: 62,
                  decoration: BoxDecoration(
                    color: theme.card,
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: theme.cardBorder),
                  ),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(it.$2, color: theme.primary, size: 22),
                      const SizedBox(height: 4),
                      Text(it.$3, style: TextStyle(color: theme.textSecondary, fontSize: 10)),
                    ],
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildProperties(AppTheme theme) {
    final e = _selected;
    if (e == null) {
      return Center(
        child: Text('Tap an element to edit, or add one above',
            style: TextStyle(color: theme.textMuted, fontSize: 13)),
      );
    }
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Row(
          children: [
            Text(e.type.name.toUpperCase(),
                style: TextStyle(color: theme.textPrimary, fontSize: 13, fontWeight: FontWeight.w700, letterSpacing: 1)),
            const Spacer(),
            IconButton(
              icon: Icon(Icons.delete_outline, color: theme.error, size: 20),
              onPressed: _deleteSelected,
            ),
          ],
        ),
        // Size
        _slider(theme, 'Width', e.w, 10, 240, (v) => setState(() => e.w = v)),
        _slider(theme, 'Height', e.h, 10, 240, (v) => setState(() => e.h = v)),
        // Binding (data source) — for gauge/readout/bar
        if (e.type == CwType.gauge || e.type == CwType.readout || e.type == CwType.bar) ...[
          _label(theme, 'Data source'),
          _bindDropdown(theme, e),
          _slider(theme, 'Min', e.min, -180, 12000, (v) => setState(() => e.min = v)),
          _slider(theme, 'Max', e.max, -180, 12000, (v) => setState(() => e.max = v)),
        ],
        // Font size
        if (e.type == CwType.readout || e.type == CwType.label || e.type == CwType.icon)
          _fontSelector(theme, e),
        // Text
        if (e.type == CwType.label || e.type == CwType.readout)
          _textField(theme, e.type == CwType.label ? 'Text' : 'Unit suffix', e.text,
              (v) => setState(() => e.text = v)),
        // Icon token
        if (e.type == CwType.icon) _iconSelector(theme, e),
        // Shape kind
        if (e.type == CwType.shape) _shapeSelector(theme, e),
        // Color
        _label(theme, 'Color'),
        _colorRow(theme, e),
      ],
    );
  }

  Widget _label(AppTheme theme, String s) => Padding(
        padding: const EdgeInsets.only(top: 14, bottom: 6),
        child: Text(s.toUpperCase(),
            style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 1.5)),
      );

  Widget _slider(AppTheme theme, String name, double v, double min, double max, ValueChanged<double> onCh) {
    return Row(
      children: [
        SizedBox(width: 60, child: Text(name, style: TextStyle(color: theme.textSecondary, fontSize: 12))),
        Expanded(
          child: Slider(value: v.clamp(min, max), min: min, max: max, onChanged: onCh),
        ),
        SizedBox(width: 44, child: Text(v.toStringAsFixed(0),
            textAlign: TextAlign.right, style: TextStyle(color: theme.textMuted, fontSize: 11))),
      ],
    );
  }

  Widget _bindDropdown(AppTheme theme, CwElement e) {
    return DropdownButtonFormField<CwBind>(
      initialValue: e.bind,
      dropdownColor: theme.card,
      items: CwBind.values
          .map((b) => DropdownMenuItem(value: b, child: Text(_bindName(b), style: TextStyle(color: theme.textPrimary))))
          .toList(),
      onChanged: (b) => setState(() => e.bind = b ?? CwBind.none),
    );
  }

  String _bindName(CwBind b) => switch (b) {
        CwBind.none => 'Static (no data)',
        CwBind.heading => 'Heading (0-360°)',
        CwBind.pitch => 'Pitch (°)',
        CwBind.roll => 'Roll (°)',
        CwBind.temperature => 'Temperature (°C)',
        CwBind.altitude => 'Altitude (ft)',
        CwBind.pressure => 'Pressure (hPa)',
      };

  Widget _fontSelector(AppTheme theme, CwElement e) {
    return Padding(
      padding: const EdgeInsets.only(top: 8),
      child: Row(
        children: [
          SizedBox(width: 60, child: Text('Font', style: TextStyle(color: theme.textSecondary, fontSize: 12))),
          const SizedBox(width: 8),
          for (final f in [(0, 'S'), (1, 'M'), (2, 'L')])
            Padding(
              padding: const EdgeInsets.only(right: 8),
              child: ChoiceChip(
                label: Text(f.$2),
                selected: e.font == f.$1,
                onSelected: (_) => setState(() => e.font = f.$1),
              ),
            ),
        ],
      ),
    );
  }

  Widget _textField(AppTheme theme, String label, String value, ValueChanged<String> onCh) {
    return Padding(
      padding: const EdgeInsets.only(top: 12),
      child: TextFormField(
        initialValue: value,
        style: TextStyle(color: theme.textPrimary, fontSize: 13),
        decoration: InputDecoration(labelText: label, isDense: true),
        onChanged: onCh,
      ),
    );
  }

  Widget _iconSelector(AppTheme theme, CwElement e) {
    const tokens = ['GPS', 'WIFI', 'BLUETOOTH', 'BATTERY', 'CHARGE', 'BELL', 'SETTINGS', 'POWER', 'PLAY', 'WARNING'];
    return Wrap(
      spacing: 6,
      children: [
        for (final t in tokens)
          ChoiceChip(
            avatar: Icon(iconFor(t), size: 16, color: theme.textSecondary),
            label: Text(t.substring(0, 1) + t.substring(1).toLowerCase(), style: const TextStyle(fontSize: 10)),
            selected: e.icon == t,
            onSelected: (_) => setState(() => e.icon = t),
          ),
      ],
    );
  }

  Widget _shapeSelector(AppTheme theme, CwElement e) {
    return Padding(
      padding: const EdgeInsets.only(top: 8),
      child: Row(
        children: [
          for (final s in CwShape.values)
            Padding(
              padding: const EdgeInsets.only(right: 8),
              child: ChoiceChip(
                label: Text(s.name),
                selected: e.shape == s,
                onSelected: (_) => setState(() => e.shape = s),
              ),
            ),
          const Spacer(),
          Text('Filled', style: TextStyle(color: theme.textSecondary, fontSize: 12)),
          Switch(value: e.filled, onChanged: (v) => setState(() => e.filled = v)),
        ],
      ),
    );
  }

  Widget _colorRow(AppTheme theme, CwElement e) {
    const palette = [0xC8C9CC, 0xF2F3F5, 0x6FA8C7, 0x7FD1A6, 0xE0B15A, 0xD46A6A, 0x8E9195, 0x00CCFF];
    return Wrap(
      spacing: 10,
      runSpacing: 10,
      children: [
        for (final c in palette)
          GestureDetector(
            onTap: () => setState(() => e.color = c),
            child: Container(
              width: 34,
              height: 34,
              decoration: BoxDecoration(
                color: Color(0xFF000000 | c),
                shape: BoxShape.circle,
                border: Border.all(
                  color: e.color == c ? theme.accent : theme.cardBorder,
                  width: e.color == c ? 3 : 1,
                ),
              ),
            ),
          ),
      ],
    );
  }
}
