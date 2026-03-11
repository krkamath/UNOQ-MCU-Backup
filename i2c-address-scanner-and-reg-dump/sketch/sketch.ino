#include <Wire.h>
#include <Arduino_RouterBridge.h>

struct ProbeReg {
  uint8_t reg;
  const char* name;
};

ProbeReg commonRegs[] = {
  {0x00, "REG00"},
  {0x01, "REG01"},
  {0x02, "REG02"},
  {0x03, "REG03"},
  {0x04, "REG04"},
  {0x05, "REG05"},
  {0x06, "REG06"},
  {0x07, "REG07"},
  {0x08, "REG08"},
  {0x09, "REG09"},
  {0x0A, "REG0A"},
  {0x0B, "REG0B"},
  {0x0C, "REG0C"},
  {0x0D, "REG0D"},
  {0x0E, "REG0E"},
  {0x0F, "WHO_AM_I/ID"},
  {0x10, "REG10"},
  {0x20, "REG20"},
  {0x2A, "REG2A"},
  {0x42, "REG42"},
  {0x47, "REG47"},
  {0x4F, "REG4F"},
  {0x75, "WHO_AM_I"},
  {0x76, "REG76"},
  {0x77, "REG77"},
  {0x7F, "REG7F"},
  {0x8F, "ID/STATUS"},
  {0x92, "ID"},
  {0xA0, "REGA0"},
  {0xD0, "CHIP_ID"},
  {0xD1, "REGD1"},
  {0xE0, "REGE0"},
  {0xEA, "WHO_AM_I"},
  {0xF0, "REGF0"},
  {0xFC, "REGFC"},
  {0xFD, "REGFD"},
  {0xFE, "DEVICE_ID"},
  {0xFF, "REGFF"}
};

bool pingAddress(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

bool readReg8(uint8_t addr, uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  uint8_t n = Wire.requestFrom((int)addr, 1);
  if (n != 1) return false;

  value = Wire.read();
  return true;
}

void printHex2(uint8_t v) {
  if (v < 16) Monitor.print("0");
  Monitor.print(v, HEX);
}

void printKnownHint(uint8_t addr) {
  Monitor.print("Hint: ");
  switch (addr) {
    case 0x3C:
    case 0x3D:
      Monitor.println("Possible SSD1306/SH1106 OLED");
      break;
    case 0x60:
    case 0x61:
      Monitor.println("Possible Si5351 / clock chip family");
      break;
    case 0x68:
      Monitor.println("Possible DS3231 / MPU6050 / RTC-IMU family");
      break;
    case 0x76:
    case 0x77:
      Monitor.println("Possible BME/BMP280/BMP388 family");
      break;
    case 0x27:
    case 0x3F:
      Monitor.println("Possible PCF8574 LCD backpack");
      break;
    case 0x48:
    case 0x49:
      Monitor.println("Possible ADC / temp sensor family");
      break;
    default:
      Monitor.println("No address hint");
      break;
  }
}

void dumpSelectedRegs(uint8_t addr) {
  Monitor.println("Common register probes:");
  for (unsigned int i = 0; i < sizeof(commonRegs) / sizeof(commonRegs[0]); i++) {
    uint8_t val;
    if (readReg8(addr, commonRegs[i].reg, val)) {
      Monitor.print("  ");
      Monitor.print(commonRegs[i].name);
      Monitor.print(" @0x");
      printHex2(commonRegs[i].reg);
      Monitor.print(" = 0x");
      printHex2(val);
      Monitor.println();
    }
  }
}

void dumpRange(uint8_t addr, uint8_t startReg, uint8_t endReg) {
  Monitor.print("Register dump 0x");
  printHex2(startReg);
  Monitor.print("..0x");
  printHex2(endReg);
  Monitor.println(":");

  for (uint8_t reg = startReg; reg <= endReg; reg++) {
    uint8_t val;
    if ((reg - startReg) % 8 == 0) {
      Monitor.print("  ");
      printHex2(reg);
      Monitor.print(": ");
    }

    if (readReg8(addr, reg, val)) {
      printHex2(val);
      Monitor.print(" ");
    } else {
      Monitor.print("-- ");
    }

    if ((reg - startReg) % 8 == 7) {
      Monitor.println();
    }

    if (reg == 0xFF) break;
  }
  Monitor.println();
}

void analyzeDevice(uint8_t addr) {
  Monitor.println();
  Monitor.print("Device at 0x");
  printHex2(addr);
  Monitor.println();

  printKnownHint(addr);

  dumpSelectedRegs(addr);

  if (addr == 0x60 || addr == 0x61) {
    Monitor.println("Si5351-focused registers:");
    uint8_t v;
    if (readReg8(addr, 0x00, v)) {
      Monitor.print("  Status reg 0x00 = 0x");
      printHex2(v);
      Monitor.println();
    }
    if (readReg8(addr, 0x01, v)) {
      Monitor.print("  Interrupt sticky reg 0x01 = 0x");
      printHex2(v);
      Monitor.println();
    }
    if (readReg8(addr, 0x03, v)) {
      Monitor.print("  Output enable reg 0x03 = 0x");
      printHex2(v);
      Monitor.println();
    }
  }

  dumpRange(addr, 0x00, 0x1F);
}

void setup() {
  Wire.begin();
  Monitor.begin();
  delay(1000);

  Monitor.println();
  Monitor.println("Generic I2C Detective");
  Monitor.println("Scanning bus...");
}

void loop() {
  int found = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    if (pingAddress(addr)) {
      found++;
      analyzeDevice(addr);
    }
  }

  if (found == 0) {
    Monitor.println("No I2C devices found");
  } else {
    Monitor.print("Total devices found: ");
    Monitor.println(found);
  }

  Monitor.println();
  Monitor.println("Rescanning in 10 seconds...");
  Monitor.println();
  delay(10000);
}
