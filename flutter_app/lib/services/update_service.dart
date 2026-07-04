import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:open_filex/open_filex.dart';
import 'package:path_provider/path_provider.dart';
import 'package:package_info_plus/package_info_plus.dart';

class AppUpdateInfo {
  final String version;
  final String apkUrl;
  final String changelog;
  const AppUpdateInfo({required this.version, required this.apkUrl, this.changelog = ''});
}

/// A firmware `.bin` attached to a GitHub release, ready to push to the
/// device over the existing WiFi OTA route (`POST /update`) — no manual file
/// picking required. See [DeviceService.uploadFirmware].
class FirmwareUpdateInfo {
  final String version;
  final String binUrl;
  final String changelog;
  const FirmwareUpdateInfo({required this.version, required this.binUrl, this.changelog = ''});
}

/// Checks GitHub Releases once and derives BOTH the app-APK and the
/// firmware-.bin update info from the same release, since both artifacts are
/// published together (see the repo's release notes).
///
/// Requires the GitHub repo to be PUBLIC — GitHub's REST API returns 404 for
/// unauthenticated release lookups on private repos, which is indistinguishable
/// from "no releases exist" without inspecting the repo itself.
class UpdateService extends ChangeNotifier {
  String githubRepo = 'varshinicb1/Star-Trail-Cluster';

  AppUpdateInfo? _latestApp;
  FirmwareUpdateInfo? _latestFirmware;
  bool _checking = false;
  bool _downloading = false;
  double _downloadProgress = 0;
  String? _error;
  bool _appUpdateAvailable = false;

  AppUpdateInfo? get latestUpdate => _latestApp;
  FirmwareUpdateInfo? get latestFirmware => _latestFirmware;
  bool get checking => _checking;
  bool get downloading => _downloading;
  double get downloadProgress => _downloadProgress;
  String? get error => _error;
  bool get updateAvailable => _appUpdateAvailable;

  Future<PackageInfo> _getPackageInfo() => PackageInfo.fromPlatform();

  String _parseVersionTag(String tag) => tag.replaceAll(RegExp(r'^v'), '');

  /// GitHub's API requires a User-Agent header on every request and rejects
  /// requests without one (403) — easy to miss since browsers set it for you.
  static const _apiHeaders = {
    'Accept': 'application/vnd.github.v3+json',
    'User-Agent': 'StarTrailApp',
  };

  Future<AppUpdateInfo?> checkForUpdate() async {
    _checking = true;
    _error = null;
    notifyListeners();

    try {
      final currentInfo = await _getPackageInfo();
      final resp = await http
          .get(Uri.parse('https://api.github.com/repos/$githubRepo/releases/latest'), headers: _apiHeaders)
          .timeout(const Duration(seconds: 10));

      if (resp.statusCode == 404) {
        _error = 'No published release found (or the repo is private — '
            'GitHub hides releases from unauthenticated requests on private repos)';
        _appUpdateAvailable = false;
        return null;
      }
      if (resp.statusCode != 200) {
        _error = 'GitHub API error: ${resp.statusCode}';
        _appUpdateAvailable = false;
        return null;
      }

      final data = jsonDecode(resp.body) as Map<String, dynamic>;
      final tagVersion = _parseVersionTag(data['tag_name'] as String? ?? '');
      final changelog = data['body'] as String? ?? '';
      final assets = data['assets'] as List<dynamic>? ?? [];

      String? apkUrl;
      String? binUrl;
      for (final asset in assets) {
        final name = (asset['name'] as String? ?? '').toLowerCase();
        final url = asset['browser_download_url'] as String?;
        if (apkUrl == null && name.endsWith('.apk')) apkUrl = url;
        if (binUrl == null && name.endsWith('.bin')) binUrl = url;
      }

      _latestFirmware = binUrl != null
          ? FirmwareUpdateInfo(version: tagVersion, binUrl: binUrl, changelog: changelog)
          : null;

      if (apkUrl == null) {
        _error = 'No APK found in latest release';
        _appUpdateAvailable = false;
        _latestApp = null;
        return null;
      }

      _latestApp = AppUpdateInfo(version: tagVersion, apkUrl: apkUrl, changelog: changelog);
      _appUpdateAvailable = _isNewer(tagVersion, currentInfo.version);
      return _latestApp;
    } catch (e) {
      _error = e.toString();
      _appUpdateAvailable = false;
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
    if (_latestApp == null) return null;
    _downloading = true;
    _downloadProgress = 0;
    notifyListeners();

    try {
      final dir = await getTemporaryDirectory();
      final filePath = '${dir.path}/star_trail_${_latestApp!.version}.apk';
      final file = File(filePath);

      final streamed = await http.Client().send(http.Request('GET', Uri.parse(_latestApp!.apkUrl)));
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

  /// Downloads the latest firmware `.bin` into memory so it can be pushed
  /// straight to the device over WiFi (see `DeviceService.uploadFirmware`) —
  /// no manual file picking.
  Future<Uint8List?> downloadFirmwareBytes({void Function(double)? onProgress}) async {
    if (_latestFirmware == null) return null;
    _downloading = true;
    _downloadProgress = 0;
    notifyListeners();

    try {
      final streamed = await http.Client().send(http.Request('GET', Uri.parse(_latestFirmware!.binUrl)));
      final total = streamed.contentLength ?? 0;
      final builder = BytesBuilder(copy: false);

      int received = 0;
      await for (final chunk in streamed.stream) {
        builder.add(chunk);
        received += chunk.length;
        if (total > 0) {
          _downloadProgress = received / total;
          onProgress?.call(_downloadProgress);
          notifyListeners();
        }
      }
      _downloadProgress = 1.0;
      notifyListeners();
      return builder.takeBytes();
    } catch (e) {
      _error = 'Firmware download failed: $e';
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
    _latestApp = null;
    _latestFirmware = null;
    _appUpdateAvailable = false;
    _checking = false;
    _downloading = false;
    _downloadProgress = 0;
    _error = null;
    notifyListeners();
  }
}
