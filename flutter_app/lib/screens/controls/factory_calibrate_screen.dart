import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/theme_provider.dart';
import '../../services/device_service.dart';
import '../../widgets/glass_container.dart';

/// Guided one-time FACTORY calibration wizard — the installer runs this once
/// per unit at fitment; customers never see or need it again. Two steps:
///   1) Orientation zero — car parked level, captures the 25-30deg mount tilt.
///   2) Figure-8 magnetometer calibration — corrects hard/soft-iron effects.
/// Both commands are defined in BenzCluster/commands.cpp and persisted to
/// SPIFFS by the firmware, so this only needs to run once, ever, per device.
class FactoryCalibrateScreen extends StatefulWidget {
  const FactoryCalibrateScreen({super.key});
  @override
  State<FactoryCalibrateScreen> createState() => _FactoryCalibrateScreenState();
}

enum _Step { intro, zeroing, zeroed, magcalRunning, done, failed }

class _FactoryCalibrateScreenState extends State<FactoryCalibrateScreen> {
  _Step _step = _Step.intro;
  String? _debugLine;
  double _magProgress = 0;
  Timer? _magTimer;
  static const _magSeconds = 20;

  @override
  void dispose() {
    _magTimer?.cancel();
    super.dispose();
  }

  Future<void> _runZero() async {
    setState(() => _step = _Step.zeroing);
    final reply = await context
        .read<DeviceService>()
        .sendCommandWithReply('factory_zero', timeout: const Duration(seconds: 5));
    if (!mounted) return;
    setState(() => _step = reply != null ? _Step.zeroed : _Step.failed);
  }

  Future<void> _runMagCal() async {
    setState(() { _step = _Step.magcalRunning; _magProgress = 0; });
    final svc = context.read<DeviceService>();
    // Local progress bar ticks in lockstep with the firmware's blocking
    // figure-8 sample window so the user has something to watch.
    _magTimer = Timer.periodic(const Duration(milliseconds: 200), (t) {
      if (!mounted) { t.cancel(); return; }
      setState(() => _magProgress = (_magProgress + 0.2 / _magSeconds).clamp(0, 0.98));
    });
    final reply = await svc.sendCommandWithReply(
      'factory_magcal=$_magSeconds',
      timeout: Duration(seconds: _magSeconds + 8),
    );
    _magTimer?.cancel();
    if (!mounted) return;
    setState(() {
      _magProgress = 1.0;
      _step = reply != null ? _Step.done : _Step.failed;
    });
  }

