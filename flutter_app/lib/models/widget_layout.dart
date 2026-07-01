import 'dart:convert';

/// Element kinds supported by the custom widget designer. Names must match the
/// firmware `CwType` tokens (see BenzCluster/custom_widget.h) exactly — this is
/// the shared serialization contract pushed to the device.
enum CwType { gauge, readout, label, bar, icon, image, shape }

/// Live data sources an element can bind to. Token strings must match the
/// firmware `cw_bind_from_str()` mapping.
enum CwBind { none, heading, pitch, roll, temperature, altitude, pressure }

enum CwShape { rect, circle, line }

const _bindTokens = {
  CwBind.none: '',
  CwBind.heading: 'heading',
  CwBind.pitch: 'pitch',
  CwBind.roll: 'roll',
  CwBind.temperature: 'temp',
  CwBind.altitude: 'altitude',
  CwBind.pressure: 'pressure',
};

CwBind cwBindFromToken(String? t) {
  if (t == null || t.isEmpty) return CwBind.none;
  return _bindTokens.entries
      .firstWhere((e) => e.value == t, orElse: () => const MapEntry(CwBind.none, ''))
      .key;
}

/// The round panel is 240x240; the designer canvas mirrors this 1:1.
const double kCwScreen = 240;

class CwElement {
  CwType type;
  double x, y, w, h;
  int color; // 0xRRGGBB
  CwBind bind;
  double min, max;
  int font; // 0=small 1=med 2=large
  CwShape shape;
  bool filled;
  String text; // static label text / readout unit suffix
  String icon; // symbol token for icon elements

  CwElement({
    required this.type,
    this.x = 90,
    this.y = 90,
    this.w = 60,
    this.h = 60,
    this.color = 0xC8C9CC,
    this.bind = CwBind.none,
    this.min = 0,
    this.max = 100,
    this.font = 1,
    this.shape = CwShape.rect,
    this.filled = true,
    this.text = '',
    this.icon = 'GPS',
  });

  /// Sensible defaults for a freshly-dropped element of [type].
  factory CwElement.defaultFor(CwType type) {
    switch (type) {
      case CwType.gauge:
        return CwElement(type: type, w: 140, h: 140, bind: CwBind.heading, min: 0, max: 360);
      case CwType.readout:
        return CwElement(type: type, w: 90, h: 40, bind: CwBind.temperature, text: '°C', font: 2);
      case CwType.label:
        return CwElement(type: type, w: 90, h: 30, text: 'LABEL', font: 1);
      case CwType.bar:
        return CwElement(type: type, w: 160, h: 18, bind: CwBind.altitude, min: 0, max: 12000);
      case CwType.icon:
        return CwElement(type: type, w: 40, h: 40, icon: 'GPS', font: 2);
      case CwType.image:
        return CwElement(type: type, w: 80, h: 80, filled: false);
      case CwType.shape:
        return CwElement(type: type, w: 60, h: 60, shape: CwShape.circle, color: 0x6FA8C7);
    }
  }

  Map<String, dynamic> toJson() {
    final m = <String, dynamic>{
      'type': type.name,
      'x': x.round(),
      'y': y.round(),
      'w': w.round(),
      'h': h.round(),
      'color': '#${color.toRadixString(16).padLeft(6, '0').toUpperCase()}',
    };
    if (bind != CwBind.none) m['bind'] = _bindTokens[bind];
    if (type == CwType.gauge || type == CwType.bar) {
      m['min'] = min;
      m['max'] = max;
    }
    if (type == CwType.readout || type == CwType.label || type == CwType.icon) {
      m['font'] = font;
    }
    if (type == CwType.shape) {
      m['shape'] = shape.name;
      m['filled'] = filled;
    }
    if (text.isNotEmpty) m['text'] = text;
    if (type == CwType.icon) m['icon'] = icon;
    return m;
  }

  factory CwElement.fromJson(Map<String, dynamic> j) {
    int parseColor(String? s) {
      if (s == null || !s.startsWith('#')) return 0xC8C9CC;
      return int.parse(s.substring(1), radix: 16) & 0xFFFFFF;
    }

    return CwElement(
      type: CwType.values.firstWhere((t) => t.name == (j['type'] ?? 'label'),
          orElse: () => CwType.label),
      x: (j['x'] ?? 0).toDouble(),
      y: (j['y'] ?? 0).toDouble(),
      w: (j['w'] ?? 60).toDouble(),
      h: (j['h'] ?? 60).toDouble(),
      color: parseColor(j['color']),
      bind: cwBindFromToken(j['bind']),
      min: (j['min'] ?? 0).toDouble(),
      max: (j['max'] ?? 100).toDouble(),
      font: (j['font'] ?? 1).toInt(),
      shape: CwShape.values.firstWhere((s) => s.name == (j['shape'] ?? 'rect'),
          orElse: () => CwShape.rect),
      filled: j['filled'] ?? true,
      text: j['text'] ?? '',
      icon: j['icon'] ?? 'GPS',
    );
  }

  CwElement copy() => CwElement.fromJson(toJson());
}

class CwLayout {
  String name;
  int bg; // 0xRRGGBB
  List<CwElement> elements;

  CwLayout({this.name = 'My Face', this.bg = 0x0A0A0C, List<CwElement>? elements})
      : elements = elements ?? [];

  Map<String, dynamic> toJson() => {
        'v': 1,
        'name': name,
        'bg': '#${bg.toRadixString(16).padLeft(6, '0').toUpperCase()}',
        'elements': elements.map((e) => e.toJson()).toList(),
      };

  String toJsonString() => jsonEncode(toJson());

  factory CwLayout.fromJson(Map<String, dynamic> j) {
    int parseColor(String? s) {
      if (s == null || !s.startsWith('#')) return 0x0A0A0C;
      return int.parse(s.substring(1), radix: 16) & 0xFFFFFF;
    }

    return CwLayout(
      name: j['name'] ?? 'My Face',
      bg: parseColor(j['bg']),
      elements: ((j['elements'] ?? []) as List)
          .map((e) => CwElement.fromJson(e as Map<String, dynamic>))
          .toList(),
    );
  }

  factory CwLayout.fromJsonString(String s) =>
      CwLayout.fromJson(jsonDecode(s) as Map<String, dynamic>);
}
