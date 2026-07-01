import 'package:flutter_test/flutter_test.dart';
import 'package:star_trail/main.dart';

void main() {
  testWidgets('App launches without crash', (WidgetTester tester) async {
    await tester.pumpWidget(const StarTrailApp());
    await tester.pump();

    expect(find.text('STAR TRAIL'), findsOneWidget);
    expect(find.text('ENTER'), findsOneWidget);
  });

  testWidgets('Welcome screen shows theme options', (WidgetTester tester) async {
    await tester.pumpWidget(const StarTrailApp());
    await tester.pump();

    expect(find.text('Star Trail'), findsWidgets);
    expect(find.text('Illuminati'), findsWidgets);
    expect(find.text('DR'), findsWidgets);
  });
}