  Future<void> _refreshDebug() async {
    final reply = await context.read<DeviceService>().sendCommandWithReply('debug_sensors');
    if (mounted) setState(() => _debugLine = reply ?? 'No reply — check connection');
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    final connected = context.watch<DeviceService>().isConnected;
    return Scaffold(
      appBar: AppBar(title: const Text('Factory Calibration')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          GlassContainer(
            padding: const EdgeInsets.all(16),
            child: Row(
              children: [
                Icon(Icons.build_circle_outlined, color: theme.warning, size: 22),
                const SizedBox(width: 10),
                Expanded(
                  child: Text(
                    'One-time setup — run this ONCE per unit at install. '
                    'Customers never need to calibrate.',
                    style: TextStyle(color: theme.textSecondary, fontSize: 12.5, height: 1.4),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 20),
          _StepCard(
            index: 1,
            title: 'Orientation Zero',
            body: 'Park the car on level ground. This captures the 25-30° '
                'enclosure mount angle so attitude and heading read true.',
            actionLabel: switch (_step) {
              _Step.intro => 'Start',
              _Step.zeroing => 'Zeroing…',
              _ => 'Re-run',
            },
            busy: _step == _Step.zeroing,
            done: _step.index >= _Step.zeroed.index,
            enabled: connected && _step != _Step.zeroing,
            onTap: _runZero,
            theme: theme,
          ),
          const SizedBox(height: 12),
          _StepCard(
            index: 2,
            title: 'Magnetometer Calibration',
            body: 'Rotate the whole device slowly through a figure-8 pattern '
                'in every orientation for $_magSeconds seconds, away from '
                'large metal objects.',
            actionLabel: _step == _Step.magcalRunning ? 'Rotating… keep going' : 'Start figure-8',
            busy: _step == _Step.magcalRunning,
            done: _step == _Step.done,
            enabled: connected &&
                _step.index >= _Step.zeroed.index &&
                _step != _Step.magcalRunning,
            progress: _step == _Step.magcalRunning ? _magProgress : null,
            onTap: _runMagCal,
            theme: theme,
          ),
          if (_step == _Step.done) ...[
            const SizedBox(height: 20),
            GlassContainer(
              borderColor: theme.success,
              padding: const EdgeInsets.all(16),
              child: Row(children: [
                Icon(Icons.check_circle, color: theme.success),
                const SizedBox(width: 10),
                Expanded(
                  child: Text('Factory calibration complete. This unit is ready to ship.',
                      style: TextStyle(color: theme.textPrimary, fontSize: 13, fontWeight: FontWeight.w600)),
                ),
              ]),
            ),
          ],
          if (_step == _Step.failed) ...[
            const SizedBox(height: 20),
            GlassContainer(
              borderColor: theme.error,
              padding: const EdgeInsets.all(16),
              child: Row(children: [
                Icon(Icons.error_outline, color: theme.error),
                const SizedBox(width: 10),
                Expanded(
                  child: Text('No response from device — check the connection and try again.',
                      style: TextStyle(color: theme.textPrimary, fontSize: 13)),
                ),
              ]),
            ),
          ],
          const SizedBox(height: 24),
          Divider(color: theme.cardBorder),
          const SizedBox(height: 12),
          Row(
            children: [
              Text('SENSOR DIAGNOSTICS', style: TextStyle(color: theme.textMuted, fontSize: 11, letterSpacing: 1)),
              const Spacer(),
              TextButton(
                onPressed: connected ? _refreshDebug : null,
                child: Text('Refresh', style: TextStyle(color: theme.primary, fontSize: 12)),
              ),
            ],
          ),
          const SizedBox(height: 4),
          GlassContainer(
            padding: const EdgeInsets.all(14),
            child: Text(
              _debugLine ?? 'Tap Refresh to read raw + fused sensor values.',
              style: TextStyle(color: theme.textSecondary, fontSize: 11, fontFamily: 'monospace'),
            ),
          ),
        ],
      ),
    );
  }
}

class _StepCard extends StatelessWidget {
  final int index;
  final String title;
  final String body;
  final String actionLabel;
  final bool busy;
  final bool done;
  final bool enabled;
  final double? progress;
  final VoidCallback onTap;
  final dynamic theme;
  const _StepCard({
    required this.index,
    required this.title,
    required this.body,
    required this.actionLabel,
    required this.busy,
    required this.done,
    required this.enabled,
    required this.onTap,
    required this.theme,
    this.progress,
  });

  @override
  Widget build(BuildContext context) {
    return GlassContainer(
      borderColor: done ? theme.success : theme.cardBorder,
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                width: 28,
                height: 28,
                alignment: Alignment.center,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: (done ? theme.success : theme.primary).withAlpha(30),
                ),
                child: done
                    ? Icon(Icons.check, color: theme.success, size: 16)
                    : Text('$index', style: TextStyle(color: theme.primary, fontWeight: FontWeight.w700)),
              ),
              const SizedBox(width: 10),
              Text(title, style: TextStyle(color: theme.textPrimary, fontSize: 15, fontWeight: FontWeight.w600)),
            ],
          ),
          const SizedBox(height: 8),
          Text(body, style: TextStyle(color: theme.textSecondary, fontSize: 12.5, height: 1.4)),
          if (progress != null) ...[
            const SizedBox(height: 12),
            ClipRRect(
              borderRadius: BorderRadius.circular(4),
              child: LinearProgressIndicator(
                value: progress,
                minHeight: 6,
                backgroundColor: theme.cardBorder,
                valueColor: AlwaysStoppedAnimation(theme.primary),
              ),
            ),
          ],
          const SizedBox(height: 12),
          SizedBox(
            width: double.infinity,
            child: ElevatedButton(
              onPressed: enabled ? onTap : null,
              style: ElevatedButton.styleFrom(
                backgroundColor: theme.primary,
                foregroundColor: theme.onPrimary,
                disabledBackgroundColor: theme.cardBorder,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                padding: const EdgeInsets.symmetric(vertical: 12),
              ),
              child: busy
                  ? SizedBox(
                      width: 18, height: 18,
                      child: CircularProgressIndicator(strokeWidth: 2, color: theme.onPrimary),
                    )
                  : Text(actionLabel, style: const TextStyle(fontWeight: FontWeight.w600)),
            ),
          ),
        ],
      ),
    );
  }
}
