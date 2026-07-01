import 'dart:async';
import 'dart:convert';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:http/http.dart' as http;
import '../models/device_data.dart';
import '../models/widget_layout.dart';

enum ConnectionMode { none, ble, wifi, simulator }

/// Result of a layout push, so the UI can report which transport was used.
enum PushResult { ok, okViaWifi, failed, notConnected }

// Custom-layout GATT (must match BenzCluster/ble_notify.cpp).
const String kLayoutServiceUuid = '00002222-3333-4444-5555-666677778888';
const String kLayoutCharUuid = '00002222-3333-4444-5555-666677778889';

class DeviceService extends ChangeNotifier {
  DeviceData _data = DeviceData();
  DeviceData get data => _data;
  ConnectionMode mode = ConnectionMode.none;
  bool get isConnected => mode != ConnectionMode.none;

  // BLE
  BluetoothDevice? _bleDevice;
  BluetoothCharacteristic? _dataChar;
  BluetoothCharacteristic? _layoutChar;

  // WiFi
  String? _wifiHost;
  Timer? _pollTimer;

  // Simulator
  Timer? _simTimer;
  double _simHeading = 0;
  double _simPitch = 0;
  double _simRoll = 0;

  @override
  void dispose() {
    _pollTimer?.cancel();
    _simTimer?.cancel();
    super.dispose();
  }

  // --- BLE ---
  Future<List<ScanResult>> scanBLE() async {
    await FlutterBluePlus.startScan(timeout: Duration(seconds: 10));
    await Future.delayed(Duration(seconds: 5));
    await FlutterBluePlus.stopScan();
    return FlutterBluePlus.lastScanResults;
  }

  Future<bool> connectBLE(String deviceId) async {
    try {
      _bleDevice = BluetoothDevice(remoteId: DeviceIdentifier(deviceId));
      await _bleDevice!.connect(license: License.nonprofit);
      var services = await _bleDevice!.discoverServices();
      bool foundData = false;
      for (var svc in services) {
        for (var chr in svc.characteristics) {
          // Layout push characteristic (write) — capture regardless of order.
          if (chr.uuid.toString().toLowerCase() == kLayoutCharUuid) {
            _layoutChar = chr;
          }
          if (!foundData && chr.properties.notify) {
            _dataChar = chr;
            await chr.setNotifyValue(true);
            chr.onValueReceived.listen((val) {
              var json = utf8.decode(val);
              var parsed = jsonDecode(json);
              _updateFromJson(parsed);
            });
            foundData = true;
          }
        }
      }
      if (foundData) {
        mode = ConnectionMode.ble;
        notifyListeners();
        return true;
      }
      return false;
    } catch (_) {
      return false;
    }
  }

  // --- WiFi ---
  Future<bool> connectWiFi(String host) async {
    _wifiHost = host;
    try {
      var resp = await http
          .get(Uri.parse('http://$host/api/status'))
          .timeout(Duration(seconds: 3));
      if (resp.statusCode == 200) {
        mode = ConnectionMode.wifi;
        _startWiFiPoll();
        notifyListeners();
        return true;
      }
    } catch (_) {}
    return false;
  }

  void _startWiFiPoll() {
    _pollTimer?.cancel();
    _pollTimer = Timer.periodic(Duration(seconds: 2), (_) async {
      if (_wifiHost == null) return;
      try {
        var resp = await http
            .get(Uri.parse('http://$_wifiHost/api/status'))
            .timeout(Duration(seconds: 2));
        if (resp.statusCode == 200) {
          _updateFromJson(jsonDecode(resp.body));
        }
      } catch (_) {}
    });
  }

  // --- Simulator ---
  void startSimulator() {
    mode = ConnectionMode.simulator;
    _simTimer = Timer.periodic(Duration(milliseconds: 50), (_) {
      _simHeading += 0.5;
      if (_simHeading >= 360) _simHeading = 0;
      _simPitch = sin(DateTime.now().millisecondsSinceEpoch / 2000.0) * 5;
      _simRoll = cos(DateTime.now().millisecondsSinceEpoch / 2500.0) * 5;

      _updateFromJson({
        'heading': _simHeading,
        'pitch': _simPitch,
        'roll': _simRoll,
        'temp': 25.0 + sin(DateTime.now().millisecondsSinceEpoch / 10000.0) * 2,
        'alt_ft': 6300 + sin(DateTime.now().millisecondsSinceEpoch / 5000.0) * 500,
        'pressure': 1013,
        'mx': 100, 'my': 200, 'mz': 300,
        'ip': 'simulator',
        'rssi': -50,
        'ssid': 'SIMULATOR',
        'time': '12:00:00',
        'uptime': 3600,
        'heap': 200000,
      });
      notifyListeners();
    });
  }

  void setSimPitchRoll(double pitch, double roll) {
    _simPitch = pitch;
    _simRoll = roll;
  }

  void setSimHeading(double heading) {
    _simHeading = heading;
  }

  // --- Common ---
  void _updateFromJson(Map<String, dynamic> json) {
    _data = DeviceData.fromJson(json);
  }

  /// Push a designed layout to the device. Tries BLE first (chunked writes
  /// with 'B'/'C'/'F' opcodes matching the firmware), falling back to the
  /// WiFi HTTP route if BLE is unavailable or the transfer fails.
  Future<PushResult> pushLayout(CwLayout layout) async {
    final json = layout.toJsonString();

    // 1) BLE primary
    if (_layoutChar != null) {
      try {
        const opBegin = 0x42; // 'B'
        const opChunk = 0x43; // 'C'
        const opFinish = 0x46; // 'F'
        final bytes = utf8.encode(json);
        const chunkSize = 160; // conservative; opcode byte + payload < MTU

        await _layoutChar!.write([opBegin], withoutResponse: false);
        for (var i = 0; i < bytes.length; i += chunkSize) {
          final end = (i + chunkSize < bytes.length) ? i + chunkSize : bytes.length;
          await _layoutChar!.write([opChunk, ...bytes.sublist(i, end)], withoutResponse: false);
        }
        await _layoutChar!.write([opFinish], withoutResponse: false);
        return PushResult.ok;
      } catch (_) {
        // fall through to WiFi
      }
    }

    // 2) WiFi fallback
    if (_wifiHost != null) {
      try {
        final resp = await http
            .post(Uri.parse('http://$_wifiHost/api/custom_layout'),
                headers: {'Content-Type': 'application/json'}, body: json)
            .timeout(const Duration(seconds: 5));
        if (resp.statusCode == 200) return PushResult.okViaWifi;
        return PushResult.failed;
      } catch (_) {
        return PushResult.failed;
      }
    }

    return _layoutChar == null && _wifiHost == null ? PushResult.notConnected : PushResult.failed;
  }

  Future<bool> sendCommand(String command) async {
    if (mode == ConnectionMode.wifi && _wifiHost != null) {
      try {
        var resp = await http
            .get(Uri.parse('http://$_wifiHost/$command'))
            .timeout(Duration(seconds: 3));
        return resp.statusCode == 200;
      } catch (_) {
        return false;
      }
    }
    if (mode == ConnectionMode.ble && _dataChar != null) {
      try {
        await _dataChar!.write(utf8.encode(command));
        return true;
      } catch (_) {
        return false;
      }
    }
    return false;
  }
}
