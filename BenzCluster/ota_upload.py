"""
WiFi OTA Upload Script for Star Trail Gadget
Uploads compiled firmware binary to the ESP32-S3 over WiFi.

Usage:
  python ota_upload.py [IP_ADDRESS]
  
  Default IP: 192.168.4.1 (AP mode) or auto-detect via mDNS
  The ESP32 web server accepts POST /update with the firmware binary.
"""
import sys
import os
import glob
import requests
import time

# Default device IP (change to your device's IP on your network)
DEFAULT_IP = "10.91.5.73"

def find_firmware_binary():
    """Find the compiled .bin file in the Arduino build output."""
    # Arduino CLI places the binary in a build directory
    sketch_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Check common Arduino CLI build output locations
    patterns = [
        os.path.join(sketch_dir, "build", "**", "*.ino.bin"),
        os.path.join(os.environ.get("LOCALAPPDATA", ""), "Temp", "arduino", "sketches", "**", "*.ino.bin"),
    ]
    
    for pattern in patterns:
        matches = glob.glob(pattern, recursive=True)
        if matches:
            # Return the most recently modified one
            return max(matches, key=os.path.getmtime)
    
    return None

def upload_ota(ip, bin_path):
    """Upload firmware binary via HTTP POST to /update endpoint."""
    url = f"http://{ip}/update"
    file_size = os.path.getsize(bin_path)
    
    print(f"[OTA] Target: {url}")
    print(f"[OTA] Firmware: {bin_path}")
    print(f"[OTA] Size: {file_size / 1024:.1f} KB")
    print(f"[OTA] Uploading...")
    
    start = time.time()
    with open(bin_path, "rb") as f:
        files = {"update": (os.path.basename(bin_path), f, "application/octet-stream")}
        try:
            resp = requests.post(url, files=files, timeout=120)
            elapsed = time.time() - start
            
            if resp.status_code == 200:
                print(f"[OTA] Success! ({elapsed:.1f}s, {file_size/elapsed/1024:.1f} KB/s)")
                print(f"[OTA] Device will reboot in 3 seconds...")
                return True
            else:
                print(f"[OTA] FAILED: HTTP {resp.status_code} - {resp.text}")
                return False
        except requests.exceptions.ConnectionError:
            print(f"[OTA] Cannot connect to {ip}. Is the device on your network?")
            return False
        except requests.exceptions.Timeout:
            print(f"[OTA] Upload timed out.")
            return False

if __name__ == "__main__":
    ip = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_IP
    
    # Find firmware binary
    bin_path = find_firmware_binary()
    if not bin_path:
        print("[OTA] ERROR: Cannot find compiled firmware binary.")
        print("[OTA] Run 'arduino-cli compile' first.")
        sys.exit(1)
    
    if not upload_ota(ip, bin_path):
        sys.exit(1)
