/*
 * SensorDiag v2 — Raw Register Dump
 * Dumps ALL registers (0x00-0xFF) from every detected I2C device.
 * No sensor-specific assumptions.
 * GPS: Quectel L86 on RX=49, TX=50
 */

#include <Wire.h>

#define I2C_SDA 38
#define I2C_SCL 39
#define GPS_RX 49 // ESP32 receives from GPS TX
#define GPS_TX 50 // ESP32 sends to GPS RX
#define GPS_BAUD 9600

// I2C read helper — returns 0 on error
uint8_t readReg(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0)
    return 0;
  Wire.requestFrom(addr, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

bool writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  uint8_t err = Wire.endTransmission();
  if (err != 0)
    Serial.printf("  WRITE FAIL addr=0x%02X reg=0x%02X val=0x%02X err=%d\n",
                  addr, reg, val, err);
  return err == 0;
}

// Full 256-register dump for one device
void dumpAllRegs(uint8_t addr) {
  Serial.printf("\n=== FULL REGISTER DUMP: 0x%02X ===\n", addr);
  Serial.print("     ");
  for (int c = 0; c < 16; c++)
    Serial.printf(" %02X", c);
  Serial.println();

  for (int row = 0; row < 16; row++) {
    Serial.printf("  %02X:", row * 16);
    for (int col = 0; col < 16; col++) {
      uint8_t reg = row * 16 + col;
      Wire.beginTransmission(addr);
      Wire.write(reg);
      uint8_t err = Wire.endTransmission(false);
      if (err != 0) {
        Serial.print(" --");
      } else {
        Wire.requestFrom(addr, (uint8_t)1);
        if (Wire.available()) {
          Serial.printf(" %02X", Wire.read());
        } else {
          Serial.print(" ??");
        }
      }
    }
    Serial.println();
  }
}

// I2C scan — returns device addresses in array
int scanI2C(const char *label, uint8_t *devices, int maxDevices) {
  Serial.printf("\n=== I2C SCAN: %s ===\n", label);
  int found = 0;
  for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  0x%02X found\n", addr);
      if (found < maxDevices)
        devices[found] = addr;
      found++;
    }
  }
  Serial.printf("  Total: %d device(s)\n", found);
  return found;
}

// GPS streaming
static char gpsLine[256];
static int gpsIdx = 0;

