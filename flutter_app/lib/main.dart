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

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setSystemUIOverlayStyle(SystemUiOverlayStyle.light);
  SystemChrome.setPreferredOrientations([DeviceOrientation.portraitUp]);
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

class _AppShellState extends State<AppShell> with TickerProviderStateMixin {
  int _page = 0;
  late PageController _pageController;
  late AnimationController _navAnimController;

  final _pages = const [HomeScreen(), DeviceControlsScreen(), OTAScreen()];
  final _labels = ['Dashboard', 'Controls', 'OTA'];
  final _icons = [Icons.speed, Icons.tune, Icons.system_update];

  @override
  void initState() {
    super.initState();
    _pageController = PageController();
    _navAnimController = AnimationController(
      vsync: this,
      duration: Duration(milliseconds: 400),
    );
  }

  @override
  void dispose() {
    _pageController.dispose();
    _navAnimController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeProvider>().theme;
    return Scaffold(
      body: PageView(
        controller: _pageController,
        onPageChanged: (i) {
          setState(() => _page = i);
          _navAnimController.forward(from: 0);
        },
        children: _pages,
      ),
      bottomNavigationBar: _AnimatedNavBar(
        theme: theme,
        page: _page,
        labels: _labels,
        icons: _icons,
        onTap: (i) {
          _pageController.animateToPage(
            i,
            duration: Duration(milliseconds: 350),
            curve: Curves.easeInOutCubic,
          );
        },
      ),
    );
  }
}

class _AnimatedNavBar extends StatelessWidget {
  final AppTheme theme;
  final int page;
  final List<String> labels;
  final List<IconData> icons;
  final ValueChanged<int> onTap;

  const _AnimatedNavBar({
    required this.theme,
    required this.page,
    required this.labels,
    required this.icons,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 72,
      decoration: BoxDecoration(
        color: theme.surfaceLight,
        border: Border(top: BorderSide(color: theme.cardBorder, width: 0.5)),
        boxShadow: [
          BoxShadow(
            color: theme.primary.withAlpha(6),
            blurRadius: 24,
            offset: Offset(0, -4),
          ),
        ],
      ),
      child: Row(
        children: List.generate(labels.length, (i) {
          final selected = page == i;
          return Expanded(
            child: GestureDetector(
              onTap: () => onTap(i),
              behavior: HitTestBehavior.opaque,
              child: AnimatedContainer(
                duration: Duration(milliseconds: 300),
                curve: Curves.easeOutCubic,
                margin: EdgeInsets.symmetric(vertical: 8, horizontal: 4),
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(14),
                  color: selected ? theme.primary.withAlpha(15) : Colors.transparent,
                ),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Stack(
                      alignment: Alignment.center,
                      children: [
                        if (selected)
                          Container(
                            width: 36,
                            height: 36,
                            decoration: BoxDecoration(
                              shape: BoxShape.circle,
                              boxShadow: [
                                BoxShadow(
                                  color: theme.primary.withAlpha(30),
                                  blurRadius: 12,
                                  spreadRadius: 2,
                                ),
                              ],
                            ),
                          ),
                        Icon(
                          icons[i],
                          size: selected ? 24 : 22,
                          color: selected ? theme.primary : theme.textMuted,
                        ),
                      ],
                    ),
                    SizedBox(height: 2),
                    Text(
                      labels[i],
                      style: TextStyle(
                        fontSize: 10,
                        fontWeight: selected ? FontWeight.w600 : FontWeight.w400,
                        color: selected ? theme.primary : theme.textMuted,
                        letterSpacing: 0.5,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          );
        }),
      ),
    );
  }
}
