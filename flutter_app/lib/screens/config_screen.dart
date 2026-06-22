import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/theme_provider.dart';
import '../services/device_service.dart';
import '../theme/app_theme.dart';

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
    setState(() {
      _bleScanning = true;
      _bleDevices.clear();
    });
    try {
      var results = await svc.scanBLE();
      for (var r in results) {
        _bleDevices.add({
          'id': r.device.remoteId.toString(),
          'name': r.device.platformName.isNotEmpty
              ? r.device.platformName
              : r.device.remoteId.toString(),
        });
      }
    } catch (_) {}
    if (mounted) setState(() => _bleScanning = false);
  }

  Future<void> _connectWiFi(DeviceService svc) async {
    var host = _wifiController.text.trim();
    if (host.isEmpty) return;
    setState(() {
      _wifiConnecting = true;
      _wifiError = null;
    });
    var ok = await svc.connectWiFi(host);
    if (mounted) {
      setState(() {
        _wifiConnecting = false;
        if (ok) {
          _wifiConnected = true;
          _wifiError = null;
        } else {
          _wifiError = 'Connection failed';
        }
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<ThemeProvider>(
      builder: (_, tp, _) {
        final theme = tp.theme;
        return Scaffold(
          backgroundColor: theme.surface,
          appBar: AppBar(
            title: const Text('Settings'),
            leading: IconButton(
              icon: const Icon(Icons.arrow_back),
              onPressed: () => Navigator.pop(context),
            ),
          ),
          body: SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _sectionHeader(theme, 'Connection'),
                const SizedBox(height: 8),
                _buildConnectionPanel(theme),
                const SizedBox(height: 24),
                _sectionHeader(theme, 'Theme'),
                const SizedBox(height: 8),
                _buildThemeSelector(theme, tp),
                const SizedBox(height: 24),
                _sectionHeader(theme, 'App Info'),
                const SizedBox(height: 8),
                _buildAppInfo(theme),
                const SizedBox(height: 24),
                _sectionHeader(theme, 'About'),
                const SizedBox(height: 8),
                _buildAbout(theme),
                const SizedBox(height: 24),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _sectionHeader(AppTheme theme, String title) {
    return Text(
      title.toUpperCase(),
      style: TextStyle(
        color: theme.textMuted,
        fontSize: 10,
        letterSpacing: 2,
        fontWeight: FontWeight.w600,
      ),
    );
  }

  Widget _buildThemeSelector(AppTheme theme, ThemeProvider tp) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: theme.card,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: theme.cardBorder),
      ),
      child: Row(
        children: AppThemeMode.values.map((mode) {
          final t = AppTheme.fromMode(mode);
          final selected = tp.mode == mode;
          return Expanded(
            child: GestureDetector(
              onTap: () => tp.setTheme(mode),
              child: Container(
                margin: const EdgeInsets.symmetric(horizontal: 4),
                padding: const EdgeInsets.symmetric(vertical: 14),
                decoration: BoxDecoration(
                  color: selected ? t.primary.withAlpha(15) : Colors.transparent,
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(
                    color: selected ? t.primary : t.cardBorder,
                    width: selected ? 2 : 1,
                  ),
                ),
                child: Column(
                  children: [
                    Container(
                      width: 28,
                      height: 28,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        gradient: LinearGradient(
                          colors: [t.primary, t.accent],
                        ),
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      t.name,
                      style: TextStyle(
                        color: selected ? t.textPrimary : t.textMuted,
                        fontSize: 11,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          );
        }).toList(),
      ),
    );
  }

  Widget _buildAppInfo(AppTheme theme) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: theme.card,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: theme.cardBorder),
      ),
      child: Column(
        children: [
          _infoRow(theme, 'Version', '1.0.0'),
          const SizedBox(height: 8),
          _infoRow(theme, 'Build', '2024.1'),
          const SizedBox(height: 8),
          _infoRow(theme, 'Platform', 'ESP32'),
          const SizedBox(height: 8),
          _infoRow(theme, 'Protocol', 'BLE 4.0 / WiFi'),
        ],
      ),
    );
  }

  Widget _infoRow(AppTheme theme, String label, String value) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Text(
          label,
          style: TextStyle(color: theme.textMuted, fontSize: 12),
        ),
        Text(
          value,
          style: TextStyle(
            color: theme.textSecondary,
            fontSize: 12,
            fontFamily: 'monospace',
          ),
        ),
      ],
    );
  }

  Widget _buildAbout(AppTheme theme) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: theme.card,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: theme.cardBorder),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Star Trail Cluster Controller',
            style: TextStyle(
              color: theme.textPrimary,
              fontSize: 14,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 4),
          Text(
            'v1.0.0',
            style: TextStyle(color: theme.textMuted, fontSize: 11),
          ),
          const SizedBox(height: 8),
          Text(
            'Premium telemetry dashboard for your Star Trail '
            'cluster. Monitor altitude, heading, temperature, '
            'and vehicle dynamics in real-time.',
            style: TextStyle(
              color: theme.textSecondary,
              fontSize: 12,
              height: 1.4,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildConnectionPanel(AppTheme theme) {
    var svc = Provider.of<DeviceService>(context, listen: false);
    return Container(
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: theme.card,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: theme.cardBorder),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.bluetooth, color: theme.primary, size: 20),
              const SizedBox(width: 8),
              Text(
                'BLE',
                style: TextStyle(color: theme.textPrimary, fontSize: 13),
              ),
              const Spacer(),
              if (_bleScanning)
                SizedBox(
                  width: 14,
                  height: 14,
                  child: CircularProgressIndicator(
                    strokeWidth: 2,
                    color: theme.primary,
                  ),
                )
              else
                TextButton(
                  onPressed: () => _scanBLE(svc),
                  child: Text(
                    'Scan',
                    style: TextStyle(color: theme.primary),
                  ),
                ),
            ],
          ),
          if (_bleDevices.isNotEmpty) ...[
            const SizedBox(height: 8),
            ...List.generate(_bleDevices.length, (i) {
              var d = _bleDevices[i];
              return GestureDetector(
                onTap: () async {
                  var ok = await svc.connectBLE(d['id']!);
                  if (ok && context.mounted) {
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(
                        content: Text('Connected to ${d['name']}'),
                        backgroundColor: theme.success,
                      ),
                    );
                  }
                },
                child: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
                  margin: const EdgeInsets.only(bottom: 4),
                  decoration: BoxDecoration(
                    color: theme.surface,
                    borderRadius: BorderRadius.circular(8),
                    border: Border.all(color: theme.cardBorder),
                  ),
                  child: Row(
                    children: [
                      Icon(Icons.bluetooth_connected, color: theme.primary, size: 14),
                      const SizedBox(width: 8),
                      Expanded(
                        child: Text(
                          d['name']!,
                          style: TextStyle(color: theme.textPrimary, fontSize: 11),
                        ),
                      ),
                      Icon(
                        Icons.arrow_forward_ios,
                        color: theme.textMuted,
                        size: 10,
                      ),
                    ],
                  ),
                ),
              );
            }),
          ],
          const SizedBox(height: 14),
          Row(
            children: [
              Icon(
                Icons.wifi,
                color: _wifiConnected ? theme.success : theme.textMuted,
                size: 20,
              ),
              const SizedBox(width: 8),
              Expanded(
                child: TextField(
                  controller: _wifiController,
                  style: TextStyle(color: theme.textPrimary, fontSize: 12),
                  decoration: InputDecoration(
                    hintText: 'Device IP address...',
                    hintStyle: TextStyle(
                      color: theme.textMuted.withAlpha(150),
                      fontSize: 12,
                    ),
                    border: InputBorder.none,
                    isDense: true,
                    filled: true,
                    fillColor: theme.surface,
                    contentPadding: const EdgeInsets.symmetric(
                      horizontal: 10,
                      vertical: 8,
                    ),
                  ),
                ),
              ),
              if (_wifiConnecting)
                SizedBox(
                  width: 14,
                  height: 14,
                  child: CircularProgressIndicator(
                    strokeWidth: 2,
                    color: theme.primary,
                  ),
                )
              else
                TextButton(
                  onPressed: _wifiConnected ? null : () => _connectWiFi(svc),
                  child: Text(
                    _wifiConnected ? 'Connected' : 'Connect',
                    style: TextStyle(
                      color: _wifiConnected ? theme.success : theme.primary,
                    ),
                  ),
                ),
            ],
          ),
          if (_wifiError != null)
            Padding(
              padding: const EdgeInsets.only(left: 28, top: 4),
              child: Text(
                _wifiError!,
                style: TextStyle(color: theme.error, fontSize: 10),
              ),
            ),
          const SizedBox(height: 14),
          Divider(color: theme.cardBorder),
          const SizedBox(height: 8),
          Consumer<DeviceService>(
            builder: (_, svc, _) => Column(
              children: [
                Row(
                  children: [
                    Icon(Icons.smartphone, color: theme.warning, size: 20),
                    const SizedBox(width: 8),
                    Text(
                      'Simulator',
                      style: TextStyle(color: theme.textPrimary, fontSize: 13),
                    ),
                    const Spacer(),
                    ElevatedButton.icon(
                      onPressed: svc.mode == ConnectionMode.simulator
                          ? null
                          : () {
                              svc.startSimulator();
                              Navigator.pushReplacementNamed(context, '/home');
                            },
                      icon: const Icon(Icons.play_arrow, size: 16),
                      label: Text(
                        svc.mode == ConnectionMode.simulator ? 'Active' : 'Start',
                      ),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: svc.mode == ConnectionMode.simulator
                            ? theme.success.withAlpha(40)
                            : theme.warning,
                        foregroundColor: svc.mode == ConnectionMode.simulator
                            ? theme.success
                            : Colors.white,
                        padding: const EdgeInsets.symmetric(
                          horizontal: 14,
                          vertical: 8,
                        ),
                        minimumSize: Size.zero,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(8),
                        ),
                        elevation: 0,
                      ),
                    ),
                  ],
                ),
                if (svc.mode == ConnectionMode.simulator)
                  Padding(
                    padding: const EdgeInsets.only(left: 28, top: 4),
                    child: Text(
                      'Simulator is running',
                      style: TextStyle(color: theme.success, fontSize: 10),
                    ),
                  ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
