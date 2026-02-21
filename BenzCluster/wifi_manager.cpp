#include "wifi_manager.h"
#include "config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>


static bool wifiConnected = false;
static char timeBuffer[32] = "00:00:00";
static float cachedSeaLevelPressure = DEFAULT_SEA_LEVEL_PRESSURE;

void wifi_init() {
  Serial.printf("[WIFI] Connecting to %s...\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Non-blocking: check in loop or task
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\n[WIFI] Connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());

    // Sync time
    wifi_sync_time();

    // Fetch weather data
    wifi_get_sea_level_pressure();
  } else {
    Serial.println("\n[WIFI] Connection failed, running offline");
  }
}

bool wifi_is_connected() { return WiFi.status() == WL_CONNECTED; }

void wifi_sync_time() {
  if (!wifi_is_connected())
    return;

  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);
    Serial.printf("[NTP] Time synced: %s IST\n", timeBuffer);
  } else {
    Serial.println("[NTP] Failed to sync time");
  }
}

const char *wifi_get_time() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);
  }
  return timeBuffer;
}

float wifi_get_sea_level_pressure() {
  if (!wifi_is_connected()) {
    return cachedSeaLevelPressure;
  }

  // Check if API key is configured
  if (strlen(WEATHER_API_KEY) < 10) {
    Serial.println("[WEATHER] No API key, using default pressure");
    return DEFAULT_SEA_LEVEL_PRESSURE;
  }

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=";
  url += WEATHER_LAT;
  url += "&lon=";
  url += WEATHER_LON;
  url += "&appid=";
  url += WEATHER_API_KEY;

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      float pressure = doc["main"]["pressure"];
      if (pressure > 900 && pressure < 1100) {
        cachedSeaLevelPressure = pressure;
        Serial.printf("[WEATHER] Sea-level pressure: %.1f hPa\n", pressure);
      }
    }
  } else {
    Serial.printf("[WEATHER] API error: %d\n", httpCode);
  }

  http.end();
  return cachedSeaLevelPressure;
}
