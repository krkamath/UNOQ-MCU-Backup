#include <Wire.h>
#include <Arduino_RouterBridge.h>

struct Reg8Probe {
  uint8_t reg;
  const char* name;
};

Reg8Probe reg8List[] = {
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
  {0xD0, "CHIP_ID"},
  {0xD1, "REGD1"},
  {0xE0, "REGE0"},
  {0xEA, "WHO_AM_I"},
  {0xFC, "REGFC"},
  {0xFD, "REGFD"},
  {0xFE, "DEVICE_ID"},
  {0xFF, "REGFF"}
};

struct Reg16Probe {
  uint8_t reg;
  const char* name;
};

Reg16Probe reg16List[] = {
  {0x00, "REG16_00"},
  {0x01, "REG16_01"},
  {0x06, "MANUF_ID?"},
  {0x07, "DEVICE_ID?"},
  {0x08, "REG16_08"},
  {0x0B, "REG16_0B"},
  {0x0C, "REG16_0C"},
  {0xFE, "REG16_FE"}
};

void printHex2(uint8_t v) {
  if (v < 16) Monitor.print("0");
  Monitor.print(v, HEX);
}

void printHex4(uint16_t v) {
  if (v < 0x1000) Monitor.print("0");
  if (v < 0x0100) Monitor.print("0");
  if (v < 0x0010) Monitor.print("0");
  Monitor.print(v, HEX);
}

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

bool readReg16BE(uint8_t addr, uint8_t reg, uint16_t &value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  uint8_t n = Wire.requestFrom((int)addr, 2);
  if (n != 2) return false;

  uint8_t hi = Wire.read();
  uint8_t lo = Wire.read();
  value = ((uint16_t)hi << 8) | lo;
  return true;
}

bool readReg16LE(uint8_t addr, uint8_t reg, uint16_t &value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  uint8_t n = Wire.requestFrom((int)addr, 2);
  if (n != 2) return false;

  uint8_t lo = Wire.read();
  uint8_t hi = Wire.read();
  value = ((uint16_t)hi << 8) | lo;
  return true;
}

void printDivider() {
  Monitor.println("--------------------------------------------------");
}

void printAddressHeader(uint8_t addr) {
  printDivider();
  Monitor.print("Device found at 0x");
  printHex2(addr);
  Monitor.println();
}

void printAddressHints(uint8_t addr) {
  Monitor.print("Address hints: ");

  bool any = false;

  if (addr == 0x3C || addr == 0x3D) {
    Monitor.print("OLED SSD1306/SH1106 ");
    any = true;
  }
  if (addr == 0x27 || addr == 0x3F) {
    Monitor.print("PCF8574 LCD backpack ");
    any = true;
  }
  if (addr == 0x60 || addr == 0x61) {
    Monitor.print("Si5351 / clock-generator family ");
    any = true;
  }
  if (addr == 0x68 || addr == 0x69) {
    Monitor.print("RTC / IMU family ");
    any = true;
  }
  if (addr == 0x76 || addr == 0x77) {
    Monitor.print("Bosch environmental sensor family ");
    any = true;
  }
  if (addr >= 0x18 && addr <= 0x1F) {
    Monitor.print("MCP9808-family address range ");
    any = true;
  }

  if (!any) Monitor.print("no strong address hint");
  Monitor.println();
}

void strongIdentify(uint8_t addr) {
  uint8_t v8;
  uint16_t v16be, v16le;

  if ((addr == 0x76 || addr == 0x77) && readReg8(addr, 0xD0, v8)) {
    if (v8 == 0x58) {
      Monitor.println("Strong match: BMP280");
      return;
    }
    if (v8 == 0x60) {
      Monitor.println("Strong match: BME280");
      return;
    }
    if (v8 == 0x61) {
      Monitor.println("Strong match: BME680");
      return;
    }
  }

  if ((addr == 0x68 || addr == 0x69) && readReg8(addr, 0x75, v8)) {
    if (v8 == 0x68) {
      Monitor.println("Strong match: MPU6050-like IMU");
      return;
    }
    if (v8 == 0x98) {
      Monitor.println("Strong match: ICM-20689-like IMU");
      return;
    }
  }

  if ((addr >= 0x18 && addr <= 0x1F)) {
    bool gotManuf = readReg16BE(addr, 0x06, v16be);
    bool gotDevLE = readReg16LE(addr, 0x07, v16le);

    if (gotManuf && gotDevLE) {
      if (v16be == 0x0054 && (v16le & 0xFF00) == 0x0400) {
        Monitor.println("Strong match: MCP9808");
        return;
      }
    }
  }

  if (addr == 0x60 || addr == 0x61) {
    uint8_t a, b, c;
    if (readReg8(addr, 0x00, a) && readReg8(addr, 0x01, b) && readReg8(addr, 0x03, c)) {
      Monitor.println("Likely match: Si5351 clock generator");
      return;
    }
  }

  if (addr == 0x3C || addr == 0x3D) {
    Monitor.println("Likely match: SSD1306 / SH1106 OLED");
    return;
  }

  if (addr == 0x27 || addr == 0x3F) {
    Monitor.println("Likely match: PCF8574 LCD backpack");
    return;
  }

  Monitor.println("Match: Unknown / generic I2C device");
}

