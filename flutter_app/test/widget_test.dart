import 'package:flutter_test/flutter_test.dart';
import 'package:star_trail/main.dart';

void main() {
  testWidgets('App boots to the welcome screen', (WidgetTester tester) async {
    await tester.pumpWidget(const StarTrailApp());
    await tester.pump();
    // The app opens on the welcome screen with the premium wordmark + CTA.
    expect(find.text('STAR TRAIL'), findsOneWidget);
    expect(find.text('Get Started'), findsOneWidget);
    expect(tester.takeException(), isNull);
  });
}
