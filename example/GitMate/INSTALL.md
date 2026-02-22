# GitMate - Quick Installation Guide

## ⚡ Fast Setup (5 Minutes)

### Step 1: Arduino IDE Setup

1. **Install Arduino IDE 2.0+** if you haven't already
   - Download from: https://www.arduino.cc/en/software

2. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to: `File → Preferences`
   - Add this URL to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to: `Tools → Board → Boards Manager`
   - Search for "esp32"
   - Install **"esp32" version 2.0.14** (important: use this specific version)

### Step 2: Configure Your WiFi and Agent

1. **Open `config.h`** in the GitMate folder
2. **Update these three lines** near the top of the file:

```cpp
// Line 28-29: Your WiFi credentials
#define WIFI_SSID "YOUR_WIFI_NAME"        // ← Change this
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // ← Change this

// Line 34: Your computer's IP address
#define AGENT_IP "192.168.1.100"          // ← Change this
```

**To find your computer's IP:**
- **Windows**: Open Command Prompt, type `ipconfig`, look for "IPv4 Address"
- **Mac/Linux**: Open Terminal, type `ifconfig` or `ip addr`

### Step 3: Upload Firmware

1. **Connect your Elecrow ESP32 board** via USB
2. **Open `GitMate.ino`** in Arduino IDE
3. **Select Board**:
   - Go to: `Tools → Board → esp32 → ESP32S3 Dev Module`
4. **Select Port**:
   - Go to: `Tools → Port → COM# (your device)`
5. **Click Upload** (→ button)
   - First compile may take 2-3 minutes
   - Wait for "Done uploading" message

### Step 4: Start Desktop Agent

1. **Open a terminal/command prompt**
2. **Navigate to desktop_agent folder**:
   ```bash
   cd example/desktop_agent
   ```
3. **Install Python dependencies** (one-time only):
   ```bash
   pip install -r requirements.txt
   ```
4. **Start the agent**:
   ```bash
   python gitmate_agent.py --repo C:\path\to\your\git\repository
   ```
   - Replace `C:\path\to\your\git\repository` with your actual Git repo path
   - The agent will show you the pairing token

### Step 5: Power On and Pair

1. **Power on your GitMate device**
2. **Watch the display**:
   - Boot screen appears
   - "Connecting to WiFi..."
   - "Pairing with agent..."
   - "Ready!" (LED turns blue)
3. **If pairing fails**:
   - Check that AGENT_IP matches your computer's IP
   - Ensure both are on the same WiFi network
   - Check that desktop agent is running

## ✅ You're Ready!

Rotate the encoder to select Git commands, click to execute!

## 🔧 Troubleshooting

### "lvgl.h: No such file or directory"
**Solution**: The libraries folder should be automatically included. If not:
- Make sure you're opening `GitMate.ino` (not individual files)
- Restart Arduino IDE
- The `libraries` folder should be inside the `GitMate` folder

### "Board not detected" or "No port available"
**Solution**:
- Install USB drivers for ESP32-S3
- Try different USB cable (some are charge-only)
- Use USB 2.0 port (some USB 3.0 ports have issues)

### WiFi won't connect
**Solution**:
- Double-check SSID and password (case-sensitive)
- ESP32 only supports 2.4GHz WiFi (not 5GHz)
- Try moving closer to WiFi router

### Device pairs but commands fail
**Solution**:
- Check desktop agent console for errors
- Verify repository path is correct
- Ensure Git is installed: `git --version`
- Try a simple command first (Status)

### "Agent offline" (gray LED)
**Solution**:
- Ensure desktop agent is running
- Check firewall isn't blocking port 8888
- Restart both device and agent

## 📚 Full Documentation

For complete documentation, see:
- **Device Documentation**: `README_GITMATE.md`
- **Agent Documentation**: `desktop_agent/README_AGENT.md`
- **Implementation Details**: See artifacts folder for walkthrough

## 🎮 Quick Command Reference

| Action | What it does |
|--------|-------------|
| **Rotate encoder** | Navigate commands (1-14) |
| **Click** | Execute selected command |
| **Double-click** | Rollback / Cancel |
| **Long press** | Return to menu |
| **Swipe up** | Stage all changes |
| **Swipe down** | Stash changes |
| **Tap screen** | Show diff |
| **Touch long press** | Quick push |

## 🚀 First Test Workflow

Try this simple workflow to test everything:

1. **Check Status**: Rotate to "Status", click
   - Should show current branch and file count
   
2. **Make a change**: Edit a file in your repo

3. **Stage changes**: Swipe up on screen
   - Should see "All changes staged"

4. **Commit**: Rotate to "Commit", click
   - Creates commit with default message

5. **Check Status again**: Should show clean repo (blue LED)

Success! Your GitMate is working! 🎉

## ⚙️ Advanced Configuration

### Change LED Brightness
In `config.h`, line 50:
```cpp
#define LED_BRIGHTNESS 25  // 0-255 (25 is default)
```

### Change Agent Port
If port 8888 is in use:
- In `config.h`, line 35: `#define AGENT_PORT 9000`
- Start agent: `python gitmate_agent.py --port 9000 --repo /path`

### Protected Branches
Add/remove protected branches in `desktop_agent/config.json`:
```json
{
  "protected_branches": ["main", "master", "production", "your-branch"]
}
```

### Debug Mode
Uncomment debug flags in `config.h` (lines 115-145) to see detailed serial output.

## 💬 Need Help?

1. Check serial monitor (115200 baud) for debug messages
2. Review full documentation in README files
3. Verify all configuration settings
4. Check desktop agent console for errors

---

**Made with ❤️ for developers who want to touch their commits!**