void printQuickReg8(uint8_t addr) {
  Monitor.println("Common 8-bit probes:");
  for (unsigned int i = 0; i < sizeof(reg8List) / sizeof(reg8List[0]); i++) {
    uint8_t val;
    Monitor.print("  ");
    Monitor.print(reg8List[i].name);
    Monitor.print(" @0x");
    printHex2(reg8List[i].reg);
    Monitor.print(" = ");

    if (readReg8(addr, reg8List[i].reg, val)) {
      Monitor.print("0x");
      printHex2(val);
    } else {
      Monitor.print("--");
    }
    Monitor.println();
  }
}

void printQuickReg16(uint8_t addr) {
  Monitor.println("Common 16-bit probes:");
  for (unsigned int i = 0; i < sizeof(reg16List) / sizeof(reg16List[0]); i++) {
    uint16_t beVal, leVal;
    bool okBE = readReg16BE(addr, reg16List[i].reg, beVal);
    bool okLE = readReg16LE(addr, reg16List[i].reg, leVal);

    Monitor.print("  ");
    Monitor.print(reg16List[i].name);
    Monitor.print(" @0x");
    printHex2(reg16List[i].reg);
    Monitor.print(" = ");

    if (okBE) {
      Monitor.print("BE:0x");
      printHex4(beVal);
      Monitor.print(" ");
    } else {
      Monitor.print("BE:---- ");
    }

    if (okLE) {
      Monitor.print("LE:0x");
      printHex4(leVal);
    } else {
      Monitor.print("LE:----");
    }

    Monitor.println();
  }
}

void dumpBlock8(uint8_t addr, uint8_t startReg, uint8_t endReg) {
  Monitor.print("8-bit dump 0x");
  printHex2(startReg);
  Monitor.print("..0x");
  printHex2(endReg);
  Monitor.println(":");

  for (uint8_t reg = startReg; reg <= endReg; reg++) {
    if ((reg - startReg) % 8 == 0) {
      Monitor.print("  ");
      printHex2(reg);
      Monitor.print(": ");
    }

    uint8_t val;
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

void si5351Details(uint8_t addr) {
  if (!(addr == 0x60 || addr == 0x61)) return;

  uint8_t s0, s1, s3;
  bool ok0 = readReg8(addr, 0x00, s0);
  bool ok1 = readReg8(addr, 0x01, s1);
  bool ok3 = readReg8(addr, 0x03, s3);

  if (!(ok0 || ok1 || ok3)) return;

  Monitor.println("Si5351-focused summary:");
  if (ok0) {
    Monitor.print("  Reg 0x00 status = 0x");
    printHex2(s0);
    Monitor.println();
  }
  if (ok1) {
    Monitor.print("  Reg 0x01 interrupt sticky = 0x");
    printHex2(s1);
    Monitor.println();
  }
  if (ok3) {
    Monitor.print("  Reg 0x03 output enable = 0x");
    printHex2(s3);
    Monitor.println();
  }
}

void mcp9808Details(uint8_t addr) {
  if (!(addr >= 0x18 && addr <= 0x1F)) return;

  uint16_t manufBE, devLE;
  bool okM = readReg16BE(addr, 0x06, manufBE);
  bool okD = readReg16LE(addr, 0x07, devLE);

  if (!(okM || okD)) return;

  Monitor.println("MCP9808-style summary:");
  if (okM) {
    Monitor.print("  Manufacturer ID = 0x");
    printHex4(manufBE);
    Monitor.println();
  }
  if (okD) {
    Monitor.print("  Device ID/Revision = 0x");
    printHex4(devLE);
    Monitor.println();
  }
}

void boschDetails(uint8_t addr) {
  if (!(addr == 0x76 || addr == 0x77)) return;

  uint8_t chip;
  if (!readReg8(addr, 0xD0, chip)) return;

  Monitor.println("Bosch environmental summary:");
  Monitor.print("  CHIP_ID @0xD0 = 0x");
  printHex2(chip);
  Monitor.println();
}

void imuDetails(uint8_t addr) {
  if (!(addr == 0x68 || addr == 0x69)) return;

  uint8_t who;
  if (!readReg8(addr, 0x75, who)) return;

  Monitor.println("IMU summary:");
  Monitor.print("  WHO_AM_I @0x75 = 0x");
  printHex2(who);
  Monitor.println();
}

void analyzeDevice(uint8_t addr) {
  printAddressHeader(addr);
  strongIdentify(addr);
  printAddressHints(addr);
  si5351Details(addr);
  boschDetails(addr);
  imuDetails(addr);
  mcp9808Details(addr);
  printQuickReg8(addr);
  printQuickReg16(addr);
  dumpBlock8(addr, 0x00, 0x1F);
}

void scanBusOnce() {
  int found = 0;

  Monitor.println();
  Monitor.println("=== I2C Detective V3 - One Shot ===");
  Monitor.println("Waiting 10 seconds before scan...");
  delay(10000);
  Monitor.println("Starting scan...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    if (pingAddress(addr)) {
      found++;
      analyzeDevice(addr);
    }
  }

  Monitor.println();
  Monitor.print("Total devices found: ");
  Monitor.println(found);
  Monitor.println("One-shot scan complete.");
  Monitor.println("Program now idle.");
  Monitor.println();
}

void setup() {
  Wire.begin();
  Monitor.begin();
  delay(1000);
  scanBusOnce();
}

void loop() {
  while (true) {
    delay(1000);
  }
}
