#include "ble_notify.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

// Custom GATT Service for receiving notifications from phone
// UUID: deadbeef-cafe-1234-5678-abcdef012345
#define NOTIFY_SERVICE_UUID "deadbeef-cafe-1234-5678-abcdef012345"
#define NOTIFY_CHAR_UUID "deadbeef-cafe-1234-5678-abcdef012346"
#define BATTERY_SERVICE_UUID "180F"
#define BATTERY_CHAR_UUID "2A19"

static char lastMessage[64] = {0};
static bool hasNewMessage = false;
static int phoneBattery = -1; // -1 = unknown

// Callback for receiving notifications from phone
class NotifyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override {
    std::string val = pChar->getValue();
    if (val.length() > 0) {
      strncpy(lastMessage, val.c_str(), sizeof(lastMessage) - 1);
      lastMessage[sizeof(lastMessage) - 1] = '\0';
      hasNewMessage = true;
      Serial.printf("[BLE-N] Received: %s\n", lastMessage);
    }
  }
};

class BatteryCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override {
    std::string val = pChar->getValue();
    if (val.length() >= 1) {
      phoneBattery = (uint8_t)val[0];
      Serial.printf("[BLE-N] Phone battery: %d%%\n", phoneBattery);
    }
  }
};

void ble_notify_init() {
  // Add notification service to existing NimBLE server
  NimBLEServer *pServer = NimBLEDevice::getServer();
  if (!pServer) {
    Serial.println("[BLE-N] No BLE server, skipping notify init");
    return;
  }

  // Notification service — phone writes text here
  NimBLEService *notifySvc = pServer->createService(NOTIFY_SERVICE_UUID);
  NimBLECharacteristic *notifyChar = notifySvc->createCharacteristic(
      NOTIFY_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  notifyChar->setCallbacks(new NotifyCallbacks());
  notifySvc->start();

  // Battery service — phone writes battery % here
  NimBLEService *batSvc = pServer->createService(BATTERY_SERVICE_UUID);
  NimBLECharacteristic *batChar = batSvc->createCharacteristic(
      BATTERY_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  batChar->setCallbacks(new BatteryCallbacks());
  batSvc->start();

  // Re-start advertising with new services
  NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
  pAdv->addServiceUUID(NOTIFY_SERVICE_UUID);
  pAdv->start();

  Serial.println("[BLE-N] Notification + Battery services started");
}

bool ble_notify_has_message() { return hasNewMessage; }

const char *ble_notify_get_message() { return lastMessage; }

void ble_notify_clear() {
  hasNewMessage = false;
  lastMessage[0] = '\0';
}

int ble_notify_get_phone_battery() { return phoneBattery; }
