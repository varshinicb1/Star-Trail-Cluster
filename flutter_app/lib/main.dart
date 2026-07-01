import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'providers/theme_provider.dart';
import 'services/device_service.dart';
import 'screens/welcome_screen.dart';
import 'screens/home_screen.dart';
import 'screens/config_screen.dart';
import 'screens/ota_screen.dart';
import 'screens/device_controls_screen.dart';
import 'screens/designer_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setSystemUIOverlayStyle(SystemUiOverlayStyle.light);
  runApp(const StarTrailApp());
}

class StarTrailApp extends StatelessWidget {
  const StarTrailApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => ThemeProvider()),
        ChangeNotifierProvider(create: (_) => DeviceService()),
      ],
      child: Consumer<ThemeProvider>(
        builder: (_, tp, _) => MaterialApp(
          title: 'Star Trail',
          theme: tp.theme.themeData,
          debugShowCheckedModeBanner: false,
          initialRoute: '/welcome',
          routes: {
            '/welcome': (_) => const WelcomeScreen(),
            '/home': (_) => const AppShell(),
            '/config': (_) => const ConfigScreen(),
            '/ota': (_) => const OTAScreen(),
            '/designer': (_) => const DesignerScreen(),
          },
        ),
      ),
    );
  }
}

class AppShell extends StatefulWidget {
  const AppShell({super.key});
  @override
  State<AppShell> createState() => _AppShellState();
}

class _AppShellState extends State<AppShell> {
  int _page = 0;
  final PageController _pageController = PageController();

  final _pages = const [HomeScreen(), DesignerScreen(), DeviceControlsScreen(), OTAScreen()];
  final _labels = ['Dashboard', 'Designer', 'Controls', 'OTA'];
  final _icons = [Icons.speed, Icons.draw, Icons.tune, Icons.system_update];

  @override
  void dispose() {
    _pageController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      body: PageView(
        controller: _pageController,
        onPageChanged: (i) => setState(() => _page = i),
        children: _pages,
      ),
      bottomNavigationBar: Container(
        decoration: BoxDecoration(
          border: Border(top: BorderSide(color: theme.cardBorder, width: 1)),
        ),
        child: BottomNavigationBar(
          backgroundColor: theme.surfaceLight,
          selectedItemColor: theme.primary,
          unselectedItemColor: theme.textMuted,
          type: BottomNavigationBarType.fixed,
          currentIndex: _page,
          onTap: (i) {
            _pageController.animateToPage(
              i,
              duration: const Duration(milliseconds: 300),
              curve: Curves.easeInOut,
            );
            setState(() => _page = i);
          },
          items: List.generate(_pages.length, (i) => BottomNavigationBarItem(
            icon: Icon(_icons[i], size: 20),
            activeIcon: Icon(_icons[i], size: 24),
            label: _labels[i],
          )),
        ),
      ),
    );
  }
}
