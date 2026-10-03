/*
  Module 02 - ESP8266 / NodeMCU board bring-up
  --------------------------------------------
  First sketch to run on a new board. Confirms the USB cable, driver, board
  selection and upload speed all work, and prints chip facts useful when
  something later misbehaves (reset reason, free heap, flash size).

  Also blinks the on-board LED and reads A0.

  Board: NodeMCU 1.0 (ESP-12E Module), upload speed 115200.

  NOTE: the on-board blue LED is wired to D4 (GPIO2). In the full feeder, D4
  is the HX711 clock line, so the LED will flicker whenever the load cell is
  read. That is normal.

  Feeder pin map (NodeMCU label -> function):
    D1 SCL  LCD I2C            D5 ENB  motor driver enable (PWM)
    D2 SDA  LCD I2C            D6 IN3  motor driver direction
    D3 DT   HX711 data         D7 IN4  motor driver direction
    D4 SCK  HX711 clock        D0      IR sensor      (module 04)
    A0      battery voltage    D8      buzzer         (module 06)
*/

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("\n\n=== ESP8266 board info ===");
  Serial.printf("Chip ID         : 0x%06X\n", ESP.getChipId());
  Serial.printf("CPU frequency   : %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash size      : %u KB (real) / %u KB (configured)\n",
                ESP.getFlashChipRealSize() / 1024, ESP.getFlashChipSize() / 1024);
  Serial.printf("Sketch size     : %u bytes (%u free)\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());
  Serial.printf("Free heap       : %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Core version    : %s\n", ESP.getCoreVersion().c_str());
  Serial.printf("SDK version     : %s\n", ESP.getSdkVersion());
  Serial.printf("Reset reason    : %s\n", ESP.getResetReason().c_str());
  Serial.println("==========================");
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);    // LED is active-LOW on NodeMCU
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(800);

  // A0 reads 0-1023 for 0-3.2 V (the NodeMCU has a divider on this pin).
  Serial.printf("alive  uptime=%lus  heap=%u  A0=%d\n",
                millis() / 1000, ESP.getFreeHeap(), analogRead(A0));
}
