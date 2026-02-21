#include "ota_update.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

void ota_init() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] WiFi not connected, OTA disabled");
    return;
  }

  ArduinoOTA.setHostname("StarTrail");

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Firmware" : "SPIFFS";
    Serial.printf("[OTA] Start updating %s\n", type.c_str());
  });

  ArduinoOTA.onEnd(
      []() { Serial.println("\n[OTA] Update complete! Rebooting..."); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.printf("[OTA] Ready! Use 'StarTrail.local' or IP %s\n",
                WiFi.localIP().toString().c_str());
}

void ota_handle() { ArduinoOTA.handle(); }
