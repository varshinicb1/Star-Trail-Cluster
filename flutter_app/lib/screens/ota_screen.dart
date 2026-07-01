import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../widgets/glass_container.dart';

class OTAScreen extends StatefulWidget {
  const OTAScreen({super.key});
  @override
  State<OTAScreen> createState() => _OTAScreenState();
}

class _OTAScreenState extends State<OTAScreen> with SingleTickerProviderStateMixin {
  double _progress = 0;
  String _status = 'Ready';
  bool _uploading = false;
  late AnimationController _pulseController;

  @override
  void initState() {
    super.initState();
    _pulseController = AnimationController(vsync: this, duration: Duration(seconds: 2))..repeat(reverse: true);
  }

  @override
  void dispose() {
    _pulseController.dispose();
    super.dispose();
  }

  void _startOTA() {
    setState(() { _uploading = true; _progress = 0; _status = 'Connecting...'; });
    Timer.periodic(Duration(milliseconds: 100), (timer) {
      if (!mounted || _progress >= 1.0) {
        timer.cancel();
        return;
      }
      setState(() {
        _progress = (_progress + 0.02).clamp(0, 1.0);
        _status = _progress < 0.3 ? 'Uploading...' : _progress < 0.7 ? 'Writing firmware...' : _progress < 0.95 ? 'Verifying...' : 'Complete!';
      });
      if (_progress >= 1.0) {
        Future.delayed(Duration(seconds: 2), () {
          if (mounted) setState(() { _uploading = false; _status = 'Ready'; _progress = 0; });
        });
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      backgroundColor: theme.surface,
      appBar: AppBar(
        title: Text('FIRMWARE', style: TextStyle(fontSize: 13, letterSpacing: 3, fontWeight: FontWeight.w300, color: theme.textMuted)),
      ),
      body: Padding(
        padding: EdgeInsets.all(20),
        child: Column(children: [
          SizedBox(height: 20),
          // Animated circle
          AnimatedBuilder(
            animation: _pulseController,
            builder: (_, child) => Transform.scale(scale: 0.96 + _pulseController.value * 0.04, child: child),
            child: Container(
              width: 160,
              height: 160,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: LinearGradient(
                  colors: [theme.primary.withAlpha(20), theme.primary.withAlpha(5)],
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                ),
                border: Border.all(color: theme.cardBorder, width: 2),
              ),
              child: Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Icon(Icons.system_update, color: theme.primary, size: 40),
                    SizedBox(height: 8),
                    Text(
                      '${(_progress * 100).toInt()}%',
                      style: TextStyle(color: theme.success, fontSize: 22, fontWeight: FontWeight.bold, fontFamily: 'monospace'),
                    ),
                  ],
                ),
              ),
            ),
          ),
          SizedBox(height: 20),
          // Progress bar
          GlassContainer(
            padding: EdgeInsets.all(4),
            borderRadius: BorderRadius.circular(4),
            bgColor: theme.cardBorder,
            borderColor: Colors.transparent,
            child: ClipRRect(
              borderRadius: BorderRadius.circular(2),
              child: LinearProgressIndicator(
                value: _progress,
                backgroundColor: Colors.transparent,
                valueColor: AlwaysStoppedAnimation<Color>(theme.primary),
                minHeight: 6,
              ),
            ),
          ),
          SizedBox(height: 10),
          Text(_status, style: TextStyle(color: theme.textMuted, fontSize: 12, letterSpacing: 0.5)),
          SizedBox(height: 28),
          // Upload card
          GlassContainer(
            padding: EdgeInsets.all(24),
            child: Column(children: [
              Icon(Icons.cloud_upload, color: theme.textMuted.withAlpha(80), size: 48),
              SizedBox(height: 12),
              Text('Select firmware .bin file', style: TextStyle(color: theme.textMuted, fontSize: 13, letterSpacing: 0.3)),
              SizedBox(height: 16),
              GlassButton(
                width: double.infinity,
                onTap: () {},
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Icon(Icons.file_open, size: 16, color: theme.primary),
                    SizedBox(width: 8),
                    Text('SELECT FIRMWARE', style: TextStyle(color: theme.primary, fontSize: 12, fontWeight: FontWeight.w600, letterSpacing: 1)),
                  ],
                ),
              ),
            ]),
          ),
          SizedBox(height: 16),
          GlassButton(
            width: double.infinity,
            height: 52,
            onTap: _uploading ? null : _startOTA,
            child: Container(
              decoration: BoxDecoration(
                gradient: LinearGradient(colors: [theme.success.withAlpha(40), theme.success.withAlpha(15)], begin: Alignment.topLeft, end: Alignment.bottomRight),
                borderRadius: BorderRadius.circular(14),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  if (_uploading) SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2, color: theme.success)),
                  if (_uploading) SizedBox(width: 10),
                  Text(
                    _uploading ? 'UPDATING...' : 'UPLOAD & UPDATE',
                    style: TextStyle(color: _uploading ? theme.success : theme.success, fontSize: 13, fontWeight: FontWeight.bold, letterSpacing: 1.5),
                  ),
                ],
              ),
            ),
          ),
        ]),
      ),
    );
  }
}
