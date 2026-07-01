import 'dart:async';
import 'dart:convert';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:http/http.dart' as http;
import '../models/device_data.dart';

enum ConnectionMode { none, ble, wifi, simulator }

class DeviceService extends ChangeNotifier {
  DeviceData _data = DeviceData();
  DeviceData get data => _data;
  ConnectionMode mode = ConnectionMode.none;
  bool get isConnected => mode != ConnectionMode.none;

  // BLE
  BluetoothDevice? _bleDevice;
  BluetoothCharacteristic? _dataChar;
  BluetoothCharacteristic? _cmdChar;

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
      for (var svc in services) {
        for (var chr in svc.characteristics) {
          if (chr.properties.notify && _dataChar == null) {
            _dataChar = chr;
            await chr.setNotifyValue(true);
            chr.onValueReceived.listen((val) {
              var json = utf8.decode(val);
              var parsed = jsonDecode(json);
              _updateFromJson(parsed);
            });
          }
          if (chr.properties.write || chr.properties.writeWithoutResponse) {
            _cmdChar = chr;
          }
        }
      }
      if (_dataChar != null) {
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

  static String _routeCommand(String command) {
    var eq = command.indexOf('=');
    var key = eq > 0 ? command.substring(0, eq) : command;
    var val = eq > 0 ? command.substring(eq + 1) : '';
    switch (key) {
      case 'reboot':       return '/reboot';
      case 'brightness':   return '/api/brightness?v=$val';
      case 'timeout':      return '/api/timeout?v=$val';
      case 'led_on':       return '/api/led?state=on';
      case 'led_off':      return '/api/led?state=off';
      case 'led_color':    return '/api/led?color=${val.replaceFirst("#", "")}';
      case 'led_brightness': return '/api/led?brightness=$val';
      case 'led_pattern':  return '/api/led?pattern=$val';
      case 'led_speed':    return '/api/led?speed=$val';
      case 'widgets':      return '/api/widgets?enabled=$val';
      case 'widget_order': return '/api/widgets?order=$val';
      case 'swipe_dir':    return '/api/widgets?swipe=$val';
      case 'knob_mode':    return '/api/widgets?knob=$val';
      case 'play':         return '/api/music?cmd=play';
      case 'next':         return '/api/music?cmd=next';
      case 'prev':         return '/api/music?cmd=prev';
      default:             return '/$command';
    }
  }

  Future<bool> sendCommand(String command) async {
    if (mode == ConnectionMode.wifi && _wifiHost != null) {
      try {
        var path = _routeCommand(command);
        var resp = await http
            .get(Uri.parse('http://$_wifiHost$path'))
            .timeout(Duration(seconds: 3));
        return resp.statusCode == 200;
      } catch (_) {
        return false;
      }
    }
    if (mode == ConnectionMode.ble && _cmdChar != null) {
      try {
        await _cmdChar!.write(utf8.encode(command));
        return true;
      } catch (_) {
        return false;
      }
    }
    return false;
  }
}
