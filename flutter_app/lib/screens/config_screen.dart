import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../theme/app_theme.dart';
import '../widgets/glass_container.dart';

class ConfigScreen extends StatefulWidget {
  const ConfigScreen({super.key});
  @override
  State<ConfigScreen> createState() => _ConfigScreenState();
}

class _ConfigScreenState extends State<ConfigScreen> {
  final List<Map<String, String>> _bleDevices = [];
  bool _bleScanning = false;
  final TextEditingController _wifiController = TextEditingController();
  bool _wifiConnecting = false;
  String? _wifiError;
  bool _wifiConnected = false;

  @override
  void dispose() {
    _wifiController.dispose();
    super.dispose();
  }

  Future<void> _scanBLE(DeviceService svc) async {
    setState(() { _bleScanning = true; _bleDevices.clear(); });
    try {
      var results = await svc.scanBLE();
      for (var r in results) {
        _bleDevices.add({
          'id': r.device.remoteId.toString(),
          'name': r.device.platformName.isNotEmpty ? r.device.platformName : r.device.remoteId.toString(),
        });
      }
    } catch (_) {}
    if (mounted) setState(() => _bleScanning = false);
  }

  Future<void> _connectWiFi(DeviceService svc) async {
    var host = _wifiController.text.trim();
    if (host.isEmpty) return;
    setState(() { _wifiConnecting = true; _wifiError = null; });
    var ok = await svc.connectWiFi(host);
    if (mounted) setState(() { _wifiConnecting = false; if (ok) { _wifiConnected = true; _wifiError = null; } else { _wifiError = 'Connection failed'; } });
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<ThemeProvider>(
      builder: (_, tp, _) {
        final theme = tp.theme;
        return Scaffold(
          backgroundColor: theme.surface,
          appBar: AppBar(
            title: Text('SETTINGS', style: TextStyle(fontSize: 13, letterSpacing: 3, fontWeight: FontWeight.w300, color: theme.textMuted)),
            leading: IconButton(icon: Icon(Icons.arrow_back), onPressed: () => Navigator.pop(context)),
          ),
          body: SingleChildScrollView(
            padding: EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _sectionHeader(theme, 'Connection'),
                SizedBox(height: 10),
                _buildConnectionPanel(theme),
                SizedBox(height: 28),
                _sectionHeader(theme, 'Theme'),
                SizedBox(height: 10),
                _buildThemeSelector(theme, tp),
                SizedBox(height: 28),
                _sectionHeader(theme, 'About'),
                SizedBox(height: 10),
                _buildAbout(theme),
                SizedBox(height: 24),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _sectionHeader(AppTheme theme, String title) {
    return Text(title.toUpperCase(), style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600));
  }

  Widget _buildThemeSelector(AppTheme theme, ThemeProvider tp) {
    return GlassContainer(
      padding: EdgeInsets.all(8),
      borderRadius: BorderRadius.circular(16),
      child: Row(
        children: AppThemeMode.values.map((mode) {
          final t = AppTheme.fromMode(mode);
          final selected = tp.mode == mode;
          return Expanded(
            child: GestureDetector(
              onTap: () => tp.setTheme(mode),
              child: AnimatedContainer(
                duration: Duration(milliseconds: 300),
                margin: EdgeInsets.symmetric(horizontal: 4),
                padding: EdgeInsets.symmetric(vertical: 16),
                decoration: BoxDecoration(
                  color: selected ? t.primary.withAlpha(15) : Colors.transparent,
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(color: selected ? t.primary.withAlpha(60) : t.cardBorder, width: selected ? 1.5 : 1),
                ),
                child: Column(
                  children: [
                    Container(width: 28, height: 28, decoration: BoxDecoration(shape: BoxShape.circle, gradient: LinearGradient(colors: [t.primary, t.accent]))),
                    SizedBox(height: 8),
                    Text(t.name, style: TextStyle(color: selected ? t.textPrimary : t.textMuted, fontSize: 11, fontWeight: selected ? FontWeight.w600 : FontWeight.w400, letterSpacing: 0.3)),
                  ],
                ),
              ),
            ),
          );
        }).toList(),
      ),
    );
  }

  Widget _buildAbout(AppTheme theme) {
    return GlassContainer(
      padding: EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text('Star Trail Cluster Controller', style: TextStyle(color: theme.textPrimary, fontSize: 14, fontWeight: FontWeight.w600, letterSpacing: 0.3)),
          SizedBox(height: 4),
          Text('v1.0.0', style: TextStyle(color: theme.textMuted, fontSize: 11, letterSpacing: 0.5)),
          SizedBox(height: 10),
          Text(
            'Premium telemetry dashboard for your Star Trail cluster. Monitor altitude, heading, temperature, and vehicle dynamics in real-time.',
            style: TextStyle(color: theme.textSecondary, fontSize: 12, letterSpacing: 0.2, height: 1.5),
          ),
          SizedBox(height: 12),
          _infoRow(theme, 'Platform', 'ESP32-S3'),
          _infoRow(theme, 'Protocol', 'BLE 5.0 / WiFi'),
          _infoRow(theme, 'Display', '1.28" 240×240 IPS'),
        ],
      ),
    );
  }

  Widget _infoRow(AppTheme theme, String label, String value) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 4),
      child: Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [
        Text(label, style: TextStyle(color: theme.textMuted, fontSize: 12)),
        Text(value, style: TextStyle(color: theme.textSecondary, fontSize: 12, fontFamily: 'monospace')),
      ]),
    );
  }

  Widget _buildConnectionPanel(AppTheme theme) {
    var svc = context.read<DeviceService>();
    return GlassContainer(
      padding: EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // BLE Section
          Row(children: [
            Icon(Icons.bluetooth, color: theme.primary, size: 20),
            SizedBox(width: 10),
            Text('BLE', style: TextStyle(color: theme.textPrimary, fontSize: 13, letterSpacing: 0.5)),
            Spacer(),
            if (_bleScanning)
              SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2, color: theme.primary))
            else
              TextButton(onPressed: () => _scanBLE(svc), child: Text('Scan', style: TextStyle(color: theme.primary, letterSpacing: 0.5))),
          ]),
          if (_bleDevices.isNotEmpty) ...[
            SizedBox(height: 8),
            ...List.generate(_bleDevices.length, (i) {
              var d = _bleDevices[i];
              return GestureDetector(
                onTap: () async {
                  var ok = await svc.connectBLE(d['id']!);
                  if (ok && context.mounted) {
                    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Connected to ${d['name']}'), backgroundColor: theme.success));
                  }
                },
                child: Container(
                  padding: EdgeInsets.symmetric(horizontal: 12, vertical: 10),
                  margin: EdgeInsets.only(bottom: 4),
                  decoration: BoxDecoration(color: theme.surface, borderRadius: BorderRadius.circular(10), border: Border.all(color: theme.cardBorder)),
                  child: Row(children: [
                    Icon(Icons.bluetooth_connected, color: theme.primary, size: 14),
                    SizedBox(width: 8),
                    Expanded(child: Text(d['name']!, style: TextStyle(color: theme.textPrimary, fontSize: 11))),
                    Icon(Icons.arrow_forward_ios, color: theme.textMuted, size: 10),
                  ]),
                ),
              );
            }),
          ],
          SizedBox(height: 16),
          Divider(color: theme.cardBorder, height: 1),
          SizedBox(height: 16),
          // WiFi Section
          Row(children: [
            Icon(Icons.wifi, color: _wifiConnected ? theme.success : theme.textMuted, size: 20),
            SizedBox(width: 10),
            Expanded(
              child: TextField(
                controller: _wifiController,
                style: TextStyle(color: theme.textPrimary, fontSize: 13),
                decoration: InputDecoration(
                  hintText: 'Device IP address...',
                  hintStyle: TextStyle(color: theme.textMuted.withAlpha(120), fontSize: 12),
                  border: InputBorder.none,
                  isDense: true,
                  filled: true,
                  fillColor: theme.surface,
                  contentPadding: EdgeInsets.symmetric(horizontal: 12, vertical: 10),
                ),
              ),
            ),
            if (_wifiConnecting)
              SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2, color: theme.primary))
            else
              TextButton(
                onPressed: _wifiConnected ? null : () => _connectWiFi(svc),
                child: Text(_wifiConnected ? 'Connected' : 'Connect', style: TextStyle(color: _wifiConnected ? theme.success : theme.primary, letterSpacing: 0.5)),
              ),
          ]),
          if (_wifiError != null) Padding(padding: EdgeInsets.only(left: 28, top: 4), child: Text(_wifiError!, style: TextStyle(color: theme.error, fontSize: 10))),
          SizedBox(height: 16),
          Divider(color: theme.cardBorder, height: 1),
          SizedBox(height: 12),
          // Simulator
          Consumer<DeviceService>(builder: (_, svc, _) => Row(children: [
            Icon(Icons.smartphone, color: theme.warning, size: 20),
            SizedBox(width: 10),
            Text('Simulator', style: TextStyle(color: theme.textPrimary, fontSize: 13, letterSpacing: 0.5)),
            Spacer(),
            GlassButton(
              onTap: svc.mode == ConnectionMode.simulator ? null : () { svc.startSimulator(); Navigator.pushReplacementNamed(context, '/home'); },
              child: Text(
                svc.mode == ConnectionMode.simulator ? 'Active' : 'Start',
                style: TextStyle(color: svc.mode == ConnectionMode.simulator ? theme.success : theme.warning, fontSize: 12, fontWeight: FontWeight.w600, letterSpacing: 1),
              ),
            ),
          ])),
        ],
      ),
    );
  }
}
