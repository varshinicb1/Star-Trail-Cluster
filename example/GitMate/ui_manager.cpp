#include "ui_manager.h"
#include "config.h"
#include <Arduino.h>

// Global UI manager instance
UIManager uiManager;

// Static display reference for LVGL callback
static lgfx::LGFX_Device *globalDisplay = nullptr;

UIManager::UIManager() {
  _display = nullptr;
  _currentScreen = SCREEN_BOOT;
  _buf1 = nullptr;
  _buf2 = nullptr;
  _screenObj = nullptr;
  _commandList = nullptr;
  _messageLabel = nullptr;
  _statusLabel = nullptr;
  _backlightBrightness = 50;
}

void UIManager::begin(lgfx::LGFX_Device *display) {
  _display = display;
  globalDisplay = display;

  // Initialize display
  _display->init();
  _display->initDMA();
  _display->startWrite();
  _display->fillScreen(TFT_BLACK);

  // Initialize LVGL
  lv_init();

  // Allocate buffers
  size_t bufferSize = sizeof(lv_color_t) * SCREEN_WIDTH * SCREEN_HEIGHT;
  _buf1 = (lv_color_t *)heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);
  _buf2 = (lv_color_t *)heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);

  if (!_buf1 || !_buf2) {
    Serial.println("ERROR: Failed to allocate LVGL buffers!");
    return;
  }

  lv_disp_draw_buf_init(&_drawBuf, _buf1, _buf2, SCREEN_WIDTH * SCREEN_HEIGHT);

  // Initialize display driver
  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = SCREEN_WIDTH;
  dispDrv.ver_res = SCREEN_HEIGHT;
  dispDrv.flush_cb = displayFlushCb;
  dispDrv.draw_buf = &_drawBuf;
  lv_disp_drv_register(&dispDrv);

  // Initialize backlight (ESP32 v2.x API)
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(SCREEN_BACKLIGHT_PIN, PWM_CHANNEL);
  setBacklight(50);

  Serial.println("UI Manager initialized");
}

void UIManager::displayFlushCb(lv_disp_drv_t *disp, const lv_area_t *area,
                               lv_color_t *color_p) {
  if (globalDisplay && globalDisplay->getStartCount() > 0) {
    globalDisplay->endWrite();
  }

  if (globalDisplay) {
    globalDisplay->pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1,
                                area->y2 - area->y1 + 1,
                                (lgfx::rgb565_t *)&color_p->full);
  }

  lv_disp_flush_ready(disp);
}

void UIManager::update() { lv_timer_handler(); }

void UIManager::clearScreen() {
  if (_screenObj) {
    lv_obj_del(_screenObj);
    _screenObj = nullptr;
  }
  _commandList = nullptr;
  _messageLabel = nullptr;
  _statusLabel = nullptr;
}

void UIManager::showBootScreen() {
  clearScreen();
  _currentScreen = SCREEN_BOOT;

  _screenObj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(_screenObj, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(_screenObj, lv_color_hex(0x1a1a1a), 0);

  lv_obj_t *label = lv_label_create(_screenObj);
  lv_label_set_text(label, "GitMate");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0x3498db), 0);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

  lv_obj_t *subLabel = lv_label_create(_screenObj);
  lv_label_set_text(subLabel, "Initializing...");
  lv_obj_set_style_text_color(subLabel, lv_color_hex(0xffffff), 0);
  lv_obj_align(subLabel, LV_ALIGN_CENTER, 0, 20);
}

void UIManager::showCommandList(int selectedIndex) {
  clearScreen();
  _currentScreen = SCREEN_COMMAND_LIST;
  createCommandListUI(selectedIndex);
}

