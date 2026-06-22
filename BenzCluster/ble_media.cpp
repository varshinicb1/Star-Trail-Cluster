#include "ble_media.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

static NimBLEHIDDevice *hid = nullptr;
static NimBLECharacteristic *inputChar = nullptr;
static bool bleConnected = false;

// HID Report Descriptor for Consumer Control (Media keys)
static const uint8_t hidReportDescriptor[] = {
    0x05, 0x0C, // Usage Page (Consumer)
    0x09, 0x01, // Usage (Consumer Control)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x01, //   Report ID (1)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x05, //   Report Count (5)
    0x09, 0xEA, //   Usage (Volume Down)
    0x09, 0xE9, //   Usage (Volume Up)
    0x09, 0xB6, //   Usage (Scan Previous Track)
    0x09, 0xB5, //   Usage (Scan Next Track)
    0x09, 0xCD, //   Usage (Play/Pause)
    0x81, 0x02, //   Input (Data, Variable, Absolute)
    0x95, 0x03, //   Report Count (3) padding bits
    0x81, 0x01, //   Input (Constant)
    0xC0        // End Collection
};

// NimBLE 2.3.7 callback signatures
class BLECallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    bleConnected = true;
    Serial.println("[BLE] *** CONNECTED ***");
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo,
                    int reason) override {
    bleConnected = false;
    Serial.printf("[BLE] Disconnected (reason=%d), restarting ads...\n",
                  reason);
    NimBLEDevice::startAdvertising();
  }

  void onAuthenticationComplete(NimBLEConnInfo &connInfo) override {
    if (!connInfo.isEncrypted()) {
      Serial.println("[BLE] Encrypt fail -> disconnect");
      NimBLEDevice::getServer()->disconnect(connInfo);
      return;
    }
    Serial.println("[BLE] Paired & bonded!");
  }
};

void ble_media_init() {
  Serial.println("[BLE] Init NimBLE HID...");
  NimBLEDevice::init("Star Trail");

  // Security: bonding + MITM + SC, no I/O (just-works pairing)
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new BLECallbacks());

  hid = new NimBLEHIDDevice(pServer);

  hid->setManufacturer("Mercedes-Benz");
  hid->setPnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->setHidInfo(0x00, 0x01);
  hid->setReportMap((uint8_t *)hidReportDescriptor,
                    sizeof(hidReportDescriptor));

  inputChar = hid->getInputReport(1); // Report ID 1

  hid->startServices();

  NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
  pAdv->setAppearance(HID_KEYBOARD);
  pAdv->addServiceUUID(hid->getHidService()->getUUID());
  pAdv->start();

  Serial.println("[BLE] Advertising as 'Star Trail'. Ready to pair!");
}

bool ble_media_connected() { return bleConnected; }

static void sendKey(uint8_t bits) {
  if (!bleConnected || !inputChar)
    return;
  uint8_t report = bits;
  inputChar->setValue(&report, 1);
  inputChar->notify();
  delay(30);
  report = 0;
  inputChar->setValue(&report, 1);
  inputChar->notify();
}

void ble_media_play_pause() {
  Serial.println("[BLE] >> Play/Pause");
  sendKey(0b00010000); // bit 4
}

void ble_media_next() {
  Serial.println("[BLE] >> Next");
  sendKey(0b00001000); // bit 3
}

void ble_media_prev() {
  Serial.println("[BLE] >> Prev");
  sendKey(0b00000100); // bit 2
}

void ble_media_vol_up() {
  sendKey(0b00000010); // bit 1
}

void ble_media_vol_down() {
  sendKey(0b00000001); // bit 0
}
