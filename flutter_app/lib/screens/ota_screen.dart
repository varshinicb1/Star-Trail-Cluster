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
  String _fwStatus = 'Checking for firmware updates...';
  bool _fwBusy = false;
  PlatformFile? _manualFwFile;
  bool _showManualUpload = false;
  late AnimationController _pulseController;

  @override
  void initState() {
    super.initState();
    _pulseController = AnimationController(vsync: this, duration: const Duration(seconds: 2))..repeat(reverse: true);
    // One check drives BOTH the app-update and firmware-update cards, since
    // they're published together in the same GitHub release.
    Future.delayed(const Duration(milliseconds: 300), () {
      if (mounted) context.read<UpdateService>().checkForUpdate();
    });
  }

  @override
  void dispose() {
    _pulseController.dispose();
    super.dispose();
  }

  /// One-tap firmware update: downloads the latest release .bin and pushes it
  /// straight to the device over WiFi — no manual file picking. This is what
  /// "firmware update" means day-to-day: whenever a new build (new widget,
  /// config change, bugfix) is published, this button gets it onto the device.
  Future<void> _updateFirmwareFromRelease() async {
    final us = context.read<UpdateService>();
    final svc = context.read<DeviceService>();
    final fw = us.latestFirmware;
    if (fw == null) return;

    if (svc.mode != ConnectionMode.wifi) {
      setState(() => _fwStatus = 'Connect via WiFi first (Controls tab) — firmware push needs the device\'s WiFi OTA endpoint.');
      return;
    }

    setState(() { _fwBusy = true; _fwProgress = 0; _fwStatus = 'Downloading firmware ${fw.version}...'; });

    final bytes = await us.downloadFirmwareBytes(onProgress: (p) {
      if (mounted) setState(() => _fwProgress = p * 0.5); // first half = download
    });

    if (bytes == null) {
      if (mounted) setState(() { _fwBusy = false; _fwStatus = us.error ?? 'Download failed'; });
      return;
    }

    if (mounted) setState(() { _fwProgress = 0.5; _fwStatus = 'Pushing to device...'; });
    final ok = await svc.uploadFirmware(bytes);

    if (!mounted) return;
    setState(() {
      _fwBusy = false;
      _fwProgress = ok ? 1.0 : 0;
      _fwStatus = ok ? 'Firmware ${fw.version} installed! Device rebooting...' : 'Push failed. Check connection and try again.';
    });
  }

  Future<void> _pickManualFirmware() async {
    final result = await FilePicker.platform.pickFiles(type: FileType.custom, allowedExtensions: ['bin']);
    if (result != null && mounted) {
      setState(() => _manualFwFile = result.files.first);
    }
  }

  Future<void> _uploadManualFirmware() async {
    if (_manualFwFile == null || _manualFwFile!.path == null) return;
    final svc = context.read<DeviceService>();
    if (svc.mode != ConnectionMode.wifi) {
      setState(() => _fwStatus = 'Connect via WiFi first (Controls tab)');
      return;
    }
    setState(() { _fwBusy = true; _fwStatus = 'Uploading ${_manualFwFile!.name}...'; _fwProgress = 0.1; });
    final bytes = await File(_manualFwFile!.path!).readAsBytes();
    final ok = await svc.uploadFirmware(bytes);
    if (!mounted) return;
    setState(() {
      _fwBusy = false;
      _fwProgress = ok ? 1.0 : 0;
      _fwStatus = ok ? 'Firmware updated! Device rebooting...' : 'Upload failed. Check connection.';
      if (ok) _manualFwFile = null;
    });
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
        padding: const EdgeInsets.all(20),
        children: [
          // === FIRMWARE UPDATE (auto, from GitHub release) ===
          Text('ESP32 FIRMWARE', style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600)),
          const SizedBox(height: 16),
          Consumer<UpdateService>(builder: (_, us, _) {
            final fw = us.latestFirmware;
            return GlassContainer(
              padding: const EdgeInsets.all(20),
              child: Column(children: [
                AnimatedBuilder(
                  animation: _pulseController,
                  builder: (_, child) => Transform.scale(scale: _fwBusy ? 0.96 + _pulseController.value * 0.04 : 1.0, child: child),
                  child: Container(
                    width: 72, height: 72,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      gradient: LinearGradient(colors: [theme.primary.withAlpha(20), theme.primary.withAlpha(5)], begin: Alignment.topLeft, end: Alignment.bottomRight),
                      border: Border.all(color: theme.cardBorder, width: 2),
                    ),
                    child: Center(child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(Icons.memory, color: theme.primary, size: 26),
                        const SizedBox(height: 2),
                        Text('${(_fwProgress * 100).toInt()}%', style: TextStyle(color: theme.success, fontSize: 13, fontWeight: FontWeight.bold, fontFamily: 'monospace')),
                      ],
                    )),
                  ),
                ),
                const SizedBox(height: 16),
                ClipRRect(
                  borderRadius: BorderRadius.circular(3),
                  child: LinearProgressIndicator(
                    value: _fwProgress > 0 ? _fwProgress : null,
                    backgroundColor: theme.cardBorder,
                    valueColor: AlwaysStoppedAnimation<Color>(_fwBusy ? theme.warning : theme.success),
                    minHeight: 5,
                  ),
                ),
                const SizedBox(height: 10),
                Text(
                  us.checking ? 'Checking for firmware updates...' : (fw != null ? 'Firmware ${fw.version} available' : _fwStatus),
                  style: TextStyle(color: theme.textMuted, fontSize: 11, height: 1.3),
                  textAlign: TextAlign.center,
                ),
                if (fw != null && fw.changelog.isNotEmpty) ...[
                  const SizedBox(height: 12),
                  Container(
                    width: double.infinity,
                    padding: const EdgeInsets.all(12),
                    decoration: BoxDecoration(color: theme.card, borderRadius: BorderRadius.circular(10)),
                    child: Text(fw.changelog, style: TextStyle(color: theme.textSecondary, fontSize: 11, height: 1.5)),
                  ),
                ],
                const SizedBox(height: 16),
                if (fw != null)
                  SizedBox(
                    width: double.infinity,
                    child: GlassButton(
                      height: 48,
                      bgColor: _fwBusy ? theme.card : theme.primary.withAlpha(20),
                      onTap: _fwBusy ? null : _updateFirmwareFromRelease,
                      child: _fwBusy
                          ? SizedBox(width: 18, height: 18, child: CircularProgressIndicator(strokeWidth: 2, color: theme.primary))
                          : Text('UPDATE FIRMWARE — ${fw.version}', style: TextStyle(color: theme.primary, fontSize: 12, fontWeight: FontWeight.bold, letterSpacing: 1)),
                    ),
                  )
                else if (!us.checking)
                  GlassButton(
                    width: double.infinity, height: 44,
                    bgColor: theme.card,
                    onTap: () => us.checkForUpdate(),
                    child: Text('CHECK FOR FIRMWARE', style: TextStyle(color: theme.textSecondary, fontSize: 11, fontWeight: FontWeight.w600, letterSpacing: 1)),
                  ),
                const SizedBox(height: 10),
                GestureDetector(
                  onTap: () => setState(() => _showManualUpload = !_showManualUpload),
                  child: Text(
                    _showManualUpload ? 'Hide manual .bin upload' : 'Advanced: upload a .bin file manually',
                    style: TextStyle(color: theme.textMuted, fontSize: 11, decoration: TextDecoration.underline),
                  ),
                ),
                if (_showManualUpload) ...[
                  const SizedBox(height: 12),
                  if (_manualFwFile != null)
                    Padding(
                      padding: const EdgeInsets.only(bottom: 10),
                      child: Text('Loaded: ${_manualFwFile!.name} (${(_manualFwFile!.size / 1024).toStringAsFixed(0)} KB)',
                          style: TextStyle(color: theme.textSecondary, fontSize: 11)),
                    ),
                  Row(children: [
                    Expanded(
                      child: GlassButton(
                        height: 42,
                        onTap: _fwBusy ? null : _pickManualFirmware,
                        child: Text('SELECT .BIN', style: TextStyle(color: theme.textSecondary, fontSize: 11, fontWeight: FontWeight.w600, letterSpacing: 0.5)),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: GlassButton(
                        height: 42,
                        bgColor: _fwBusy ? theme.card : theme.warning.withAlpha(20),
                        onTap: (_fwBusy || _manualFwFile == null) ? null : _uploadManualFirmware,
                        child: Text('UPLOAD', style: TextStyle(color: _manualFwFile == null ? theme.textMuted : theme.warning, fontSize: 11, fontWeight: FontWeight.bold, letterSpacing: 1)),
                      ),
                    ),
                  ]),
                ],
              ]),
            );
          }),

          const SizedBox(height: 32),

          // === APP UPDATE ===
          Row(children: [
            Text('STAR TRAIL APP', style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600)),
            const Spacer(),
            Consumer<UpdateService>(builder: (_, us, _) {
              if (us.checking) return SizedBox(width: 14, height: 14, child: CircularProgressIndicator(strokeWidth: 1.5, color: theme.textMuted));
              if (us.updateAvailable) {
                return Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                  decoration: BoxDecoration(color: theme.success.withAlpha(25), borderRadius: BorderRadius.circular(20)),
                  child: Text('v${us.latestUpdate!.version}', style: TextStyle(color: theme.success, fontSize: 10, fontWeight: FontWeight.bold)),
                );
              }
              return const SizedBox.shrink();
            }),
          ]),
          const SizedBox(height: 14),
          Consumer<UpdateService>(
            builder: (_, us, _) {
              return GlassContainer(
                padding: const EdgeInsets.all(18),
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
                    const SizedBox(width: 14),
                    Expanded(child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text('Star Trail Companion', style: TextStyle(color: theme.textPrimary, fontSize: 14, fontWeight: FontWeight.w600)),
                        const SizedBox(height: 3),
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
                        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                        decoration: BoxDecoration(color: theme.success.withAlpha(20), borderRadius: BorderRadius.circular(20)),
                        child: Text('UPDATE', style: TextStyle(color: theme.success, fontSize: 10, fontWeight: FontWeight.bold, letterSpacing: 1)),
                      ),
                  ]),
                  if (us.error != null) ...[
                    const SizedBox(height: 10),
                    Text(us.error!, style: TextStyle(color: theme.error, fontSize: 11)),
                  ],
                  if (us.updateAvailable && us.latestUpdate!.changelog.isNotEmpty) ...[
                    const SizedBox(height: 14),
                    Container(
                      width: double.infinity,
                      padding: const EdgeInsets.all(12),
                      decoration: BoxDecoration(color: theme.card, borderRadius: BorderRadius.circular(10)),
                      child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
                        Text('What\'s New', style: TextStyle(color: theme.textSecondary, fontSize: 11, fontWeight: FontWeight.w600, letterSpacing: 0.5)),
                        const SizedBox(height: 6),
                        Text(us.latestUpdate!.changelog, style: TextStyle(color: theme.textMuted, fontSize: 11, height: 1.5)),
                      ]),
                    ),
                  ],
                  const SizedBox(height: 14),
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
                    const SizedBox(height: 8),
                    Text('Downloading... ${(us.downloadProgress * 100).toInt()}%', style: TextStyle(color: theme.textMuted, fontSize: 11)),
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
          const SizedBox(height: 40),
        ],
      ),
    );
  }
}
