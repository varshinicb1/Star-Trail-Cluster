import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

class ThemeProvider extends ChangeNotifier {
  AppThemeMode _mode = AppThemeMode.starTrail;

  AppThemeMode get mode => _mode;
  AppTheme get theme => AppTheme.fromMode(_mode);

  void setTheme(AppThemeMode mode) {
    if (_mode == mode) return;
    _mode = mode;
    notifyListeners();
  }

  void cycleTheme() {
    int next = (_mode.index + 1) % AppThemeMode.values.length;
    _mode = AppThemeMode.values[next];
    notifyListeners();
  }
}
