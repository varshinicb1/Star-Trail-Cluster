#ifndef CUSTOM_SCREEN_H
#define CUSTOM_SCREEN_H

// Screen wrapper that hosts a user-designed custom widget layout (see
// custom_widget.h). Owns an LVGL container, loads the active layout from
// SPIFFS, renders it, and refreshes bound values each frame. New layouts
// arriving over BLE/WiFi call custom_screen_reload() to re-render live.

void custom_screen_init();
void custom_screen_show();
void custom_screen_hide();
void custom_screen_update();

// Re-load the layout from SPIFFS and re-render (call after a push lands).
void custom_screen_reload();

#endif // CUSTOM_SCREEN_H
