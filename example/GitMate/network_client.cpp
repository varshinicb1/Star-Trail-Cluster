#include "network_client.h"
#include <Arduino.h>
#include <Preferences.h>

GitMateNetwork::GitMateNetwork() {
  _ssid = "";
  _password = "";
  _agentIP = AGENT_IP;
  _agentPort = AGENT_PORT;
  _token = "";
  _status = NET_DISCONNECTED;
}

bool GitMateNetwork::begin(const char *ssid, const char *password) {
  _ssid = ssid;
  _password = password;

  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid.c_str(), _password.c_str());

  DEBUG_PRINTLN_NETWORK("Connecting to WiFi...");

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    DEBUG_PRINT_NETWORK(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN_NETWORK("\nWiFi Connected!");
    DEBUG_PRINT_NETWORK("IP Address: ");
    DEBUG_PRINTLN_NETWORK(WiFi.localIP());

    // Load token from preferences
    Preferences prefs;
    prefs.begin("gitmate", true);
    _token = prefs.getString("token", "");
    prefs.end();

    if (_token.length() > 0) {
      DEBUG_PRINTLN_NETWORK("Loaded pairing token");
      _status = NET_PAIRED;
    } else {
      _status = NET_CONNECTED;
    }
    return true;
  } else {
    DEBUG_PRINTLN_NETWORK("\nWiFi Connection Failed");
    _status = NET_ERROR;
    return false;
  }
}

NetworkStatus GitMateNetwork::getStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    return NET_DISCONNECTED;
  }
  return _status;
}

bool GitMateNetwork::reconnect() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.disconnect();
  WiFi.begin(_ssid.c_str(), _password.c_str());

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000) {
    delay(100);
  }

  return WiFi.status() == WL_CONNECTED;
}

void GitMateNetwork::setAgentAddress(const char *ip, int port) {
  _agentIP = ip;
  _agentPort = port;
}

void GitMateNetwork::setToken(const char *token) {
  _token = token;

  // Save to preferences
  Preferences prefs;
  prefs.begin("gitmate", false);
  prefs.putString("token", _token);
  prefs.end();

  _status = NET_PAIRED;
}

String GitMateNetwork::getToken() { return _token; }

bool GitMateNetwork::isPaired() { return _token.length() > 0; }

String GitMateNetwork::getAgentURL() {
  return "http://" + _agentIP + ":" + String(_agentPort);
}

String GitMateNetwork::buildRequest(const char *cmd, JsonDocument *args) {
  JsonDocument doc;
  doc["cmd"] = cmd;
  doc["token"] = _token;

  if (args != nullptr) {
    doc["args"] = *args;
  } else {
    doc.createNestedObject("args");
  }

  String output;
  serializeJson(doc, output);
  return output;
}

CommandResponse GitMateNetwork::parseResponse(String json) {
  CommandResponse response;
  response.success = false;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    response.message = "JSON Error: " + String(error.c_str());
    return response;
  }

  String status = doc["status"] | "error";
  response.success = (status == "ok");
  response.message = doc["msg"] | "";

  if (doc.containsKey("payload")) {
    response.payload = doc["payload"];
  }

  return response;
}

CommandResponse GitMateNetwork::sendCommand(const char *cmd,
                                            JsonDocument *args) {
  CommandResponse response;
  response.success = false;

  if (WiFi.status() != WL_CONNECTED) {
    if (!reconnect()) {
      response.message = "WiFi Disconnected";
      return response;
    }
  }

  HTTPClient http;
  String url = getAgentURL() + "/command";

  DEBUG_PRINT_NETWORK("POST " + url);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT);

  String requestBody = buildRequest(cmd, args);
  DEBUG_PRINTLN_NETWORK("Body: " + requestBody);

  int httpCode = http.POST(requestBody);

  if (httpCode > 0) {
    DEBUG_PRINT_NETWORK("HTTP Code: ");
    DEBUG_PRINTLN_NETWORK(httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      DEBUG_PRINTLN_NETWORK("Response: " + payload);
      response = parseResponse(payload);
    } else {
      response.message = "HTTP Error: " + String(httpCode);
      if (httpCode == 401) {
        _status = NET_CONNECTED; // Token invalid
        response.message = "Auth Failed";
      }
    }
  } else {
    response.message = "Connect Failed: " + http.errorToString(httpCode);
    DEBUG_PRINTLN_NETWORK("Error: " + http.errorToString(httpCode));
  }

  http.end();
  return response;
}

CommandResponse GitMateNetwork::getRepoStatus() {
  return sendCommand("git_status");
}

bool GitMateNetwork::pairWithAgent() {
  if (WiFi.status() != WL_CONNECTED)
    return false;

  HTTPClient http;
  String url = getAgentURL() + "/pair";

  DEBUG_PRINTLN_NETWORK("Pairing request to " + url);

  http.begin(url);
  http.setTimeout(HTTP_TIMEOUT);

  int httpCode = http.GET();
  bool success = false;

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DEBUG_PRINTLN_NETWORK("Pair response: " + payload);

    JsonDocument doc;
    deserializeJson(doc, payload);

    if (doc["status"] == "ok" && doc.containsKey("token")) {
      const char *token = doc["token"];
      setToken(token);
      success = true;
    }
  } else {
    DEBUG_PRINTLN_NETWORK("Pairing failed: " + http.errorToString(httpCode));
  }

  http.end();
  return success;
}
