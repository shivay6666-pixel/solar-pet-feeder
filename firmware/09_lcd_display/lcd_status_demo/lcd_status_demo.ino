/*
  Module 09b - 16x2 LCD status screens (demo with fake data)
  ----------------------------------------------------------
  Shows the three idle screens used by the real feeder, rotating every 5 s,
  plus the "Feeding Now!" override. No WiFi, no clock: the data is fake so
  you can check the display and the layout on their own.

    1. Clock        "10:42 AM / Mon Oct 05"
    2. Next Feed    "Next Feed: / 5:30 PM"
    3. Last Fed     "Last Fed: / 3:45 PM"

  The real firmware shows plain clock times on purpose (not "in 2h 10m"
  countdowns). See 00_full_system for the live version.

  Library: "LiquidCrystal_I2C" (Library Manager).
  Wiring: SDA -> D2, SCL -> D1, VCC -> 5 V, GND -> GND.
  Address 0x27 below; run i2c_scanner if the screen stays blank.
*/

#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

void show(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(line1);
  lcd.setCursor(0, 1); lcd.print(line2);
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  show("Pet Feeder", "LCD test");
  delay(2000);
}

void loop() {
  show("10:42 AM", "Mon Oct 05");   delay(5000);
  show("Next Feed:", "5:30 PM");    delay(5000);
  show("Last Fed:", "3:45 PM");     delay(5000);

  show("Feeding Now!", "Enjoy your meal");
  delay(2000);
}
