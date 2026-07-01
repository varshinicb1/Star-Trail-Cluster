import 'dart:async';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:file_picker/file_picker.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../services/update_service.dart';
import '../widgets/glass_container.dart';

class OTAScreen extends StatefulWidget {
  const OTAScreen({super.key});
  @override
  State<OTAScreen> createState() => _OTAScreenState();
}

class _OTAScreenState extends State<OTAScreen> with SingleTickerProviderStateMixin {
  double _fwProgress = 0;
  String _fwStatus = 'Select a .bin file to update firmware';
  bool _fwUploading = false;
  PlatformFile? _fwFile;
  late AnimationController _pulseController;

  bool _appUpdateExpanded = false;

  @override
  void initState() {
    super.initState();
    _pulseController = AnimationController(vsync: this, duration: Duration(seconds: 2))..repeat(reverse: true);
    _initUpdateCheck();
  }

  void _initUpdateCheck() {
    Future.delayed(Duration(seconds: 3), () {
      if (mounted) context.read<UpdateService>().checkForUpdate();
    });
  }

  @override
  void dispose() {
    _pulseController.dispose();
    super.dispose();
  }

  Future<void> _pickFirmware() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['bin'],
    );
    if (result != null && mounted) {
      setState(() {
        _fwFile = result.files.first;
        _fwStatus = 'Loaded: ${_fwFile!.name} (${(_fwFile!.size / 1024).toStringAsFixed(0)} KB)';
        _fwProgress = 0;
      });
    }
  }

  Future<void> _uploadFirmware() async {
    if (_fwFile == null || _fwFile!.path == null) {
      setState(() => _fwStatus = 'Select a firmware .bin file first');
      return;
    }
    final svc = context.read<DeviceService>();
    if (svc.mode != ConnectionMode.wifi) {
      setState(() => _fwStatus = 'Connect via WiFi first (Controls tab)');
      return;
    }

    setState(() {
      _fwUploading = true;
      _fwStatus = 'Uploading firmware...';
      _fwProgress = 0.1;
    });

    final bytes = await File(_fwFile!.path!).readAsBytes();
    final ok = await svc.uploadFirmware(bytes);

    if (!mounted) return;
    setState(() {
      _fwUploading = false;
      _fwProgress = ok ? 1.0 : 0;
      _fwStatus = ok ? 'Firmware updated! Device rebooting...' : 'Upload failed. Check connection.';
    });

    if (ok) {
      Future.delayed(Duration(seconds: 5), () {
        if (mounted) {
          setState(() {
          _fwProgress = 0;
          _fwStatus = 'Select a .bin file to update firmware';
          _fwFile = null;
        });
        }
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      backgroundColor: theme.surface,
      appBar: AppBar(
        title: Text('UPDATES', style: TextStyle(fontSize: 13, letterSpacing: 3, fontWeight: FontWeight.w300, color: theme.textMuted)),
      ),
      body: ListView(
        padding: EdgeInsets.all(20),
        children: [
          // === FIRMWARE UPDATE ===
          Text('ESP32 FIRMWARE', style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600)),
          SizedBox(height: 16),
          GlassContainer(
            padding: EdgeInsets.all(20),
            child: Column(children: [
              AnimatedBuilder(
                animation: _pulseController,
                builder: (_, child) => Transform.scale(scale: 0.96 + _pulseController.value * 0.04, child: child),
                child: Container(
                  width: 72, height: 72,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: LinearGradient(
                      colors: [theme.primary.withAlpha(20), theme.primary.withAlpha(5)],
                      begin: Alignment.topLeft, end: Alignment.bottomRight,
                    ),
                    border: Border.all(color: theme.cardBorder, width: 2),
                  ),
                  child: Center(child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(Icons.memory, color: theme.primary, size: 26),
                      SizedBox(height: 2),
                      Text('${(_fwProgress * 100).toInt()}%', style: TextStyle(color: theme.success, fontSize: 13, fontWeight: FontWeight.bold, fontFamily: 'monospace')),
                    ],
                  )),
                ),
              ),
              SizedBox(height: 16),
              ClipRRect(
                borderRadius: BorderRadius.circular(3),
                child: LinearProgressIndicator(
                  value: _fwProgress > 0 ? _fwProgress : null,
                  backgroundColor: theme.cardBorder,
                  valueColor: AlwaysStoppedAnimation<Color>(_fwUploading ? theme.warning : theme.success),
                  minHeight: 5,
                ),
              ),
              SizedBox(height: 10),
              Text(_fwStatus, style: TextStyle(color: theme.textMuted, fontSize: 11, height: 1.3)),
              SizedBox(height: 16),
              Row(children: [
                Expanded(
                  child: GlassButton(
                    height: 46,
                    onTap: _fwUploading ? null : _pickFirmware,
                    child: Text('SELECT .BIN', style: TextStyle(color: theme.primary, fontSize: 12, fontWeight: FontWeight.w600, letterSpacing: 1)),
                  ),
                ),
                SizedBox(width: 12),
                Expanded(
                  child: GlassButton(
                    height: 46,
                    bgColor: _fwUploading ? theme.card : theme.warning.withAlpha(20),
                    onTap: _fwUploading || _fwFile == null ? null : _uploadFirmware,
                    child: _fwUploading
                      ? SizedBox(width: 18, height: 18, child: CircularProgressIndicator(strokeWidth: 2, color: theme.warning))
                      : Text('UPLOAD', style: TextStyle(color: _fwFile == null ? theme.textMuted : theme.warning, fontSize: 12, fontWeight: FontWeight.bold, letterSpacing: 1.5)),
                  ),
                ),
              ]),
            ]),
          ),

          SizedBox(height: 32),

          // === APP UPDATE ===
          GestureDetector(
            onTap: () => setState(() => _appUpdateExpanded = !_appUpdateExpanded),
            child: Row(children: [
              Text('STAR TRAIL APP', style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600)),
              Spacer(),
              Consumer<UpdateService>(builder: (_, us, _) {
                if (us.checking) return SizedBox(width: 14, height: 14, child: CircularProgressIndicator(strokeWidth: 1.5, color: theme.textMuted));
                if (us.updateAvailable) {
                  return Container(
                  padding: EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                  decoration: BoxDecoration(color: theme.success.withAlpha(25), borderRadius: BorderRadius.circular(20)),
                  child: Text('v${us.latestUpdate!.version}', style: TextStyle(color: theme.success, fontSize: 10, fontWeight: FontWeight.bold)),
                );
                }
                return SizedBox.shrink();
              }),
            ]),
          ),
          SizedBox(height: 14),
          Consumer<UpdateService>(
            builder: (_, us, _) {
              return GlassContainer(
                padding: EdgeInsets.all(18),
                child: Column(children: [
                  Row(children: [
                    Container(
                      width: 44, height: 44,
                      decoration: BoxDecoration(
                        gradient: LinearGradient(colors: [theme.primary.withAlpha(25), theme.primary.withAlpha(10)], begin: Alignment.topLeft, end: Alignment.bottomRight),
                        borderRadius: BorderRadius.circular(12),
                      ),
                      child: Icon(Icons.smartphone, color: theme.primary, size: 22),
                    ),
                    SizedBox(width: 14),
                    Expanded(child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text('Star Trail Companion', style: TextStyle(color: theme.textPrimary, fontSize: 14, fontWeight: FontWeight.w600)),
                        SizedBox(height: 3),
                        Text(
                          us.updateAvailable
                            ? 'v${us.latestUpdate!.version} available'
                            : us.checking ? 'Checking for updates...'
                            : us.error != null ? 'Check failed'
                            : 'Up to date',
                          style: TextStyle(color: us.updateAvailable ? theme.success : theme.textMuted, fontSize: 11),
                        ),
                      ],
                    )),
                    if (us.updateAvailable)
                      Container(
                        padding: EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                        decoration: BoxDecoration(color: theme.success.withAlpha(20), borderRadius: BorderRadius.circular(20)),
                        child: Text('UPDATE', style: TextStyle(color: theme.success, fontSize: 10, fontWeight: FontWeight.bold, letterSpacing: 1)),
                      ),
                  ]),
                  if (us.error != null) ...[
                    SizedBox(height: 10),
                    Text(us.error!, style: TextStyle(color: theme.error, fontSize: 11)),
                  ],
                  if (us.updateAvailable && us.latestUpdate!.changelog.isNotEmpty) ...[
                    SizedBox(height: 14),
                    Container(
                      width: double.infinity,
                      padding: EdgeInsets.all(12),
                      decoration: BoxDecoration(
                        color: theme.card,
                        borderRadius: BorderRadius.circular(10),
                      ),
                      child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
                        Text('What\'s New', style: TextStyle(color: theme.textSecondary, fontSize: 11, fontWeight: FontWeight.w600, letterSpacing: 0.5)),
                        SizedBox(height: 6),
                        Text(us.latestUpdate!.changelog, style: TextStyle(color: theme.textMuted, fontSize: 11, height: 1.5)),
                      ]),
                    ),
                  ],
                  SizedBox(height: 14),
                  if (us.updateAvailable && !us.downloading)
                    GlassButton(
                      width: double.infinity, height: 46,
                      onTap: () async {
                        final path = await us.downloadApk();
                        if (path != null && context.mounted) {
                          await us.installApk(path);
                        }
                      },
                      child: Text('DOWNLOAD & INSTALL', style: TextStyle(color: theme.primary, fontSize: 12, fontWeight: FontWeight.w600, letterSpacing: 1.5)),
                    ),
                  if (us.downloading) ...[
                    ClipRRect(
                      borderRadius: BorderRadius.circular(3),
                      child: LinearProgressIndicator(
                        value: us.downloadProgress,
                        backgroundColor: theme.cardBorder,
                        valueColor: AlwaysStoppedAnimation<Color>(theme.success),
                        minHeight: 5,
                      ),
                    ),
                    SizedBox(height: 8),
                    Text('Downloading... ${(us.downloadProgress * 100).toInt()}%',
                      style: TextStyle(color: theme.textMuted, fontSize: 11)),
                  ],
                  if (!us.updateAvailable && !us.checking) ...[
                    GlassButton(
                      width: double.infinity, height: 44,
                      bgColor: theme.card,
                      onTap: () => us.checkForUpdate(),
                      child: Text('CHECK FOR UPDATES', style: TextStyle(color: theme.textSecondary, fontSize: 11, fontWeight: FontWeight.w600, letterSpacing: 1)),
                    ),
                  ],
                ]),
              );
            },
          ),
          SizedBox(height: 40),
        ],
      ),
    );
  }
}