void UIManager::createCommandListUI(int selectedIndex) {
  _screenObj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(_screenObj, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(_screenObj, lv_color_hex(0x1a1a1a), 0);

  // Title
  lv_obj_t *title = lv_label_create(_screenObj);
  lv_label_set_text(title, "Git Commands");
  lv_obj_set_style_text_color(title, lv_color_hex(0x3498db), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Command list
  _commandList = lv_obj_create(_screenObj);
  lv_obj_set_size(_commandList, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60);
  lv_obj_align(_commandList, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_color(_commandList, lv_color_hex(0x2a2a2a), 0);
  lv_obj_set_scrollbar_mode(_commandList, LV_SCROLLBAR_MODE_OFF);

  // Add commands (show 3 at a time: previous, current, next)
  int cmdCount = getGitCommandCount();
  int startIdx = max(0, selectedIndex - 1);
  int endIdx = min(cmdCount - 1, selectedIndex + 1);

  int yPos = 10;
  for (int i = startIdx; i <= endIdx; i++) {
    const GitCommand *cmd = getGitCommand(i);

    lv_obj_t *cmdLabel = lv_label_create(_commandList);
    lv_label_set_text(cmdLabel, cmd->name);

    if (i == selectedIndex) {
      // Highlighted command
      lv_obj_set_style_text_color(cmdLabel, lv_color_hex(0x3498db), 0);
      lv_obj_set_style_text_font(cmdLabel, &lv_font_montserrat_20, 0);
      lv_obj_align(cmdLabel, LV_ALIGN_TOP_MID, 0, yPos);
      yPos += 40;
    } else {
      // Non-highlighted commands
      lv_obj_set_style_text_color(cmdLabel, lv_color_hex(0x808080), 0);
      lv_obj_set_style_text_font(cmdLabel, &lv_font_montserrat_14, 0);
      lv_obj_align(cmdLabel, LV_ALIGN_TOP_MID, 0, yPos);
      yPos += 30;
    }
  }
}

void UIManager::showStatus(const char *branch, int uncommitted, int conflicts) {
  clearScreen();
  _currentScreen = SCREEN_STATUS;

  _screenObj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(_screenObj, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(_screenObj, lv_color_hex(0x1a1a1a), 0);

  // Title
  lv_obj_t *title = lv_label_create(_screenObj);
  lv_label_set_text(title, "Repository Status");
  lv_obj_set_style_text_color(title, lv_color_hex(0x3498db), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Branch name
  lv_obj_t *branchLabel = lv_label_create(_screenObj);
  char branchText[64];
  snprintf(branchText, sizeof(branchText), "Branch: %s", branch);
  lv_label_set_text(branchLabel, branchText);
  lv_obj_set_style_text_color(branchLabel, lv_color_hex(0xffffff), 0);
  lv_obj_align(branchLabel, LV_ALIGN_CENTER, 0, -30);

  // Uncommitted files
  lv_obj_t *uncommittedLabel = lv_label_create(_screenObj);
  char uncommittedText[64];
  snprintf(uncommittedText, sizeof(uncommittedText), "Uncommitted: %d",
           uncommitted);
  lv_label_set_text(uncommittedLabel, uncommittedText);
  lv_color_t uncommittedColor =
      uncommitted > 0 ? lv_color_hex(0xf39c12) : lv_color_hex(0x2ecc71);
  lv_obj_set_style_text_color(uncommittedLabel, uncommittedColor, 0);
  lv_obj_align(uncommittedLabel, LV_ALIGN_CENTER, 0, 0);

  // Conflicts
  if (conflicts > 0) {
    lv_obj_t *conflictLabel = lv_label_create(_screenObj);
    char conflictText[64];
    snprintf(conflictText, sizeof(conflictText), "Conflicts: %d", conflicts);
    lv_label_set_text(conflictLabel, conflictText);
    lv_obj_set_style_text_color(conflictLabel, lv_color_hex(0xe74c3c), 0);
    lv_obj_align(conflictLabel, LV_ALIGN_CENTER, 0, 30);
  }
}

void UIManager::showConfirmDialog(const char *message) {
  clearScreen();
  _currentScreen = SCREEN_CONFIRM;
  createMessageUI("Confirm", message, lv_color_hex(0xf39c12));

  lv_obj_t *hint = lv_label_create(_screenObj);
  lv_label_set_text(hint, "Click to confirm\nDouble-click to cancel");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x808080), 0);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void UIManager::showExecuting(const char *command) {
  clearScreen();
  _currentScreen = SCREEN_EXECUTING;
  createMessageUI("Executing", command, lv_color_hex(0x3498db));

  lv_obj_t *spinner = lv_spinner_create(_screenObj, 1000, 60);
  lv_obj_set_size(spinner, 40, 40);
  lv_obj_align(spinner, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void UIManager::showError(const char *message) {
  clearScreen();
  _currentScreen = SCREEN_ERROR;
  createMessageUI("Error", message, lv_color_hex(0xe74c3c));
}

void UIManager::showSuccess(const char *message) {
  clearScreen();
  _currentScreen = SCREEN_SUCCESS;
  createMessageUI("Success", message, lv_color_hex(0x2ecc71));
}

void UIManager::createMessageUI(const char *title, const char *message,
                                lv_color_t color) {
  _screenObj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(_screenObj, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(_screenObj, lv_color_hex(0x1a1a1a), 0);

  // Title
  lv_obj_t *titleLabel = lv_label_create(_screenObj);
  lv_label_set_text(titleLabel, title);
  lv_obj_set_style_text_color(titleLabel, color, 0);
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 20);

  // Message
  _messageLabel = lv_label_create(_screenObj);
  lv_label_set_text(_messageLabel, message);
  lv_label_set_long_mode(_messageLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(_messageLabel, SCREEN_WIDTH - 40);
  lv_obj_set_style_text_color(_messageLabel, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(_messageLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(_messageLabel, LV_ALIGN_CENTER, 0, 0);
}

UIScreen UIManager::getCurrentScreen() { return _currentScreen; }

void UIManager::setBacklight(int brightness) {
  _backlightBrightness = constrain(brightness, 0, 100);
  int pwmValue = (_backlightBrightness * 255) / 100;
  // ESP32 v2.x API: ledcWrite(channel, duty)
  ledcWrite(PWM_CHANNEL, pwmValue);
}
