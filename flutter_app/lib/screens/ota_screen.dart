import 'package:flutter/material.dart';

class OTAScreen extends StatefulWidget {
  const OTAScreen({super.key});
  @override
  State<OTAScreen> createState() => _OTAScreenState();
}

class _OTAScreenState extends State<OTAScreen> {
  final double _progress = 0;
  final String _status = 'Ready';

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Color(0xFF0A0A0A),
      appBar: AppBar(
        backgroundColor: Color(0xFF12121A),
        title: Text('Firmware Update', style: TextStyle(color: Colors.white, fontSize: 16)),
        leading: IconButton(icon: Icon(Icons.arrow_back, color: Colors.white), onPressed: () => Navigator.pop(context)),
      ),
      body: Padding(
        padding: EdgeInsets.all(20),
        child: Column(children: [
          SizedBox(height: 20),
          Container(
            width: 160, height: 160,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              border: Border.all(color: Color(0xFF333333), width: 2),
            ),
            child: Center(child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(Icons.system_update, color: Color(0xFF00CCFF), size: 40),
                SizedBox(height: 8),
                Text('${(_progress * 100).toInt()}%', style: TextStyle(color: Color(0xFF00FF88), fontSize: 24, fontWeight: FontWeight.bold)),
              ],
            )),
          ),
          SizedBox(height: 20),
          Container(
            width: double.infinity,
            height: 4,
            decoration: BoxDecoration(borderRadius: BorderRadius.circular(2), color: Color(0xFF222222)),
            child: FractionallySizedBox(
              alignment: Alignment.centerLeft,
              widthFactor: _progress,
              child: Container(
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(2),
                  gradient: LinearGradient(colors: [Color(0xFF00CCFF), Color(0xFF00FF88)]),
                ),
              ),
            ),
          ),
          SizedBox(height: 12),
          Text(_status, style: TextStyle(color: Color(0xFF888888), fontSize: 12)),
          SizedBox(height: 24),
          Container(
            width: double.infinity,
            padding: EdgeInsets.all(20),
            decoration: BoxDecoration(
              border: Border.all(color: Color(0xFF333333), style: BorderStyle.solid, width: 1),
              borderRadius: BorderRadius.circular(10),
              color: Color(0xFF0D0D1A),
            ),
            child: Column(children: [
              Icon(Icons.cloud_upload, color: Colors.white38, size: 48),
              SizedBox(height: 12),
              Text('Tap to select firmware .bin file', style: TextStyle(color: Color(0xFF888888), fontSize: 13)),
              SizedBox(height: 12),
              ElevatedButton.icon(
                onPressed: () {},
                icon: Icon(Icons.file_open),
                label: Text('Select Firmware'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Color(0xFF1A1A2E),
                  foregroundColor: Color(0xFF00CCFF),
                  side: BorderSide(color: Color(0xFF00CCFF)),
                ),
              ),
            ]),
          ),
          SizedBox(height: 16),
          SizedBox(
            width: double.infinity,
            child: ElevatedButton(
              onPressed: () {},
              style: ElevatedButton.styleFrom(
                backgroundColor: Color(0xFF00CC44),
                foregroundColor: Colors.white,
                padding: EdgeInsets.symmetric(vertical: 14),
              ),
              child: Text('Upload & Update', style: TextStyle(fontWeight: FontWeight.bold)),
            ),
          ),
        ]),
      ),
    );
  }
}
