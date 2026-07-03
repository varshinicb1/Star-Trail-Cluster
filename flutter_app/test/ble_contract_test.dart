import 'package:flutter_test/flutter_test.dart';
import 'package:star_trail/services/device_service.dart';

/// Locks the BLE GATT UUIDs to the firmware's source of truth
/// (BenzCluster/ble_notify.cpp) so a future edit on either side can't quietly
/// break device discovery again. This is a regression test for a real bug:
/// the app used to pick "whichever writable characteristic was found first",
/// which could bind the command channel to the layout-push characteristic
/// (or vice versa) depending on BLE discovery order.
void main() {
  group('BLE characteristic UUID contract', () {
    test('layout characteristic matches firmware LAYOUT_CHAR_UUID', () {
      expect(kLayoutCharUuid, '00002222-3333-4444-5555-666677778889');
    });

    test('command characteristic matches firmware NOTIFY_CHAR_UUID', () {
      expect(kNotifyCharUuid, 'deadbeef-cafe-1234-5678-abcdef012346');
    });

    test('layout and command characteristics are distinct', () {
      expect(kLayoutCharUuid, isNot(equals(kNotifyCharUuid)));
    });
  });
}
