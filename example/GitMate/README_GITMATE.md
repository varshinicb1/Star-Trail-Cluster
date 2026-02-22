# GitMate - Hardware Git Controller

**Transform complex Git operations into intuitive physical interactions**

GitMate is a pocket-sized ESP32-based hardware device that revolutionizes Git workflow management through rotary encoder navigation, touch gestures, and visual LED feedback. Built on the Elecrow 1.28" Round Display, it communicates wirelessly with a desktop agent to execute Git commands safely and efficiently.

![GitMate Device](https://via.placeholder.com/800x400.png?text=GitMate+Hardware+Device)

## ✨ Features

### Hardware Controls
- **Rotary Encoder**: Navigate through 14 Git commands with smooth rotation
- **Click**: Execute selected command
- **Double-Click**: Rollback (undo last commit)
- **Long Press**: Return to main menu

### Touch Gestures
- **Tap**: View diff summary
- **Swipe Up**: Stage all changes (`git add .`)
- **Swipe Down**: Stash changes
- **Swipe Left/Right**: Navigate branches
- **Long Press**: Quick push

### Visual Feedback
- **RGB LED States**: Real-time Git status indication
  - 🔵 Blue breathing: Clean repository
  - 🟡 Yellow pulsing: Uncommitted changes
  - 🔴 Red fast pulse: Merge conflicts
  - ⚪ White flash: Command executing
  - 🟢 Green: Success
  - 🟠 Orange: Warning
  - 🔴 Red solid: Error
  - ⚫ Gray breathing: Connection lost

### Safety Features
- Protected branch detection (main/master/production)
- Confirmation dialogs for dangerous operations
- Rollback support
- Merge conflict detection
- Token-based authentication

## 📋 Requirements

### Hardware
- **Elecrow 1.28" ESP32-S3 Round Display** ([Product Page](https://www.elecrow.com/))
  - ESP32-S3 dual-core processor
  - 240x240 round GC9A01 display
  - Rotary encoder with button
  - CST816D touch controller
  - 5x WS2812 RGB LEDs
  - Wi-Fi/Bluetooth connectivity

### Software
- **Arduino IDE** 2.0+ or **PlatformIO**
- **ESP32 Board Support** (version 2.0.14)
- **Python 3.8+** (for desktop agent)

### Required Libraries
- LVGL (v8.x)
- LovyanGFX
- Adafruit_NeoPixel
- ArduinoJson
- WiFi (ESP32 core)
- HTTPClient (ESP32 core)
- Preferences (ESP32 core)

## 🚀 Quick Start

### 1. Hardware Setup

The Elecrow 1.28" ESP32-S3 board comes pre-configured with the correct pin mappings. All hardware connections are already set up in the firmware based on the official example code.

**Pin Configuration** (auto-configured, no wiring needed):
- Rotary Encoder: GPIO 45 (CLK), GPIO 42 (DT), GPIO 41 (SW)
- RGB LEDs: GPIO 48
- Display: SPI2 (SCLK: 10, MOSI: 11, CS: 9, DC: 3, RST: 14)
- Touch: I2C1 (SDA: 6, SCL: 7, INT: 5, RST: 13)
- Backlight: GPIO 46 (PWM)

### 2. Firmware Installation

#### Option A: Arduino IDE

1. **Install ESP32 Board Support**:
   - Open Arduino IDE
   - Go to File → Preferences
   - Add to Additional Board Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Boards Manager
   - Install "esp32" version 2.0.14

2. **Install Required Libraries**:
   - Go to Sketch → Include Library → Manage Libraries
   - Install: LVGL, LovyanGFX, Adafruit_NeoPixel, ArduinoJson

3. **Copy Library Files**:
   - Copy the `libraries` folder from `example/RotaryScreen_1_28_new/libraries`
   - Paste into your Arduino `libraries` folder

4. **Configure WiFi and Agent**:
   - Open `GitMate/config.h`
   - Update:
     ```cpp
     #define WIFI_SSID "YourWiFiName"
     #define WIFI_PASSWORD "YourWiFiPassword"
     #define AGENT_IP "192.168.1.100"  // Your computer's IP
     ```

5. **Upload Firmware**:
   - Open `GitMate/GitMate.ino`
   - Select Board: "ESP32S3 Dev Module"
   - Select Port: Your ESP32's COM port
   - Click Upload

#### Option B: PlatformIO

1. **Open Project**:
   ```bash
   cd example/GitMate
   pio run --target upload
   ```

2. **Monitor Serial Output**:
   ```bash
   pio device monitor
   ```

### 3. Desktop Agent Setup

1. **Install Python Dependencies**:
   ```bash
   cd example/desktop_agent
   pip install -r requirements.txt
   ```

2. **Start Agent**:
   ```bash
   python gitmate_agent.py --repo /path/to/your/git/repository
   ```

3. **Note the Pairing Token** displayed in the console

4. **Configure Your Computer's IP**:
   - Find your IP address:
     - Windows: `ipconfig`
     - macOS/Linux: `ifconfig` or `ip addr`
   - Update `AGENT_IP` in `GitMate/config.h`
   - Re-upload firmware if needed

### 4. Pairing

1. Power on the GitMate device
2. It will connect to WiFi
3. Device automatically pairs with desktop agent
4. LED turns blue when ready

## 🎮 Usage Guide

### Git Commands (Rotary Navigation)

Rotate the encoder to select commands:

1. **Init Repository** - Initialize new Git repo
2. **Status** - View repo status
3. **Stage All** - Stage all changes (`git add .`)
4. **Commit** - Commit staged changes
5. **Pull** - Pull from remote
6. **Push** - Push to remote (requires confirmation)
7. **Checkout Branch** - Switch branches
8. **Create Branch** - Create new branch
9. **Merge** - Merge branches (requires confirmation)
10. **Stash** - Stash current changes
11. **Stash Pop** - Apply stashed changes
12. **Reset** - Reset to previous commit (dangerous!)
13. **Diff** - View changes summary
14. **Resolve Conflict** - Get conflict information

### Control Reference

| Action | Function |
|--------|----------|
| **Rotate CW/CCW** | Navigate commands |
| **Single Click** | Execute command |
| **Double Click** | Rollback / Cancel |
| **Long Press** | Return to menu |
| **Tap Screen** | Show diff |
| **Swipe Up** | Stage all |
| **Swipe Down** | Stash |
| **Swipe Left** | Previous branch |
| **Swipe Right** | Next branch |
| **Touch Long Press** | Quick push |

### Workflow Example

1. **Make code changes** in your repository
2. **Check status**: Rotate to "Status", click to execute
   - LED turns yellow (uncommitted changes)
   - Display shows branch and file count
3. **Stage changes**: Swipe up on screen
   - All changes staged
4. **Commit**: Rotate to "Commit", click
   - Default message: "GitMate commit"
5. **Push**: Rotate to "Push", click
   - Confirmation required for protected branches
   - LED flashes white during push
   - Green on success

## 🔧 Configuration

### Firmware Configuration (`config.h`)

```cpp
// WiFi Settings
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"

// Desktop Agent
#define AGENT_IP "192.168.1.100"  // Your computer's IP
#define AGENT_PORT 8888

// Timing
#define DEBOUNCE_TIME 20          // Encoder debounce (ms)
#define DOUBLE_CLICK_TIME 300     // Double-click window (ms)
#define LONG_PRESS_TIME 1000      // Long press duration (ms)

// LED
#define LED_BRIGHTNESS 25         // 0-255
```

### Desktop Agent Configuration (`config.json`)

```json
{
  "port": 8888,
  "host": "0.0.0.0",
  "repo_path": "/path/to/repository",
  "protected_branches": ["main", "master", "production"],
  "pairing_token": "auto-generated",
  "debug": false
}
```

## 🛡️ Safety Features

### Protected Branches
- Push operations on main/master/production require double confirmation
- First click: warning dialog
- Second click: execution
- Double-click: cancel

### Dangerous Operations
- **Reset** (hard/soft): Requires confirmation
- **Merge**: Conflict detection before execution
- **Push to protected**: Extra confirmation step

### Fail-Safe Mechanisms
- **Connection monitoring**: Auto-reconnect on WiFi drop
- **Timeout protection**: Commands timeout after 30 seconds
- **Error recovery**: Clear error messages with auto-dismiss
- **Token authentication**: Prevents unauthorized access

## 📁 Project Structure

```
example/
├── GitMate/                      # ESP32 Firmware
│   ├── GitMate.ino              # Main firmware file
│   ├── config.h                  # Hardware & network configuration
│   ├── git_commands.h/cpp        # Git command definitions
│   ├── encoder_handler.h/cpp     # Rotary encoder input
│   ├── touch_handler.h/cpp       # Touch gesture detection
│   ├── led_controller.h/cpp      # RGB LED animations
│   ├── network_client.h/cpp      # WiFi & HTTP communication
│   ├── ui_manager.h/cpp          # LVGL display rendering
│   └── CST816D.h/cpp            # Touch driver (from example)
│
├── desktop_agent/                # Python Desktop Agent
│   ├── gitmate_agent.py         # Flask HTTP server
│   ├── requirements.txt          # Python dependencies
│   ├── config.json              # Agent configuration
│   └── README_AGENT.md          # Agent documentation
│
└── README_GITMATE.md            # This file
```

## 🔍 Troubleshooting

### Device won't connect to WiFi
- **Check credentials**: Verify SSID and password in `config.h`
- **Check signal**: Ensure device is within WiFi range
- **Check compatibility**: 2.4GHz networks only (ESP32 limitation)
- **Serial monitor**: View connection status via USB

### Device can't pair with agent
- **Check IP**: Ensure `AGENT_IP` matches computer's IP
- **Check network**: Both must be on same network
- **Check firewall**: Allow port 8888
- **Restart agent**: Stop and restart desktop agent

### Commands fail to execute
- **Check repository**: Ensure repo path is correct
- **Check Git**: Ensure Git is installed and in PATH
- **Check permissions**: Ensure write access to repo
- **Check status**: View errors on display or serial monitor

### LED shows gray (offline)
- **WiFi disconnected**: Check WiFi connection
- **Agent offline**: Ensure desktop agent is running
- **Network issue**: Check network connectivity

### Display not updating
- **Refresh issue**: Long press to return to menu
- **Firmware issue**: Re-upload firmware
- **Power issue**: Check USB power supply

## 📊 Performance

- **Boot Time**: < 3 seconds
- **UI Refresh Rate**: 20 FPS (50ms)
- **Encoder Response**: < 20ms
- **Command Execution**: Variable (depends on Git operation)
- **Network Latency**: Typically < 100ms on local network

## 🔮 Future Enhancements

Potential features for future versions:

- [ ] Bluetooth LE support (alternative to WiFi)
- [ ] Multi-repository support
- [ ] Custom commit message input via touch keyboard
- [ ] Branch visualization on display
- [ ] Git log viewer
- [ ] Interactive rebase support
- [ ] Webhook integration for CI/CD
- [ ] Battery power option
- [ ] OLED display alternative
- [ ] Mobile app for advanced configuration

## 📝 Development

### Building from Source

1. Clone or download the repository
2. Follow Quick Start guides above
3. Modify source files as needed
4. Upload to device

### Pin Configuration Reference

All pins are extracted from the official example code:

```cpp
// Rotary Encoder
ENCODER_A_PIN  45  // CLK
ENCODER_B_PIN  42  // DT
SWITCH_PIN     41  // Press button

// RGB LED
LED_PIN        48  // NeoPixel data
LED_NUM        5   // Number of LEDs

// Display (SPI)
TFT_SCLK       10
TFT_MOSI       11
TFT_DC         3
TFT_CS         9
TFT_RST        14
BACKLIGHT      46

// Touch (I2C)
TP_SDA         6
TP_SCL         7
TP_INT         5
TP_RST         13
```

### Debug Mode

Enable debug output by uncommenting in `config.h`:

```cpp
#define DEBUG_ENCODER
#define DEBUG_TOUCH
#define DEBUG_NETWORK
#define DEBUG_LED
#define DEBUG_UI
```

View debug output via serial monitor (115200 baud).

## 📄 License

This project is provided as-is for educational and personal use.

## 🙏 Acknowledgments

- **Elecrow** for the excellent ESP32-S3 Round Display hardware
- **LVGL** team for the graphics library
- **LovyanGFX** for the display driver
- **Adafruit** for the NeoPixel library
- **ESP32** community for extensive documentation

## 📞 Support

For issues, questions, or contributions:
- Check troubleshooting section above
- Review desktop agent README (`desktop_agent/README_AGENT.md`)
- Check serial monitor for debug output
- Verify all configuration settings

---

**GitMate - Making Git Physical**

Built with ❤️ for developers who like to touch their commits.
