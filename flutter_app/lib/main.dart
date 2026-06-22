import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'services/device_service.dart';
import 'screens/home_screen.dart';
import 'screens/emulator_screen.dart';
import 'screens/config_screen.dart';
import 'screens/ota_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setSystemUIOverlayStyle(SystemUiOverlayStyle.dark);
  runApp(const StarTrailApp());
}

class StarTrailApp extends StatelessWidget {
  const StarTrailApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => DeviceService()..startSimulator(),
      child: MaterialApp(
        title: 'Star Trail',
        debugShowCheckedModeBanner: false,
        theme: ThemeData.dark(useMaterial3: true).copyWith(
          scaffoldBackgroundColor: const Color(0xFF0A0A0A),
        ),
        initialRoute: '/home',
        routes: {
          '/home': (_) => const AppShell(),
          '/emulator': (_) => const EmulatorScreen(),
          '/config': (_) => const ConfigScreen(),
          '/ota': (_) => const OTAScreen(),
        },
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

  final _pages = [const HomeScreen(), const EmulatorScreen(), const ConfigScreen(), const OTAScreen()];
  final _labels = ['Live', 'Emulator', 'Config', 'OTA'];
  final _icons = [Icons.speed, Icons.smartphone, Icons.settings, Icons.system_update];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: IndexedStack(index: _page, children: _pages),
      bottomNavigationBar: Container(
        decoration: const BoxDecoration(
          border: Border(top: BorderSide(color: Color(0xFF1A1A2E), width: 1)),
        ),
        child: BottomNavigationBar(
          backgroundColor: const Color(0xFF0F0F18),
          selectedItemColor: const Color(0xFF00CCFF),
          unselectedItemColor: const Color(0xFF555555),
          type: BottomNavigationBarType.fixed,
          currentIndex: _page,
          onTap: (i) => setState(() => _page = i),
          items: List.generate(4, (i) => BottomNavigationBarItem(
            icon: Icon(_icons[i], size: 20),
            activeIcon: Icon(_icons[i], size: 24),
            label: _labels[i],
          )),
        ),
      ),
    );
  }
}
