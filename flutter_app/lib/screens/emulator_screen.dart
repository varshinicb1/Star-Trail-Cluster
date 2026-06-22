import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/device_service.dart';
import '../painters/widget_painters.dart';

class EmulatorScreen extends StatefulWidget {
  const EmulatorScreen({super.key});
  @override
  State<EmulatorScreen> createState() => _EmulatorScreenState();
}

class _EmulatorScreenState extends State<EmulatorScreen> {
  int _currentWidget = 0;
  final List<String> _widgetNames = [
    'Clock', 'Compass', 'Attitude', 'AltTemp', 'G-Force', 'Music', 'Airplane',
  ];

  void _nextWidget() => setState(() => _currentWidget = (_currentWidget + 1) % _widgetNames.length);
  void _prevWidget() => setState(() => _currentWidget = (_currentWidget - 1 + _widgetNames.length) % _widgetNames.length);

  @override
  Widget build(BuildContext context) {
    return Consumer<DeviceService>(
      builder: (_, svc, __) {
        var d = svc.data;
        var now = DateTime.now();

        return Scaffold(
          backgroundColor: Color(0xFF0A0A0A),
          appBar: AppBar(
            backgroundColor: Color(0xFF12121A),
            title: Text('${_widgetNames[_currentWidget]} Emulator', style: TextStyle(color: Colors.white, fontSize: 14)),
            leading: IconButton(icon: Icon(Icons.arrow_back, color: Colors.white), onPressed: () => Navigator.pop(context)),
            actions: [
              TextButton(onPressed: _prevWidget, child: Text('◀', style: TextStyle(color: Colors.white54, fontSize: 20))),
              TextButton(onPressed: _nextWidget, child: Text('▶', style: TextStyle(color: Colors.white54, fontSize: 20))),
            ],
          ),
          body: Column(
            children: [
              SizedBox(height: 12),
              // Display
              GestureDetector(
                onHorizontalDragEnd: (d) {
                  if (d.primaryVelocity! < 0) _nextWidget();
                  else _prevWidget();
                },
                child: Center(
                  child: Container(
                    width: 248, height: 248,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      border: Border.all(color: Color(0xFF555555), width: 2),
                      boxShadow: [
                        BoxShadow(color: Colors.black87, blurRadius: 20, spreadRadius: 2),
                      ],
                    ),
                    child: ClipOval(child: _buildWidget(svc, d, now)),
                  ),
                ),
              ),
              SizedBox(height: 16),
              // Widget name
              Text(_widgetNames[_currentWidget], style: TextStyle(color: Color(0xFF00CCFF), fontSize: 13, letterSpacing: 2)),
              SizedBox(height: 12),
              // Sensor simulation controls
              if (svc.mode == ConnectionMode.simulator)
                _buildSensorControls(svc),
              // Live data strip
              _buildDataStrip(d),
            ],
          ),
        );
      },
    );
  }

  Widget _buildWidget(DeviceService svc, dynamic d, DateTime now) {
    switch (_widgetNames[_currentWidget]) {
      case 'Clock':
        return CustomPaint(painter: ClockPainter(now.hour, now.minute, now.second, 0), size: Size(240, 240));
      case 'Compass':
        return CustomPaint(painter: CompassPainter(d.heading, 'N'), size: Size(240, 240));
      case 'Attitude':
        return CustomPaint(painter: AttitudePainter(d.pitch, d.roll), size: Size(240, 240));
      case 'AltTemp':
        return CustomPaint(painter: AltTempPainter(d.altitude, d.temperature), size: Size(240, 240));
      case 'G-Force':
        return CustomPaint(painter: GForcePainter(d.accelX, d.accelY, d.accelZ), size: Size(240, 240));
      case 'Music':
        return CustomPaint(painter: MusicPainter('Star Trail', 'Instrument Cluster', true), size: Size(240, 240));
      case 'Airplane':
        return CustomPaint(painter: AirplanePainter(d.pitch, d.roll, d.heading), size: Size(240, 240));
      default:
        return SizedBox();
    }
  }

  Widget _buildSensorControls(DeviceService svc) {
    return Container(
      margin: EdgeInsets.symmetric(horizontal: 20),
      padding: EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Color(0xFF141428),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Color(0xFF222244)),
      ),
      child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
        Text('SENSOR SIMULATION', style: TextStyle(color: Color(0xFF666666), fontSize: 10, letterSpacing: 1)),
        SizedBox(height: 8),
        Row(children: [
          Text('Pitch', style: TextStyle(color: Colors.white54, fontSize: 11)),
          SizedBox(width: 8),
          Expanded(child: Slider(
            value: svc.data.pitch.clamp(-30, 30),
            min: -30, max: 30,
            activeColor: Color(0xFF4488FF),
            onChanged: (v) => svc.setSimPitchRoll(v, svc.data.roll),
          )),
          Text('${svc.data.pitch.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFF4488FF), fontSize: 11, fontFamily: 'monospace')),
        ]),
        Row(children: [
          Text('Roll', style: TextStyle(color: Colors.white54, fontSize: 11)),
          SizedBox(width: 8),
          Expanded(child: Slider(
            value: svc.data.roll.clamp(-60, 60),
            min: -60, max: 60,
            activeColor: Color(0xFFFF6644),
            onChanged: (v) => svc.setSimPitchRoll(svc.data.pitch, v),
          )),
          Text('${svc.data.roll.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFFFF6644), fontSize: 11, fontFamily: 'monospace')),
        ]),
        Row(children: [
          Text('Heading', style: TextStyle(color: Colors.white54, fontSize: 11)),
          SizedBox(width: 8),
          Expanded(child: Slider(
            value: svc.data.heading,
            min: 0, max: 360,
            activeColor: Color(0xFF00FF88),
            onChanged: (v) => svc.setSimHeading(v),
          )),
          Text('${svc.data.heading.toStringAsFixed(0)}°', style: TextStyle(color: Color(0xFF00FF88), fontSize: 11, fontFamily: 'monospace')),
        ]),
      ]),
    );
  }

  Widget _buildDataStrip(dynamic d) {
    return Padding(
      padding: EdgeInsets.all(12),
      child: Row(mainAxisAlignment: MainAxisAlignment.spaceEvenly, children: [
        _dataChip('${d.heading.toStringAsFixed(0)}°', 'HDG', Color(0xFF00FF88)),
        _dataChip('${d.pitch.toStringAsFixed(1)}°', 'P', Color(0xFF4488FF)),
        _dataChip('${d.roll.toStringAsFixed(1)}°', 'R', Color(0xFFFF6644)),
        _dataChip('${d.temperature.toStringAsFixed(1)}°', 'T', Color(0xFFFFCC00)),
        _dataChip('${d.altitude.toInt()}ft', 'ALT', Color(0xFFAA66FF)),
      ]),
    );
  }

  Widget _dataChip(String val, String label, Color c) {
    return Column(children: [
      Text(val, style: TextStyle(color: c, fontSize: 12, fontWeight: FontWeight.bold, fontFamily: 'monospace')),
      Text(label, style: TextStyle(color: Color(0xFF555555), fontSize: 9)),
    ]);
  }
}
