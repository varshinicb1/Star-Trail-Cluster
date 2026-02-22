/*
 * GitMate - Hardware Git Controller
 *
 * A pocket-sized ESP32-based device that transforms complex Git operations
 * into intuitive physical interactions through rotary encoder, touch gestures,
 * and visual feedback.
 *
 * Hardware: Elecrow 1.28" ESP32-S3 Round Display
 *
 * Features:
 * - Rotary encoder for command navigation
 * - Touch gestures for Git shortcuts
 * - RGB LED state feedback
 * - Round display UI with LVGL
 * - Wi-Fi communication with desktop agent
 * - Safe command execution with confirmations
 */

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>

#include "CST816D.h"
#include "config.h"
#include "encoder_handler.h"
#include "git_commands.h"
#include "led_controller.h"
#include "network_client.h"
#include "touch_handler.h"
#include "ui_manager.h"

// Display configuration class (from example)
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = SPI_FREQUENCY;
      cfg.freq_read = SPI_READ_FREQUENCY;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TFT_SCLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = -1;
      cfg.pin_dc = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = TFT_CS;
      cfg.pin_rst = TFT_RST;
      cfg.pin_busy = -1;
      cfg.memory_width = SCREEN_WIDTH;
      cfg.memory_height = SCREEN_HEIGHT;
      cfg.panel_width = SCREEN_WIDTH;
      cfg.panel_height = SCREEN_HEIGHT;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

// Global instances
LGFX gfx;
CST816D touch(TP_I2C_SDA_PIN, TP_I2C_SCL_PIN, TP_RST, TP_INT);
GitMateNetwork networkClient;

// Application state
enum AppState {
  STATE_INIT,
  STATE_PAIRING,
  STATE_IDLE,
  STATE_NAVIGATING,
  STATE_CONFIRMING,
  STATE_EXECUTING,
  STATE_ERROR,
  STATE_SUCCESS
};

AppState currentState = STATE_INIT;
int selectedCommandIndex = 0;
String pendingCommand = "";
unsigned long stateStartTime = 0;
String lastErrorMessage = "";
String lastSuccessMessage = "";
bool waitingForConfirmation = false;
bool dangerousCommandConfirmed = false;

// Repository state cache
String currentBranch = "unknown";
int uncommittedFiles = 0;
int conflictCount = 0;
unsigned long lastStatusUpdate = 0;

// Function declarations
void setup();
void loop();
void handleEncoderInput();
void handleTouchInput();
void handleState();
void executeGitCommand(int cmdIndex);
void updateRepoStatus();
void processCommandResponse(CommandResponse response);
void transitionToState(AppState newState);

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n\n=== GitMate Hardware Git Controller ===\n");

  // Initialize power pins
  pinMode(POWER_LIGHT_PIN, OUTPUT);
  digitalWrite(POWER_LIGHT_PIN, LOW);
  pinMode(POWER_PIN_1, OUTPUT);
  digitalWrite(POWER_PIN_1, HIGH);
  pinMode(POWER_PIN_2, OUTPUT);
  digitalWrite(POWER_PIN_2, HIGH);

  // Initialize I2C for touch
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize touch controller
  touch.begin();

  // Initialize display
  Serial.println("Initializing display...");
  uiManager.begin(&gfx);
  uiManager.showBootScreen();

  // Initialize LED controller
  Serial.println("Initializing LED...");
  ledController.begin(LED_PIN, LED_NUM, LED_BRIGHTNESS);
  ledController.setState(LED_RUNNING);

  // Initialize encoder
  Serial.println("Initializing encoder...");
  encoder.begin(ENCODER_A_PIN, ENCODER_B_PIN, SWITCH_PIN);

  // Initialize touch handler
  Serial.println("Initializing touch handler...");
  touchHandler.begin(&touch);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  if (networkClient.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("WiFi connected!");

    // Check if we have a pairing token
    if (networkClient.isPaired()) {
      Serial.println("Already paired with agent");
      transitionToState(STATE_IDLE);
    } else {
      Serial.println("Need to pair with agent");
      transitionToState(STATE_PAIRING);
    }
  } else {
    Serial.println("WiFi connection failed!");
    ledController.setState(LED_OFFLINE);
    lastErrorMessage = "WiFi connection failed";
    transitionToState(STATE_ERROR);
  }

  Serial.println("Setup complete!");
}

