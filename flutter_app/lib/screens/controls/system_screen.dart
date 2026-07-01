import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/theme_provider.dart';
import '../../services/device_service.dart';
import '../../theme/app_theme.dart';
import '../../widgets/glass_container.dart';

const _timeoutOptions = ['15s', '30s', '1m', '2m', '5m'];
const _timeoutValues = [15, 30, 60, 120, 300];
const _timeoutIcons = [Icons.timer_outlined, Icons.timer_outlined, Icons.timer, Icons.timer, Icons.timer_sharp];

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
  void dispose() { _ssidController.dispose(); _passwordController.dispose(); super.dispose(); }

  Future<void> _connectWiFi() async {
    final host = _ssidController.text.trim();
    if (host.isEmpty) { setState(() => _wifiStatus = 'Enter an IP address'); return; }
    setState(() { _wifiConnecting = true; _wifiStatus = null; });
    final ok = await context.read<DeviceService>().connectWiFi(host);
    if (mounted) setState(() { _wifiConnecting = false; _wifiStatus = ok ? 'Connected' : 'Connection failed'; });
  }

  Future<void> _reboot() async {
    final svc = context.read<DeviceService>();
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) {
        final t = context.read<ThemeProvider>().theme;
        return AlertDialog(
          backgroundColor: t.card,
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(24)),
          title: Text('Reboot Device', style: TextStyle(color: t.textPrimary)),
          content: Text('Are you sure you want to reboot the device?', style: TextStyle(color: t.textSecondary)),
          actions: [
            TextButton(onPressed: () => Navigator.pop(ctx, false), child: Text('Cancel')),
            ElevatedButton(
              onPressed: () => Navigator.pop(ctx, true),
              style: ElevatedButton.styleFrom(backgroundColor: t.error, foregroundColor: Colors.white, shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12))),
              child: Text('Reboot'),
            ),
          ],
        );
      },
    );
    if (confirmed == true) {
      await svc.sendCommand('reboot');
      if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Reboot command sent'), backgroundColor: context.read<ThemeProvider>().theme.warning));
    }
  }

  Future<void> _apply() async {
    setState(() => _sending = true);
    final svc = context.read<DeviceService>();
    for (final cmd in ['brightness=${_brightness.toInt()}', 'timeout=${_timeoutValues[_timeoutIndex]}']) {
      await svc.sendCommand(cmd);
    }
    if (mounted) {
      setState(() => _sending = false);
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('System settings applied'), backgroundColor: context.read<ThemeProvider>().theme.success));
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      backgroundColor: theme.surface,
      appBar: AppBar(
        title: Text('SYSTEM', style: TextStyle(fontSize: 13, letterSpacing: 3, fontWeight: FontWeight.w300, color: theme.textMuted)),
        leading: IconButton(icon: Icon(Icons.arrow_back), onPressed: () => Navigator.pop(context)),
      ),
      body: ListView(
        padding: EdgeInsets.all(16),
        children: [
          _sectionLabel(theme, 'DISPLAY BRIGHTNESS'),
          SizedBox(height: 10),
          GlassContainer(
            padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            child: Row(children: [
              Icon(Icons.brightness_low, color: theme.textMuted, size: 18),
              Expanded(child: Slider(value: _brightness, min: 0, max: 100, divisions: 100, onChanged: (v) => setState(() => _brightness = v))),
              Icon(Icons.brightness_high, color: theme.textMuted, size: 18),
              SizedBox(width: 36, child: Text('${_brightness.toInt()}%', textAlign: TextAlign.right, style: TextStyle(color: theme.primary, fontSize: 12, fontWeight: FontWeight.w600, fontFamily: 'monospace'))),
            ]),
          ),
          SizedBox(height: 24),
          _sectionLabel(theme, 'SCREEN TIMEOUT'),
          SizedBox(height: 10),
          Row(
            children: List.generate(_timeoutOptions.length, (i) {
              final selected = _timeoutIndex == i;
              return Expanded(
                child: GestureDetector(
                  onTap: () => setState(() => _timeoutIndex = i),
                  child: AnimatedContainer(
                    duration: Duration(milliseconds: 250),
                    margin: EdgeInsets.symmetric(horizontal: 4),
                    padding: EdgeInsets.symmetric(vertical: 14),
                    decoration: BoxDecoration(
                      color: selected ? theme.primary.withAlpha(25) : theme.card,
                      borderRadius: BorderRadius.circular(14),
                      border: Border.all(color: selected ? theme.primary : theme.cardBorder, width: selected ? 2 : 1),
                    ),
                    child: Column(children: [
                      Icon(_timeoutIcons[i], color: selected ? theme.primary : theme.textMuted, size: 22),
                      SizedBox(height: 6),
                      Text(_timeoutOptions[i], style: TextStyle(color: selected ? theme.primary : theme.textSecondary, fontSize: 11, fontWeight: selected ? FontWeight.w600 : FontWeight.w400)),
                    ]),
                  ),
                ),
              );
            }),
          ),
          SizedBox(height: 24),
          _sectionLabel(theme, 'WIFI SETTINGS'),
          SizedBox(height: 10),
          GlassContainer(
            padding: EdgeInsets.all(16),
            child: Column(children: [
              TextField(
                controller: _ssidController,
                style: TextStyle(color: theme.textPrimary, fontSize: 14),
                decoration: InputDecoration(labelText: 'Device IP', prefixIcon: Icon(Icons.wifi, color: theme.textMuted, size: 20)),
              ),
              SizedBox(height: 12),
              TextField(
                controller: _passwordController,
                obscureText: true,
                style: TextStyle(color: theme.textPrimary, fontSize: 14),
                decoration: InputDecoration(labelText: 'Password', prefixIcon: Icon(Icons.lock, color: theme.textMuted, size: 20)),
              ),
              SizedBox(height: 16),
              GlassButton(
                width: double.infinity,
                onTap: _wifiConnecting ? null : _connectWiFi,
                child: _wifiConnecting
                  ? SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                  : Text('CONNECT', style: TextStyle(color: theme.primary, fontSize: 13, fontWeight: FontWeight.w600, letterSpacing: 1.5)),
              ),
              if (_wifiStatus != null) ...[
                SizedBox(height: 8),
                Text(_wifiStatus!, style: TextStyle(color: _wifiStatus == 'Connected' ? theme.success : theme.error, fontSize: 12)),
              ],
            ]),
          ),
          SizedBox(height: 24),
          _sectionLabel(theme, 'DEVICE INFO'),
          SizedBox(height: 10),
          Consumer<DeviceService>(builder: (_, svc, _) {
            final d = svc.data;
            return GlassContainer(
              padding: EdgeInsets.all(16),
              child: Column(children: [
                _infoRow(theme, 'Uptime', _formatUptime(d.uptime)),
                _divider(theme),
                _infoRow(theme, 'Free Heap', '${(d.heap / 1024).floor()} KB'),
                _divider(theme),
                _infoRow(theme, 'RSSI', '${d.rssi} dBm'),
                _divider(theme),
                _infoRow(theme, 'IP Address', d.ip),
                _divider(theme),
                _infoRow(theme, 'SSID', d.ssid),
              ]),
            );
          }),
          SizedBox(height: 24),
          GlassButton(
            width: double.infinity,
            height: 52,
            onTap: _sending ? null : _apply,
            child: _sending
              ? SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: theme.primary))
              : Text('APPLY SETTINGS', style: TextStyle(color: theme.primary, fontSize: 14, fontWeight: FontWeight.w600, letterSpacing: 2)),
          ),
          SizedBox(height: 16),
          GestureDetector(
            onTap: _reboot,
            child: GlassContainer(
              width: double.infinity,
              padding: EdgeInsets.symmetric(vertical: 16),
              borderColor: theme.error.withAlpha(40),
              bgColor: theme.error.withAlpha(8),
              child: Row(mainAxisAlignment: MainAxisAlignment.center, children: [
                Icon(Icons.restart_alt, color: theme.error, size: 20),
                SizedBox(width: 8),
                Text('REBOOT DEVICE', style: TextStyle(color: theme.error, fontSize: 14, fontWeight: FontWeight.w600, letterSpacing: 1.5)),
              ]),
            ),
          ),
          SizedBox(height: 20),
        ],
      ),
    );
  }

  Widget _infoRow(AppTheme theme, String label, String value) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 8),
      child: Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [
        Text(label, style: TextStyle(color: theme.textSecondary, fontSize: 13, letterSpacing: 0.3)),
        Text(value, style: TextStyle(color: theme.primary, fontSize: 13, fontWeight: FontWeight.w500, fontFamily: 'monospace')),
      ]),
    );
  }

  Widget _divider(AppTheme theme) => Divider(color: theme.cardBorder, height: 1);

  Widget _sectionLabel(AppTheme theme, String label) {
    return Text(label, style: TextStyle(color: theme.textMuted, fontSize: 10, letterSpacing: 2, fontWeight: FontWeight.w600));
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
