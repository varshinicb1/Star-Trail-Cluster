import 'package:flutter_test/flutter_test.dart';
import 'package:star_trail/services/device_service.dart';

void main() {
  group('DeviceService._routeCommand', () {
    test('reboot', () {
      expect(DeviceService.testRoute('reboot'), '/reboot');
    });

    test('brightness', () {
      expect(DeviceService.testRoute('brightness=80'), '/api/brightness?v=80');
      expect(DeviceService.testRoute('brightness=100'), '/api/brightness?v=100');
    });

    test('timeout', () {
      expect(DeviceService.testRoute('timeout=60'), '/api/timeout?v=60');
    });

    test('led_on / led_off', () {
      expect(DeviceService.testRoute('led_on'), '/api/led?state=on');
      expect(DeviceService.testRoute('led_off'), '/api/led?state=off');
    });

    test('led_color strips # prefix', () {
      expect(DeviceService.testRoute('led_color=FF0000'), '/api/led?color=FF0000');
      expect(DeviceService.testRoute('led_color=00FF00'), '/api/led?color=00FF00');
    });

    test('led_brightness', () {
      expect(DeviceService.testRoute('led_brightness=50'), '/api/led?brightness=50');
    });

    test('led_pattern', () {
      expect(DeviceService.testRoute('led_pattern=rainbow'), '/api/led?pattern=rainbow');
    });

    test('led_speed', () {
      expect(DeviceService.testRoute('led_speed=5'), '/api/led?speed=5');
    });

    test('media commands', () {
      expect(DeviceService.testRoute('play'), '/api/music?cmd=play');
      expect(DeviceService.testRoute('next'), '/api/music?cmd=next');
      expect(DeviceService.testRoute('prev'), '/api/music?cmd=prev');
    });

    test('widget config commands', () {
      expect(DeviceService.testRoute('widgets=clock,compass'), '/api/widgets?enabled=clock,compass');
      expect(DeviceService.testRoute('widget_order=Compass,Clock'), '/api/widgets?order=Compass,Clock');
      expect(DeviceService.testRoute('swipe_dir=horizontal'), '/api/widgets?swipe=horizontal');
      expect(DeviceService.testRoute('knob_mode=scroll'), '/api/widgets?knob=scroll');
    });

    test('unknown command falls through', () {
      expect(DeviceService.testRoute('custom_cmd'), '/custom_cmd');
      expect(DeviceService.testRoute('unknown=value'), '/unknown=value');
    });
  });
}
