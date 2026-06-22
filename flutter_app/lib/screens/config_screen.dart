import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/device_service.dart';

class ConfigScreen extends StatefulWidget {
  const ConfigScreen({super.key});
  @override
  State<ConfigScreen> createState() => _ConfigScreenState();
}

class _ConfigScreenState extends State<ConfigScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Color(0xFF0A0A0A),
      appBar: AppBar(
        backgroundColor: Color(0xFF12121A),
        title: Text('Configuration', style: TextStyle(color: Colors.white, fontSize: 16)),
        leading: IconButton(icon: Icon(Icons.arrow_back, color: Colors.white), onPressed: () => Navigator.pop(context)),
      ),
      body: SingleChildScrollView(
        padding: EdgeInsets.all(16),
        child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
          _section('Widgets'),
          _buildWidgetToggle('Clock', Icons.access_time, true),
          _buildWidgetToggle('Compass', Icons.explore, true),
          _buildWidgetToggle('Attitude', Icons.flight, true),
          _buildWidgetToggle('Alt/Temp', Icons.thermostat, true),
          _buildWidgetToggle('G-Force', Icons.speed, true),
          _buildWidgetToggle('Music', Icons.music_note, true),
          _buildWidgetToggle('Airplane', Icons.airplanemode_active, true),
          SizedBox(height: 20),
          _section('Splash Screen'),
          _buildSplashSelector(),
          SizedBox(height: 20),
          _section('Display'),
          _buildSlider('Brightness', 100, 10, 100, (v) {}),
          _buildSlider('LED Brightness', 50, 0, 100, (v) {}),
          SizedBox(height: 20),
          _section('WiFi'),
          _buildTextField('SSID', 'V'),
          _buildTextField('Password', '********', obscure: true),
          SizedBox(height: 20),
          _section('Connection'),
          _buildConnectionPanel(context),
        ]),
      ),
    );
  }

  Widget _section(String title) {
    return Padding(
      padding: EdgeInsets.only(bottom: 8),
      child: Text(title, style: TextStyle(color: Color(0xFF666666), fontSize: 10, letterSpacing: 2)),
    );
  }

  Widget _buildWidgetToggle(String name, IconData icon, bool enabled) {
    return Container(
      margin: EdgeInsets.only(bottom: 4),
      padding: EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      decoration: BoxDecoration(
        color: Color(0xFF141428),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Color(0xFF222244)),
      ),
      child: Row(children: [
        Icon(icon, color: Colors.white54, size: 18),
        SizedBox(width: 10),
        Text(name, style: TextStyle(color: Colors.white, fontSize: 13)),
        Spacer(),
        Switch(value: enabled, activeTrackColor: Color(0xFF00CC44), onChanged: (_) {}),
      ]),
    );
  }

  Widget _buildSplashSelector() {
    return Container(
      padding: EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Color(0xFF141428),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Color(0xFF222244)),
      ),
      child: Row(children: [
        Expanded(child: _splashOption('Star Trail', Icons.auto_awesome, true)),
        Expanded(child: _splashOption('Illuminati', Icons.visibility, false)),
        Expanded(child: _splashOption('VW', Icons.directions_car, false)),
      ]),
    );
  }

  Widget _splashOption(String name, IconData icon, bool selected) {
    return GestureDetector(
      onTap: () {},
      child: Container(
        padding: EdgeInsets.all(8),
        decoration: BoxDecoration(
          color: selected ? Color(0xFF222244) : Colors.transparent,
          borderRadius: BorderRadius.circular(6),
        ),
        child: Column(children: [
          Icon(icon, color: selected ? Color(0xFF00CCFF) : Colors.white38, size: 24),
          SizedBox(height: 4),
          Text(name, style: TextStyle(color: selected ? Color(0xFF00CCFF) : Colors.white54, fontSize: 10)),
        ]),
      ),
    );
  }

  Widget _buildSlider(String label, int value, int min, int max, ValueChanged<int> onChanged) {
    return Container(
      margin: EdgeInsets.only(bottom: 4),
      padding: EdgeInsets.symmetric(horizontal: 12, vertical: 4),
      decoration: BoxDecoration(
        color: Color(0xFF141428),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Color(0xFF222244)),
      ),
      child: Row(children: [
        Text(label, style: TextStyle(color: Colors.white54, fontSize: 12)),
        SizedBox(width: 8),
        Expanded(child: Slider(
          value: value.toDouble(), min: min.toDouble(), max: max.toDouble(),
          activeColor: Color(0xFF00CCFF),
          onChanged: (v) => onChanged(v.toInt()),
        )),
        Text('$value', style: TextStyle(color: Color(0xFF00FF88), fontSize: 11, fontFamily: 'monospace')),
      ]),
    );
  }

  Widget _buildTextField(String label, String value, {bool obscure = false}) {
    return Container(
      margin: EdgeInsets.only(bottom: 4),
      padding: EdgeInsets.symmetric(horizontal: 12),
      decoration: BoxDecoration(
        color: Color(0xFF0A0A12),
        borderRadius: BorderRadius.circular(6),
        border: Border.all(color: Color(0xFF333333)),
      ),
      child: TextField(
        obscureText: obscure,
        style: TextStyle(color: Colors.white, fontSize: 13),
        decoration: InputDecoration(
          labelText: label,
          labelStyle: TextStyle(color: Color(0xFF666666), fontSize: 11),
          border: InputBorder.none,
        ),
      ),
    );
  }

  Widget _buildConnectionPanel(BuildContext context) {
    var svc = Provider.of<DeviceService>(context, listen: false);
    return Container(
      padding: EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Color(0xFF141428),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Color(0xFF222244)),
      ),
      child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
        Row(children: [
          Icon(Icons.bluetooth, color: Colors.blueAccent, size: 20),
          SizedBox(width: 8),
          Text('BLE', style: TextStyle(color: Colors.white, fontSize: 13)),
          Spacer(),
          TextButton(onPressed: () {}, child: Text('Scan', style: TextStyle(color: Color(0xFF00CCFF)))),
          TextButton(
            onPressed: () {
              svc.startSimulator();
              Navigator.pushReplacementNamed(context, '/home');
            },
            child: Text('Simulator', style: TextStyle(color: Color(0xFFFF8800))),
          ),
        ]),
        SizedBox(height: 8),
        Row(children: [
          Icon(Icons.wifi, color: Colors.greenAccent, size: 20),
          SizedBox(width: 8),
          Expanded(
            child: TextField(
              style: TextStyle(color: Colors.white, fontSize: 12),
              decoration: InputDecoration(
                hintText: 'Device IP address...',
                hintStyle: TextStyle(color: Color(0xFF555555), fontSize: 12),
                border: InputBorder.none,
                isDense: true,
              ),
            ),
          ),
          TextButton(
            onPressed: () {},
            child: Text('Connect', style: TextStyle(color: Color(0xFF00CC44))),
          ),
        ]),
      ]),
    );
  }
}
