/*
  Module 09a - I2C scanner
  ------------------------
  Finds the address of every I2C device on the bus. Use it when the LCD
  stays blank: most 16x2 I2C backpacks are 0x27, but some are 0x3F. Whatever
  address shows up here is the one to put in LiquidCrystal_I2C lcd(ADDR, 16, 2).

  Wiring: SDA -> NodeMCU D2, SCL -> NodeMCU D1, VCC -> 5 V, GND -> GND.
  (If the screen is on but shows only blocks or nothing, also turn the small
  blue contrast potentiometer on the back of the backpack.)
*/

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();                       // NodeMCU default: SDA = D2, SCL = D1
  Serial.println("\nModule 09a: i2c_scanner");
}

void loop() {
  int found = 0;
  Serial.println("Scanning...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  device at 0x%02X\n", addr);
      found++;
    }
  }
  Serial.println(found ? "done\n" : "No I2C devices found. Check wiring.\n");
  delay(5000);
}
