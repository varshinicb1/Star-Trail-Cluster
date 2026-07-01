import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:open_filex/open_filex.dart';
import 'package:path_provider/path_provider.dart';
import 'package:package_info_plus/package_info_plus.dart';

class AppUpdateInfo {
  final String version;
  final int buildNumber;
  final String apkUrl;
  final String changelog;
  const AppUpdateInfo({
    required this.version,
    required this.buildNumber,
    required this.apkUrl,
    this.changelog = '',
  });
}

class UpdateService extends ChangeNotifier {
  String githubRepo = 'edgehax/star_trail';

  AppUpdateInfo? _latestUpdate;
  bool _checking = false;
  bool _downloading = false;
  double _downloadProgress = 0;
  String? _error;
  bool _updateAvailable = false;

  AppUpdateInfo? get latestUpdate => _latestUpdate;
  bool get checking => _checking;
  bool get downloading => _downloading;
  double get downloadProgress => _downloadProgress;
  String? get error => _error;
  bool get updateAvailable => _updateAvailable;

  Future<PackageInfo> _getPackageInfo() => PackageInfo.fromPlatform();

  String _parseVersionTag(String tag) {
    return tag.replaceAll(RegExp(r'^v'), '');
  }

  Future<AppUpdateInfo?> checkForUpdate() async {
    _checking = true;
    _error = null;
    notifyListeners();

    try {
      final currentInfo = await _getPackageInfo();
      final currentVersion = currentInfo.version;

      final resp = await http
          .get(
            Uri.parse('https://api.github.com/repos/$githubRepo/releases/latest'),
            headers: {'Accept': 'application/vnd.github.v3+json'},
          )
          .timeout(Duration(seconds: 10));

      if (resp.statusCode != 200) {
        _error = 'GitHub API error: ${resp.statusCode}';
        _updateAvailable = false;
        return null;
      }

      final data = jsonDecode(resp.body) as Map<String, dynamic>;
      final tagVersion = _parseVersionTag(data['tag_name'] as String? ?? '');
      final changelog = data['body'] as String? ?? '';
      final assets = data['assets'] as List<dynamic>? ?? [];

      String? apkUrl;
      for (final asset in assets) {
        final name = asset['name'] as String? ?? '';
        if (name.endsWith('.apk')) {
          apkUrl = asset['browser_download_url'] as String?;
          break;
        }
      }

      if (apkUrl == null) {
        _error = 'No APK found in latest release';
        _updateAvailable = false;
        return null;
      }

      final newer = _isNewer(tagVersion, currentVersion);

      _latestUpdate = AppUpdateInfo(
        version: tagVersion,
        buildNumber: 0,
        apkUrl: apkUrl,
        changelog: changelog,
      );

      _updateAvailable = newer;
      return _latestUpdate;
    } catch (e) {
      _error = e.toString();
      _updateAvailable = false;
      return null;
    } finally {
      _checking = false;
      notifyListeners();
    }
  }

  bool _isNewer(String tagVersion, String currentVersion) {
    final tagParts = tagVersion.split('.').map((e) => int.tryParse(e) ?? 0).toList();
    final currentParts = currentVersion.split('.').map((e) => int.tryParse(e) ?? 0).toList();
    for (int i = 0; i < tagParts.length && i < currentParts.length; i++) {
      if (tagParts[i] > currentParts[i]) return true;
      if (tagParts[i] < currentParts[i]) return false;
    }
    return tagParts.length > currentParts.length;
  }

  Future<String?> downloadApk() async {
    if (_latestUpdate == null) return null;
    _downloading = true;
    _downloadProgress = 0;
    notifyListeners();

    try {
      final dir = await getTemporaryDirectory();
      final filePath = '${dir.path}/star_trail_${_latestUpdate!.version}.apk';
      final file = File(filePath);

      final resp = http.Client().send(
        http.Request('GET', Uri.parse(_latestUpdate!.apkUrl)),
      );

      final streamed = await resp;
      final total = streamed.contentLength ?? 0;
      final sink = file.openWrite();

      int received = 0;
      await for (final chunk in streamed.stream) {
        sink.add(chunk);
        received += chunk.length;
        if (total > 0) {
          _downloadProgress = received / total;
          notifyListeners();
        }
      }
      await sink.close();

      _downloadProgress = 1.0;
      notifyListeners();
      return filePath;
    } catch (e) {
      _error = 'Download failed: $e';
      return null;
    } finally {
      _downloading = false;
      notifyListeners();
    }
  }

  Future<void> installApk(String path) async {
    try {
      await OpenFilex.open(path, type: 'application/vnd.android.package-archive');
    } catch (e) {
      _error = 'Install failed: $e';
      notifyListeners();
    }
  }

  void clearError() {
    _error = null;
    notifyListeners();
  }

  void reset() {
    _latestUpdate = null;
    _updateAvailable = false;
    _checking = false;
    _downloading = false;
    _downloadProgress = 0;
    _error = null;
    notifyListeners();
  }
}