void loop() {
  // Update all modules
  uiManager.update();
  ledController.update();
  touchHandler.update();

  // Handle inputs
  handleEncoderInput();
  handleTouchInput();

  // Handle current state
  handleState();

  // Update repository status periodically (when idle)
  if (currentState == STATE_IDLE &&
      millis() - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
    updateRepoStatus();
    lastStatusUpdate = millis();
  }

  delay(5);
}

void handleEncoderInput() {
  EncoderEvent event = encoder.getEvent();

  switch (event) {
  case ENCODER_CW:
    if (currentState == STATE_IDLE || currentState == STATE_NAVIGATING) {
      selectedCommandIndex++;
      if (selectedCommandIndex >= getGitCommandCount()) {
        selectedCommandIndex = 0;
      }
      transitionToState(STATE_NAVIGATING);
      uiManager.showCommandList(selectedCommandIndex);
      Serial.print("Selected: ");
      Serial.println(getGitCommandName(selectedCommandIndex));
    }
    break;

  case ENCODER_CCW:
    if (currentState == STATE_IDLE || currentState == STATE_NAVIGATING) {
      selectedCommandIndex--;
      if (selectedCommandIndex < 0) {
        selectedCommandIndex = getGitCommandCount() - 1;
      }
      transitionToState(STATE_NAVIGATING);
      uiManager.showCommandList(selectedCommandIndex);
      Serial.print("Selected: ");
      Serial.println(getGitCommandName(selectedCommandIndex));
    }
    break;

  case ENCODER_CLICK:
    if (currentState == STATE_NAVIGATING || currentState == STATE_IDLE) {
      executeGitCommand(selectedCommandIndex);
    } else if (currentState == STATE_CONFIRMING) {
      if (waitingForConfirmation) {
        // Confirm the command
        Serial.println("Command confirmed");
        if (commandIsDangerous(selectedCommandIndex) &&
            !dangerousCommandConfirmed) {
          dangerousCommandConfirmed = true;
          uiManager.showConfirmDialog("DANGEROUS!\\nClick again to confirm");
        } else {
          transitionToState(STATE_EXECUTING);
          executeGitCommand(selectedCommandIndex);
        }
      }
    }
    break;

  case ENCODER_DOUBLE_CLICK:
    if (currentState == STATE_CONFIRMING) {
      // Cancel confirmation
      Serial.println("Confirmation cancelled");
      transitionToState(STATE_IDLE);
    } else if (currentState == STATE_IDLE || currentState == STATE_NAVIGATING) {
      // Rollback - undo last commit
      Serial.println("Rollback requested");
      JsonDocument args;
      args["soft"] = true;
      CommandResponse response =
          networkClient.sendCommand("git_rollback", &args);
      processCommandResponse(response);
    }
    break;

  case ENCODER_LONG_PRESS:
    // Return to command list from any screen
    if (currentState != STATE_EXECUTING) {
      transitionToState(STATE_IDLE);
    }
    break;

  default:
    break;
  }
}

void handleTouchInput() {
  TouchGesture gesture = touchHandler.getGesture();

  if (gesture == TOUCH_NONE)
    return;

  Serial.print("Touch gesture: ");
  Serial.println((int)gesture);

  switch (gesture) {
  case TOUCH_TAP:
    // Show diff summary
    if (currentState == STATE_IDLE || currentState == STATE_NAVIGATING) {
      Serial.println("Tap: Show diff");
      CommandResponse response = networkClient.sendCommand("git_diff");
      if (response.success) {
        String diffMsg = response.message;
        if (diffMsg.length() > 100) {
          diffMsg = diffMsg.substring(0, 97) + "...";
        }
        uiManager.showStatus(currentBranch.c_str(), uncommittedFiles,
                             conflictCount);
      }
    }
    break;

  case TOUCH_SWIPE_UP:
    // Stage all changes
    Serial.println("Swipe up: Stage all");
    transitionToState(STATE_EXECUTING);
    {
      CommandResponse response = networkClient.sendCommand("git_add");
      processCommandResponse(response);
    }
    break;

  case TOUCH_SWIPE_DOWN:
    // Stash changes
    Serial.println("Swipe down: Stash");
    transitionToState(STATE_EXECUTING);
    {
      CommandResponse response = networkClient.sendCommand("git_stash");
      processCommandResponse(response);
    }
    break;

  case TOUCH_SWIPE_LEFT:
    // Checkout previous branch (if available)
    Serial.println("Swipe left: Previous branch");
    // TODO: Implement branch history
    break;

  case TOUCH_SWIPE_RIGHT:
    // Checkout next branch (if available)
    Serial.println("Swipe right: Next branch");
    // TODO: Implement branch history
    break;

  case TOUCH_LONG_PRESS:
    // Push
    Serial.println("Long press: Push");
    executeGitCommand(CMD_PUSH);
    break;

  default:
    break;
  }
}

