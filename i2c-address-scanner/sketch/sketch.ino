#include <Wire.h>
#include <Arduino_RouterBridge.h>

void setup() {
  Wire.begin();
  Monitor.begin();
  delay(1000);
  Monitor.println("I2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Monitor.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Monitor.print("I2C device found at 0x");
      if (address < 16) Monitor.print("0");
      Monitor.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Monitor.println("No I2C devices found");
  } else {
    Monitor.println("Scan done");
  }

  Monitor.println();
  delay(3000);
}
