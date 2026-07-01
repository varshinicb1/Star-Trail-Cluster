#ifndef BLE_NOTIFY_H
#define BLE_NOTIFY_H

#include <Arduino.h>

void ble_notify_init();
bool ble_notify_has_message();
const char *ble_notify_get_message();
void ble_notify_clear();
int ble_notify_get_phone_battery();
void ble_data_notify(const char *json);

// Custom widget layout push (chunked). The app streams a JSON layout via the
// custom-layout characteristic using 1-byte opcodes: 'B' begin, 'C' chunk,
// 'F' finish. On finish the assembled document is held pending until the main
// loop consumes it. Returns true (and fills `out`) exactly once per completed
// transfer; the caller then parses/saves/reloads.
bool ble_custom_layout_take(String &out);

#endif