void handleState() {
  unsigned long stateTime = millis() - stateStartTime;

  switch (currentState) {
  case STATE_INIT:
    // Initialization state - handled in setup
    break;

  case STATE_PAIRING:
    // Try to pair with agent
    if (stateTime > 2000) { // Wait 2 seconds before pairing
      Serial.println("Attempting to pair with agent...");
      if (networkClient.pairWithAgent()) {
        Serial.println("Pairing successful!");
        transitionToState(STATE_IDLE);
      } else {
        Serial.println("Pairing failed, will retry...");
        stateStartTime = millis(); // Reset timer to retry
      }
    }
    break;

  case STATE_IDLE:
    // Show command list
    if (uiManager.getCurrentScreen() != SCREEN_COMMAND_LIST) {
      uiManager.showCommandList(selectedCommandIndex);
    }

    // Update LED based on repo state
    if (conflictCount > 0) {
      ledController.setState(LED_CONFLICT);
    } else if (uncommittedFiles > 0) {
      ledController.setState(LED_UNCOMMITTED);
    } else {
      ledController.setState(LED_CLEAN);
    }
    break;

  case STATE_NAVIGATING:
    // User is navigating through commands
    // Auto-return to idle after 3 seconds of inactivity
    if (stateTime > 3000) {
      transitionToState(STATE_IDLE);
    }
    break;

  case STATE_CONFIRMING:
    // Waiting for user confirmation
    // Auto-cancel after 10 seconds
    if (stateTime > 10000) {
      Serial.println("Confirmation timeout");
      transitionToState(STATE_IDLE);
    }
    break;

  case STATE_EXECUTING:
    // Command is executing - UI already updated
    break;

  case STATE_ERROR:
    // Show error for 5 seconds, then return to idle
    if (stateTime > 5000) {
      transitionToState(STATE_IDLE);
    }
    break;

  case STATE_SUCCESS:
    // Show success for 3 seconds, then return to idle
    if (stateTime > 3000) {
      transitionToState(STATE_IDLE);
    }
    break;
  }
}

void executeGitCommand(int cmdIndex) {
  const GitCommand *cmd = getGitCommand(cmdIndex);
  if (!cmd)
    return;

  Serial.print("Executing: ");
  Serial.println(cmd->name);

  // Check if command requires confirmation
  if (cmd->requiresConfirm && !waitingForConfirmation) {
    waitingForConfirmation = true;
    dangerousCommandConfirmed = false;
    transitionToState(STATE_CONFIRMING);
    uiManager.showConfirmDialog(cmd->name);
    return;
  }

  // Reset confirmation flags
  waitingForConfirmation = false;
  dangerousCommandConfirmed = false;

  // Show executing screen
  transitionToState(STATE_EXECUTING);
  uiManager.showExecuting(cmd->name);
  ledController.setState(LED_RUNNING);

  // Send command to agent
  CommandResponse response = networkClient.sendCommand(cmd->cmd);
  processCommandResponse(response);
}

void processCommandResponse(CommandResponse response) {
  if (response.success) {
    Serial.println("Command succeeded!");
    lastSuccessMessage = response.message;
    transitionToState(STATE_SUCCESS);
    uiManager.showSuccess(response.message.c_str());
    ledController.setState(LED_SUCCESS);

    // Update repository status
    updateRepoStatus();
  } else {
    Serial.print("Command failed: ");
    Serial.println(response.message);
    lastErrorMessage = response.message;
    transitionToState(STATE_ERROR);
    uiManager.showError(response.message.c_str());
    ledController.setState(LED_ERROR);
  }
}

void updateRepoStatus() {
  CommandResponse response = networkClient.getRepoStatus();

  if (response.success && response.payload["branch"].is<String>()) {
    currentBranch = response.payload["branch"].as<String>();
    uncommittedFiles = response.payload["uncommitted"].as<int>();
    conflictCount = response.payload["conflicts"].as<int>();

    Serial.print("Status update - Branch: ");
    Serial.print(currentBranch);
    Serial.print(", Uncommitted: ");
    Serial.print(uncommittedFiles);
    Serial.print(", Conflicts: ");
    Serial.println(conflictCount);
  }
}

void transitionToState(AppState newState) {
  if (newState != currentState) {
    Serial.print("State transition: ");
    Serial.print(currentState);
    Serial.print(" -> ");
    Serial.println(newState);

    currentState = newState;
    stateStartTime = millis();
  }
}
