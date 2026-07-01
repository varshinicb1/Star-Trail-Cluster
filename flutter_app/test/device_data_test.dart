import 'package:flutter_test/flutter_test.dart';
import 'package:star_trail/models/device_data.dart';

void main() {
  group('DeviceData.fromJson', () {
    test('parses full JSON from firmware', () {
      final json = {
        'heading': 180.5,
        'pitch': 2.3,
        'roll': -1.7,
        'temp': 27.5,
        'alt_ft': 6300,
        'pressure': 1012,
        'mx': 100,
        'my': 200,
        'mz': 300,
        'ip': '10.91.5.73',
        'rssi': -65,
        'ssid': 'StarTrail_AP',
        'time': '14:30:00',
        'uptime': 3600,
        'heap': 180000,
      };

      final data = DeviceData.fromJson(json);

      expect(data.heading, 180.5);
      expect(data.pitch, 2.3);
      expect(data.roll, -1.7);
      expect(data.temperature, 27.5);
      expect(data.altitude, 6300);
      expect(data.pressure, 1012);
      expect(data.magRawX, 100);
      expect(data.magRawY, 200);
      expect(data.magRawZ, 300);
      expect(data.ip, '10.91.5.73');
      expect(data.rssi, -65);
      expect(data.ssid, 'StarTrail_AP');
      expect(data.time, '14:30:00');
      expect(data.uptime, 3600);
      expect(data.heap, 180000);
    });

    test('handles abbreviated keys (backward compat)', () {
      final json = {'h': 90.0, 'p': 1.0, 'r': 0.5, 't': 25.0, 'a': 5000, 'pr': 1015};

      final data = DeviceData.fromJson(json);

      expect(data.heading, 90.0);
      expect(data.pitch, 1.0);
      expect(data.roll, 0.5);
      expect(data.temperature, 25.0);
      expect(data.altitude, 5000);
      expect(data.pressure, 1015);
    });

    test('handles missing fields with defaults', () {
      final data = DeviceData.fromJson({});

      expect(data.heading, 0);
      expect(data.pitch, 0);
      expect(data.roll, 0);
      expect(data.temperature, 25);
      expect(data.altitude, 920);
      expect(data.pressure, 1013);
      expect(data.ip, '--');
      expect(data.rssi, -100);
      expect(data.ssid, '--');
      expect(data.time, '--:--:--');
      expect(data.uptime, 0);
      expect(data.heap, 0);
    });

    test('handles null fields', () {
      final json = {
        'heading': null,
        'pitch': null,
        'temp': null,
        'ip': null,
      };

      final data = DeviceData.fromJson(json);

      expect(data.heading, 0);
      expect(data.pitch, 0);
      expect(data.temperature, 25);
      expect(data.ip, '--');
    });
  });

  group('DeviceData.copyWith', () {
    test('returns new instance with updated fields', () {
      final original = DeviceData(heading: 90, pitch: 1.0);
      final updated = original.copyWith(heading: 180, pitch: 2.0);

      expect(updated.heading, 180);
      expect(updated.pitch, 2.0);

      expect(original.heading, 90);
      expect(original.pitch, 1.0);
    });

    test('preserves unset fields', () {
      final original = DeviceData(heading: 90, temperature: 30);
      final updated = original.copyWith(heading: 180);

      expect(updated.heading, 180);
      expect(updated.temperature, 30);
    });
  });
}