void readGPS() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n' || c == '\r') {
      if (gpsIdx > 3) {
        gpsLine[gpsIdx] = '\0';
        Serial.printf("[GPS] %s\n", gpsLine);
      }
      gpsIdx = 0;
    } else if (gpsIdx < (int)sizeof(gpsLine) - 1) {
      gpsLine[gpsIdx++] = c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n========================================");
  Serial.println("  SENSOR DIAG v2 — RAW REGISTER DUMP");
  Serial.println("========================================\n");

  // I2C at 100kHz for reliability
  Serial.printf("I2C: SDA=%d SCL=%d @ 100kHz\n", I2C_SDA, I2C_SCL);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  delay(100);

  // GPS
  Serial.printf("GPS: RX=%d TX=%d @ %d baud\n", GPS_RX, GPS_TX, GPS_BAUD);
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  // ========== PHASE 1: Scan BEFORE MPU bypass ==========
  uint8_t devsBefore[16];
  int nBefore = scanI2C("BEFORE MPU bypass", devsBefore, 16);

  // Dump ALL registers from every device found
  for (int i = 0; i < nBefore; i++) {
    dumpAllRegs(devsBefore[i]);
  }

  // ========== PHASE 2: Enable MPU9250 bypass ==========
  Serial.println("\n===== ENABLING MPU9250 I2C BYPASS =====");

  // Check if 0x68 is present
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() == 0) {
    uint8_t who = readReg(0x68, 0x75);
    Serial.printf("  0x68 WHO_AM_I: 0x%02X\n", who);

    // Reset
    writeReg(0x68, 0x6B, 0x80);
    delay(100);
    writeReg(0x68, 0x6B, 0x01);
    delay(100);

    // Disable master, enable bypass
    writeReg(0x68, 0x6A, 0x00);
    delay(10);
    writeReg(0x68, 0x37, 0x22);
    delay(50);

    uint8_t intCfg = readReg(0x68, 0x37);
    Serial.printf("  Bypass register: 0x%02X (expect 0x22)\n", intCfg);
  } else {
    Serial.println("  0x68 NOT FOUND — skipping bypass");
  }

  // Re-init I2C after bypass
  Serial.println("  Re-initializing Wire after bypass...");
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  delay(200);

  // ========== PHASE 3: Scan AFTER bypass ==========
  uint8_t devsAfter[16];
  int nAfter = scanI2C("AFTER MPU bypass + Wire.begin()", devsAfter, 16);

  // Dump ALL registers from EVERY device (including new ones)
  for (int i = 0; i < nAfter; i++) {
    dumpAllRegs(devsAfter[i]);
  }

  // ========== PHASE 4: Try writing to 0x0D ==========
  Serial.println("\n===== WRITE TEST: 0x0D =====");
  Wire.beginTransmission(0x0D);
  if (Wire.endTransmission() == 0) {
    // Try writing register 0x09 with different values and read back each time
    uint8_t testVals[] = {0x01, 0x0D, 0x1D, 0xFF, 0x55, 0xAA};
    for (int t = 0; t < 6; t++) {
      bool ok = writeReg(0x0D, 0x09, testVals[t]);
      delay(50);
      uint8_t rb = readReg(0x0D, 0x09);
      Serial.printf("  Write 0x09=0x%02X ok=%d → readback=0x%02X %s\n",
                    testVals[t], ok, rb, (rb == testVals[t]) ? "✓" : "✗");
    }

    // Try writing to every register and check which ones are writable
    Serial.println("\n  Writable register scan (writing 0xAA then readback):");
    for (int reg = 0; reg < 16; reg++) {
      uint8_t orig = readReg(0x0D, reg);
      writeReg(0x0D, reg, 0xAA);
      delay(10);
      uint8_t after = readReg(0x0D, reg);
      writeReg(0x0D, reg, orig); // restore
      delay(10);
      if (after != orig) {
        Serial.printf(
            "  R%02X: orig=%02X → wrote 0xAA → got %02X (WRITABLE!)\n", reg,
            orig, after);
      }
    }
  } else {
    Serial.println("  0x0D NOT FOUND");
  }

  Serial.println("\n========================================");
  Serial.println("  DIAGNOSTIC COMPLETE");
  Serial.println("  Streaming GPS + live sensor data...");
  Serial.println("========================================\n");
}

unsigned long lastPrint = 0;

void loop() {
  readGPS();

  if (millis() - lastPrint > 1000) {
    lastPrint = millis();

    // Read raw data from all known addresses
    // 0x68: accel (0x3B-0x40)
    Wire.beginTransmission(0x68);
    if (Wire.endTransmission() == 0) {
      uint8_t raw[6];
      Wire.beginTransmission(0x68);
      Wire.write(0x3B);
      Wire.endTransmission(false);
      Wire.requestFrom((uint8_t)0x68, (uint8_t)6);
      for (int i = 0; i < 6 && Wire.available(); i++)
        raw[i] = Wire.read();
      int16_t ax = (raw[0] << 8) | raw[1], ay = (raw[2] << 8) | raw[3],
              az = (raw[4] << 8) | raw[5];
      Serial.printf("[0x68] A:%d,%d,%d ", ax, ay, az);
    }

    // 0x0D: mag (0x00-0x05)
    Wire.beginTransmission(0x0D);
    if (Wire.endTransmission() == 0) {
      uint8_t mraw[6];
      Wire.beginTransmission(0x0D);
      Wire.write(0x00);
      Wire.endTransmission(false);
      Wire.requestFrom((uint8_t)0x0D, (uint8_t)6);
      for (int i = 0; i < 6 && Wire.available(); i++)
        mraw[i] = Wire.read();
      int16_t mx = (int16_t)((mraw[1] << 8) | mraw[0]);
      int16_t my = (int16_t)((mraw[3] << 8) | mraw[2]);
      int16_t mz = (int16_t)((mraw[5] << 8) | mraw[4]);
      uint8_t st = readReg(0x0D, 0x06);
      Serial.printf("[0x0D] M:%d,%d,%d st=%02X", mx, my, mz, st);
    }
    Serial.println();
  }
}
