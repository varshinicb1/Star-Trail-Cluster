import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:provider/provider.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:star_trail/providers/theme_provider.dart';
import 'package:star_trail/services/device_service.dart';
import 'package:star_trail/screens/designer_screen.dart';

void main() {
  testWidgets('DesignerScreen builds without throwing', (tester) async {
    SharedPreferences.setMockInitialValues({});
    await tester.pumpWidget(
      MultiProvider(
        providers: [
          ChangeNotifierProvider(create: (_) => ThemeProvider()),
          ChangeNotifierProvider(create: (_) => DeviceService()),
        ],
        child: const MaterialApp(home: DesignerScreen()),
      ),
    );
    await tester.pump(const Duration(milliseconds: 100));
    expect(tester.takeException(), isNull);
  });
}
