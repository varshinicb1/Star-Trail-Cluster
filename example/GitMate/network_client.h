#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>


// Network status enum
enum NetworkStatus {
  NET_DISCONNECTED,
  NET_CONNECTING,
  NET_CONNECTED,
  NET_PAIRING,
  NET_PAIRED,
  NET_ERROR
};

// Command response structure
struct CommandResponse {
  bool success;
  String status; // "ok", "error", "confirm"
  String message;
  JsonDocument payload; // DynamicJsonDocument for additional data
};

// Network client class
class GitMateNetwork {
public:
  GitMateNetwork();

  // Initialize and connect to WiFi
  bool begin(const char *ssid, const char *password);

  // Check connection status
  NetworkStatus getStatus();

  // Reconnect to WiFi
  bool reconnect();

  // Send Git command to desktop agent
  CommandResponse sendCommand(const char *cmd, JsonDocument *args = nullptr);

  // Get repository status
  CommandResponse getRepoStatus();

  // Pair with desktop agent (get token)
  bool pairWithAgent();

  // Set agent IP and port
  void setAgentAddress(const char *ip, int port);

  // Set pairing token
  void setToken(const char *token);

  // Get pairing token
  String getToken();

  // Check if paired
  bool isPaired();

  // Get full agent URL
  String getAgentURL();

private:
  String _ssid;
  String _password;
  String _agentIP;
  int _agentPort;
  String _token;
  NetworkStatus _status;

  // Build request JSON
  String buildRequest(const char *cmd, JsonDocument *args);

  // Parse response JSON
  CommandResponse parseResponse(String json);
};

// Global network client instance
extern GitMateNetwork networkClient;

#endif // NETWORK_CLIENT_H
