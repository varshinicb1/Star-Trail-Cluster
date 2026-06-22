#ifndef BLE_NOTIFY_H
#define BLE_NOTIFY_H

#include <Arduino.h>

void ble_notify_init();
bool ble_notify_has_message();
const char *ble_notify_get_message();
void ble_notify_clear();
int ble_notify_get_phone_battery();
void ble_data_notify(const char *json);

#endif
