import 'widget_layout.dart';

/// Preloaded starter layouts for the Widget Designer, one per built-in
/// firmware widget (where its data is expressible via [CwBind]/[CwType]).
/// Loading a template gives the user a running start that already matches
/// the on-device look, rather than an empty canvas.
class CwTemplate {
  final String name;
  final String description;
  final CwLayout Function() build;
  const CwTemplate(this.name, this.description, this.build);
}

final List<CwTemplate> cwTemplates = [
  CwTemplate('Alt / Temp', 'Icon-left altitude and temperature rows, like the built-in widget', _altTemp),
  CwTemplate('Compass', 'Heading gauge with numeric readout', _compass),
  CwTemplate('Attitude', 'Pitch/roll arc with dual readouts', _attitude),
  CwTemplate('Barometer', 'Pressure bar with readout', _barometer),
];

CwLayout _altTemp() => CwLayout(
      name: 'Alt / Temp',
      bg: 0x000000,
      elements: [
        CwElement(type: CwType.icon, x: 32, y: 75, w: 44, h: 34, font: 2, icon: 'GPS', color: 0xFFFFFF),
        CwElement(type: CwType.readout, x: 92, y: 58, w: 110, h: 40, font: 2, bind: CwBind.altitude, text: ' ft', color: 0xFFFFFF),
        CwElement(type: CwType.label, x: 92, y: 104, w: 110, h: 18, font: 0, text: 'ALTITUDE', color: 0x777777),
        CwElement(type: CwType.icon, x: 38, y: 150, w: 20, h: 50, font: 2, icon: 'WARNING', color: 0xFFFFFF),
        CwElement(type: CwType.readout, x: 92, y: 150, w: 110, h: 40, font: 2, bind: CwBind.temperature, text: '°C', color: 0xFFFFFF),
        CwElement(type: CwType.label, x: 92, y: 196, w: 110, h: 18, font: 0, text: 'TEMPERATURE', color: 0x777777),
      ],
    );

CwLayout _compass() => CwLayout(
      name: 'Compass',
      bg: 0x000000,
      elements: [
        CwElement(type: CwType.gauge, x: 50, y: 40, w: 140, h: 140, bind: CwBind.heading, min: 0, max: 360, color: 0x00CCFF),
        CwElement(type: CwType.readout, x: 92, y: 168, w: 80, h: 40, font: 2, bind: CwBind.heading, text: '°', color: 0xFFFFFF),
        CwElement(type: CwType.label, x: 88, y: 210, w: 90, h: 18, font: 0, text: 'HEADING', color: 0x777777),
      ],
    );

CwLayout _attitude() => CwLayout(
      name: 'Attitude',
      bg: 0x000000,
      elements: [
        CwElement(type: CwType.gauge, x: 50, y: 20, w: 140, h: 100, bind: CwBind.pitch, min: -90, max: 90, color: 0xE0B15A),
        CwElement(type: CwType.readout, x: 36, y: 130, w: 80, h: 34, font: 1, bind: CwBind.pitch, text: '° P', color: 0xFFFFFF),
        CwElement(type: CwType.readout, x: 128, y: 130, w: 80, h: 34, font: 1, bind: CwBind.roll, text: '° R', color: 0xFFFFFF),
        CwElement(type: CwType.label, x: 78, y: 172, w: 90, h: 18, font: 0, text: 'ATTITUDE', color: 0x777777),
      ],
    );

CwLayout _barometer() => CwLayout(
      name: 'Barometer',
      bg: 0x000000,
      elements: [
        CwElement(type: CwType.bar, x: 40, y: 110, w: 160, h: 18, bind: CwBind.pressure, min: 950, max: 1050, color: 0x7FD1A6),
        CwElement(type: CwType.readout, x: 68, y: 60, w: 110, h: 40, font: 2, bind: CwBind.pressure, text: ' hPa', color: 0xFFFFFF),
        CwElement(type: CwType.label, x: 78, y: 140, w: 90, h: 18, font: 0, text: 'PRESSURE', color: 0x777777),
      ],
    );
