import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/device_service.dart';
import '../painters/widget_painters.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<DeviceService>(
      builder: (_, svc, __) => Scaffold(
        backgroundColor: Color(0xFF0A0A0A),
        appBar: AppBar(
          backgroundColor: Color(0xFF12121A),
          title: Text('Star Trail', style: TextStyle(color: Colors.white, fontSize: 16)),
          actions: [
            Padding(
              padding: EdgeInsets.only(right: 12),
              child: Row(children: [
                Container(width: 8, height: 8, decoration: BoxDecoration(
                  color: svc.isConnected ? Color(0xFF00FF88) : Color(0xFFFF4444),
                  shape: BoxShape.circle, boxShadow: [
                    BoxShadow(color: svc.isConnected ? Color(0xFF00FF88) : Color(0xFFFF4444), blurRadius: 6),
                  ],
                )),
                SizedBox(width: 6),
                Text(
                  svc.isConnected ? svc.data.ip : 'Offline',
                  style: TextStyle(color: Colors.white54, fontSize: 10),
                ),
              ]),
            ),
          ],
        ),
        body: SingleChildScrollView(
          padding: EdgeInsets.all(12),
          child: Column(children: [
            _buildHighlightCards(svc),
            SizedBox(height: 12),
            _buildWidgetPreview(context, svc),
            SizedBox(height: 12),
            _buildSensorDetail(svc),
          ]),
        ),
      ),
    );
  }

  Widget _buildHighlightCards(DeviceService svc) {
    var d = svc.data;
    return Row(children: [
      _card('${d.heading.toStringAsFixed(0)}°', 'HEADING', Color(0xFF00FF88)),
      _card('${d.pitch.toStringAsFixed(1)}°', 'PITCH', Color(0xFF4488FF)),
      _card('${d.roll.toStringAsFixed(1)}°', 'ROLL', Color(0xFFFF6644)),
      _card('${d.temperature.toStringAsFixed(1)}°C', 'TEMP', Color(0xFFFFCC00)),
    ]);
  }

  Widget _card(String val, String label, Color c) {
    return Expanded(child: Container(
      margin: EdgeInsets.all(3),
      padding: EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: Color(0xFF141428),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Color(0xFF222244)),
      ),
      child: Column(children: [
        Text(val, style: TextStyle(color: c, fontSize: 16, fontWeight: FontWeight.bold)),
        SizedBox(height: 2),
        Text(label, style: TextStyle(color: Color(0xFF666666), fontSize: 9)),
      ]),
    ));
  }

  Widget _buildWidgetPreview(BuildContext context, DeviceService svc) {
    var d = svc.data;
    return GestureDetector(
      onTap: () => Navigator.pushNamed(context, '/emulator'),
      child: Container(
        width: 240, height: 240,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          border: Border.all(color: Color(0xFF333333), width: 2),
          boxShadow: [BoxShadow(color: Colors.black54, blurRadius: 12)],
        ),
        child: ClipOval(child: CustomPaint(
          painter: AltTempPainter(d.altitude, d.temperature),
          size: Size(240, 240),
        )),
      ),
    );
  }

  Widget _buildSensorDetail(DeviceService svc) {
    var d = svc.data;
    return Container(
      padding: EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Color(0xFF0D0D1A),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: Color(0xFF1A1A2E)),
      ),
      child: Column(children: [
        _row('Altitude', '${d.altitude.toStringAsFixed(0)} ft'),
        _row('Pressure', '${d.pressure.toStringAsFixed(1)} hPa'),
        _row('Mag Raw', '${d.magRawX}, ${d.magRawY}, ${d.magRawZ}'),
        _row('RSSI', '${d.rssi} dBm'),
        _row('Uptime', '${(d.uptime / 3600).floor()}h ${((d.uptime % 3600) / 60).floor()}m'),
        _row('Free Heap', '${(d.heap / 1024).floor()} KB'),
      ]),
    );
  }

  Widget _row(String label, String value) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 2),
      child: Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [
        Text(label, style: TextStyle(color: Color(0xFF555555), fontSize: 11)),
        Text(value, style: TextStyle(color: Color(0xFF00FF88), fontSize: 11, fontFamily: 'monospace')),
      ]),
    );
  }
}
