class DeviceData {
  double heading;
  double pitch;
  double roll;
  double temperature;
  double altitude;
  double pressure;
  int magRawX;
  int magRawY;
  int magRawZ;
  double accelX;
  double accelY;
  double accelZ;
  String ip;
  int rssi;
  String ssid;
  String time;
  int uptime;
  int heap;

  DeviceData({
    this.heading = 0,
    this.pitch = 0,
    this.roll = 0,
    this.temperature = 25,
    this.altitude = 920,
    this.pressure = 1013,
    this.magRawX = 0,
    this.magRawY = 0,
    this.magRawZ = 0,
    this.accelX = 0,
    this.accelY = 0,
    this.accelZ = 0,
    this.ip = '--',
    this.rssi = -100,
    this.ssid = '--',
    this.time = '--:--:--',
    this.uptime = 0,
    this.heap = 0,
  });

  factory DeviceData.fromJson(Map<String, dynamic> json) {
    return DeviceData(
      heading: (json['heading'] ?? 0).toDouble(),
      pitch: (json['pitch'] ?? 0).toDouble(),
      roll: (json['roll'] ?? 0).toDouble(),
      temperature: (json['temp'] ?? 25).toDouble(),
      altitude: (json['alt_ft'] ?? 920).toDouble(),
      pressure: (json['pressure'] ?? 1013).toDouble(),
      magRawX: (json['mx'] ?? 0).toInt(),
      magRawY: (json['my'] ?? 0).toInt(),
      magRawZ: (json['mz'] ?? 0).toInt(),
      ip: json['ip'] ?? '--',
      rssi: (json['rssi'] ?? -100).toInt(),
      ssid: json['ssid'] ?? '--',
      time: json['time'] ?? '--:--:--',
      uptime: (json['uptime'] ?? 0).toInt(),
      heap: (json['heap'] ?? 0).toInt(),
    );
  }

  DeviceData copyWith({
    double? heading,
    double? pitch,
    double? roll,
    double? temperature,
    double? altitude,
    double? pressure,
    int? magRawX,
    int? magRawY,
    int? magRawZ,
    double? accelX,
    double? accelY,
    double? accelZ,
    String? ip,
    int? rssi,
    String? ssid,
    String? time,
    int? uptime,
    int? heap,
  }) {
    return DeviceData(
      heading: heading ?? this.heading,
      pitch: pitch ?? this.pitch,
      roll: roll ?? this.roll,
      temperature: temperature ?? this.temperature,
      altitude: altitude ?? this.altitude,
      pressure: pressure ?? this.pressure,
      magRawX: magRawX ?? this.magRawX,
      magRawY: magRawY ?? this.magRawY,
      magRawZ: magRawZ ?? this.magRawZ,
      accelX: accelX ?? this.accelX,
      accelY: accelY ?? this.accelY,
      accelZ: accelZ ?? this.accelZ,
      ip: ip ?? this.ip,
      rssi: rssi ?? this.rssi,
      ssid: ssid ?? this.ssid,
      time: time ?? this.time,
      uptime: uptime ?? this.uptime,
      heap: heap ?? this.heap,
    );
  }
}

class WidgetConfig {
  final bool clockEnabled;
  final bool compassEnabled;
  final bool attitudeEnabled;
  final bool alttempEnabled;
  final bool gforceEnabled;
  final bool musicEnabled;
  final bool airplaneEnabled;
  final List<String> widgetOrder;

  WidgetConfig({
    this.clockEnabled = true,
    this.compassEnabled = true,
    this.attitudeEnabled = true,
    this.alttempEnabled = true,
    this.gforceEnabled = true,
    this.musicEnabled = true,
    this.airplaneEnabled = true,
    List<String>? widgetOrder,
  }) : widgetOrder = widgetOrder ?? [
          'Clock', 'Compass', 'Attitude', 'AltTemp',
          'G-Force', 'Music', 'Airplane',
        ];

  List<String> get enabledWidgets =>
      widgetOrder.where((w) => _isEnabled(w)).toList();

  bool _isEnabled(String name) {
    switch (name) {
      case 'Clock': return clockEnabled;
      case 'Compass': return compassEnabled;
      case 'Attitude': return attitudeEnabled;
      case 'AltTemp': return alttempEnabled;
      case 'G-Force': return gforceEnabled;
      case 'Music': return musicEnabled;
      case 'Airplane': return airplaneEnabled;
      default: return true;
    }
  }

  WidgetConfig copyWith({
    bool? clockEnabled,
    bool? compassEnabled,
    bool? attitudeEnabled,
    bool? alttempEnabled,
    bool? gforceEnabled,
    bool? musicEnabled,
    bool? airplaneEnabled,
    List<String>? widgetOrder,
  }) {
    return WidgetConfig(
      clockEnabled: clockEnabled ?? this.clockEnabled,
      compassEnabled: compassEnabled ?? this.compassEnabled,
      attitudeEnabled: attitudeEnabled ?? this.attitudeEnabled,
      alttempEnabled: alttempEnabled ?? this.alttempEnabled,
      gforceEnabled: gforceEnabled ?? this.gforceEnabled,
      musicEnabled: musicEnabled ?? this.musicEnabled,
      airplaneEnabled: airplaneEnabled ?? this.airplaneEnabled,
      widgetOrder: widgetOrder ?? this.widgetOrder,
    );
  }
}

class AppSettings {
  final String wifiSsid;
  final String wifiPassword;
  final String splashTheme;
  final int brightness;
  final int ledBrightness;

  AppSettings({
    this.wifiSsid = '',
    this.wifiPassword = '',
    this.splashTheme = 'star_trail',
    this.brightness = 100,
    this.ledBrightness = 50,
  });
}
