import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/theme_provider.dart';
import '../../services/device_service.dart';
import '../../theme/app_theme.dart';

const _timeoutOptions = ['15s', '30s', '1m', '2m', '5m'];
const _timeoutValues = [15, 30, 60, 120, 300];

class SystemScreen extends StatefulWidget {
  const SystemScreen({super.key});
  @override
  State<SystemScreen> createState() => _SystemScreenState();
}

class _SystemScreenState extends State<SystemScreen> {
  double _brightness = 100;
  int _timeoutIndex = 2;
  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();
  bool _wifiConnecting = false;
  String? _wifiStatus;
  bool _sending = false;

  @override
  void dispose() {
    _ssidController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _connectWiFi() async {
    final host = _ssidController.text.trim();
    if (host.isEmpty) {
      setState(() => _wifiStatus = 'Enter an IP address');
      return;
    }
    setState(() {
      _wifiConnecting = true;
      _wifiStatus = null;
    });
    final svc = context.read<DeviceService>();
    final ok = await svc.connectWiFi(host);
    if (mounted) {
      setState(() {
        _wifiConnecting = false;
        _wifiStatus = ok ? 'Connected' : 'Connection failed';
      });
    }
  }

  Future<void> _reboot() async {
    final svc = context.read<DeviceService>();
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) {
        final t = context.read<ThemeProvider>().theme;
        return AlertDialog(
          backgroundColor: t.card,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(20),
          ),
          title: const Text('Reboot Device'),
          content: const Text(
            'Are you sure you want to reboot the device?',
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () => Navigator.pop(ctx, true),
              style: ElevatedButton.styleFrom(
                backgroundColor: t.error,
                foregroundColor: Colors.white,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(10),
                ),
              ),
              child: const Text('Reboot'),
            ),
          ],
        );
      },
    );
    if (confirmed == true) {
      await svc.sendCommand('reboot');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Text('Reboot command sent'),
            backgroundColor: context.read<ThemeProvider>().theme.warning,
          ),
        );
      }
    }
  }

  Future<void> _apply() async {
    setState(() => _sending = true);
    final svc = context.read<DeviceService>();
    final cmds = [
      'brightness=${_brightness.toInt()}',
      'timeout=${_timeoutValues[_timeoutIndex]}',
    ];
    for (final cmd in cmds) {
      await svc.sendCommand(cmd);
    }
    if (mounted) {
      setState(() => _sending = false);
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: const Text('System settings applied'),
          backgroundColor: context.read<ThemeProvider>().theme.success,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      appBar: AppBar(
        title: const Text('System Settings'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => Navigator.pop(context),
        ),
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _sectionLabel(theme, 'DISPLAY BRIGHTNESS'),
          const SizedBox(height: 8),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: theme.cardBorder),
            ),
            child: Row(
              children: [
                Icon(Icons.brightness_low, color: theme.textMuted, size: 18),
                Expanded(
                  child: Slider(
                    value: _brightness,
                    min: 0,
                    max: 100,
                    divisions: 100,
                    onChanged: (v) => setState(() => _brightness = v),
                  ),
                ),
                Icon(Icons.brightness_high, color: theme.textMuted, size: 18),
                SizedBox(
                  width: 36,
                  child: Text(
                    '${_brightness.toInt()}%',
                    textAlign: TextAlign.right,
                    style: TextStyle(
                      color: theme.primary,
                      fontSize: 12,
                      fontWeight: FontWeight.w600,
                      fontFamily: 'monospace',
                    ),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 24),
          _sectionLabel(theme, 'SCREEN TIMEOUT'),
          const SizedBox(height: 8),
          Row(
            children: List.generate(_timeoutOptions.length, (i) {
              final selected = _timeoutIndex == i;
              return Expanded(
                child: GestureDetector(
                  onTap: () => setState(() => _timeoutIndex = i),
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 250),
                    margin: const EdgeInsets.symmetric(horizontal: 4),
                    padding: const EdgeInsets.symmetric(
                      vertical: 14,
                    ),
                    decoration: BoxDecoration(
                      color: selected
                          ? theme.primary.withAlpha(25)
                          : theme.card,
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(
                        color: selected ? theme.primary : theme.cardBorder,
                        width: selected ? 2 : 1,
                      ),
                    ),
                    child: Column(
                      children: [
                        Icon(
                          _timeoutIcon(i),
                          color: selected
                              ? theme.primary
                              : theme.textMuted,
                          size: 22,
                        ),
                        const SizedBox(height: 6),
                        Text(
                          _timeoutOptions[i],
                          style: TextStyle(
                            color: selected
                                ? theme.primary
                                : theme.textSecondary,
                            fontSize: 11,
                            fontWeight: selected
                                ? FontWeight.w600
                                : FontWeight.w400,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              );
            }),
          ),
          const SizedBox(height: 24),
          _sectionLabel(theme, 'WIFI SETTINGS'),
          const SizedBox(height: 8),
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: theme.card,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: theme.cardBorder),
            ),
            child: Column(
              children: [
                TextField(
                  controller: _ssidController,
                  style: TextStyle(color: theme.textPrimary, fontSize: 14),
                  decoration: InputDecoration(
                    labelText: 'SSID / IP Address',
                    prefixIcon: Icon(Icons.wifi, color: theme.textMuted, size: 20),
                  ),
                ),
                const SizedBox(height: 12),
                TextField(
                  controller: _passwordController,
                  obscureText: true,
                  style: TextStyle(color: theme.textPrimary, fontSize: 14),
                  decoration: InputDecoration(
                    labelText: 'Password',
                    prefixIcon: Icon(Icons.lock, color: theme.textMuted, size: 20),
                  ),
                ),
                const SizedBox(height: 16),
                SizedBox(
                  width: double.infinity,
                  height: 44,
                  child: ElevatedButton.icon(
                    onPressed: _wifiConnecting ? null : _connectWiFi,
                    icon: _wifiConnecting
                        ? SizedBox(
                            width: 16,
                            height: 16,
                            child: CircularProgressIndicator(
                              strokeWidth: 2,
                              color: Colors.white,
                            ),
                          )
                        : const Icon(Icons.wifi, size: 18),
                    label: Text(
                      _wifiConnecting ? 'Connecting...' : 'Connect',
                    ),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: theme.primary,
                      foregroundColor: Colors.white,
                      elevation: 0,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                    ),
                  ),
                ),
                if (_wifiStatus != null) ...[
                  const SizedBox(height: 8),
                  Text(
                    _wifiStatus!,
                    style: TextStyle(
                      color: _wifiStatus == 'Connected'
                          ? theme.success
                          : theme.error,
                      fontSize: 12,
                    ),
                  ),
                ],
              ],
            ),
          ),
          const SizedBox(height: 24),
          _sectionLabel(theme, 'DEVICE INFO'),
          const SizedBox(height: 8),
          Consumer<DeviceService>(
            builder: (_, svc, _) {
              final d = svc.data;
              return Container(
                padding: const EdgeInsets.all(16),
                decoration: BoxDecoration(
                  color: theme.card,
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(color: theme.cardBorder),
                ),
                child: Column(
                  children: [
                    _infoRow(theme, 'Uptime', _formatUptime(d.uptime)),
                    _infoDivider(theme),
                    _infoRow(theme, 'Free Heap', '${(d.heap / 1024).floor()} KB'),
                    _infoDivider(theme),
                    _infoRow(theme, 'RSSI', '${d.rssi} dBm'),
                    _infoDivider(theme),
                    _infoRow(theme, 'IP Address', d.ip),
                    _infoDivider(theme),
                    _infoRow(theme, 'SSID', d.ssid),
                  ],
                ),
              );
            },
          ),
          const SizedBox(height: 24),
          _sectionLabel(theme, 'ACTIONS'),
          const SizedBox(height: 8),
          SizedBox(
            width: double.infinity,
            height: 52,
            child: ElevatedButton(
              onPressed: _sending ? null : _apply,
              style: ElevatedButton.styleFrom(
                backgroundColor: theme.primary,
                foregroundColor: Colors.white,
                elevation: 0,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(16),
                ),
              ),
              child: _sending
                  ? SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(
                        strokeWidth: 2,
                        color: Colors.white,
                      ),
                    )
                  : const Text(
                      'Apply Settings',
                      style: TextStyle(
                        fontSize: 15,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
            ),
          ),
          const SizedBox(height: 16),
          SizedBox(
            width: double.infinity,
            height: 52,
            child: ElevatedButton.icon(
              onPressed: _reboot,
              icon: const Icon(Icons.restart_alt, size: 20),
              label: const Text(
                'Reboot Device',
                style: TextStyle(
                  fontSize: 15,
                  fontWeight: FontWeight.w600,
                ),
              ),
              style: ElevatedButton.styleFrom(
                backgroundColor: theme.error.withAlpha(30),
                foregroundColor: theme.error,
                elevation: 0,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(16),
                  side: BorderSide(color: theme.error.withAlpha(80)),
                ),
              ),
            ),
          ),
          const SizedBox(height: 20),
        ],
      ),
    );
  }

  Widget _sectionLabel(AppTheme theme, String label) {
    return Text(
      label,
      style: TextStyle(
        color: theme.textMuted,
        fontSize: 10,
        letterSpacing: 2,
        fontWeight: FontWeight.w600,
      ),
    );
  }

  Widget _infoRow(AppTheme theme, String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(color: theme.textSecondary, fontSize: 13),
          ),
          Text(
            value,
            style: TextStyle(
              color: theme.primary,
              fontSize: 13,
              fontWeight: FontWeight.w500,
              fontFamily: 'monospace',
            ),
          ),
        ],
      ),
    );
  }

  Widget _infoDivider(AppTheme theme) {
    return Divider(color: theme.cardBorder, height: 1);
  }

  IconData _timeoutIcon(int i) {
    switch (i) {
      case 0: return Icons.timer_outlined;
      case 1: return Icons.timer_outlined;
      case 2: return Icons.timer;
      case 3: return Icons.timer;
      case 4: return Icons.timer_sharp;
      default: return Icons.timer;
    }
  }

  String _formatUptime(int seconds) {
    final h = seconds ~/ 3600;
    final m = (seconds % 3600) ~/ 60;
    final s = seconds % 60;
    if (h > 0) return '${h}h ${m}m ${s}s';
    if (m > 0) return '${m}m ${s}s';
    return '${s}s';
  }
}
