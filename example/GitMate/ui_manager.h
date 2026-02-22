#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "git_commands.h"
#include <LovyanGFX.hpp>
#include <lvgl.h>


// UI screen enum
enum UIScreen {
  SCREEN_BOOT,
  SCREEN_COMMAND_LIST,
  SCREEN_STATUS,
  SCREEN_CONFIRM,
  SCREEN_EXECUTING,
  SCREEN_ERROR,
  SCREEN_SUCCESS
};

// UI manager class
class UIManager {
public:
  UIManager();

  // Initialize display and LVGL
  void begin(lgfx::LGFX_Device *display);

  // Update UI (call in loop)
  void update();

  // Show specific screen
  void showBootScreen();
  void showCommandList(int selectedIndex);
  void showStatus(const char *branch, int uncommitted, int conflicts);
  void showConfirmDialog(const char *message);
  void showExecuting(const char *command);
  void showError(const char *message);
  void showSuccess(const char *message);

  // Get current screen
  UIScreen getCurrentScreen();

  // Set backlight brightness (0-100)
  void setBacklight(int brightness);

private:
  lgfx::LGFX_Device *_display;
  UIScreen _currentScreen;

  lv_disp_draw_buf_t _drawBuf;
  lv_color_t *_buf1;
  lv_color_t *_buf2;

  lv_obj_t *_screenObj;
  lv_obj_t *_commandList;
  lv_obj_t *_messageLabel;
  lv_obj_t *_statusLabel;

  int _backlightBrightness;

  // LVGL callbacks
  static void displayFlushCb(lv_disp_drv_t *disp, const lv_area_t *area,
                             lv_color_t *color_p);

  // Helper functions
  void createCommandListUI(int selectedIndex);
  void createMessageUI(const char *title, const char *message,
                       lv_color_t color);
  void clearScreen();
};

// Global UI manager instance
extern UIManager uiManager;

#endif // UI_MANAGER_H
