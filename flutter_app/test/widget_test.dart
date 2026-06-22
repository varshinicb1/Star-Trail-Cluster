import 'package:flutter_test/flutter_test.dart';
import 'package:star_trail/main.dart';

void main() {
  testWidgets('App renders with bottom nav', (WidgetTester tester) async {
    await tester.pumpWidget(const StarTrailApp());
    expect(find.text('Live'), findsOneWidget);
    expect(find.text('Emulator'), findsOneWidget);
    expect(find.text('Config'), findsOneWidget);
    expect(find.text('OTA'), findsOneWidget);
  });
}
