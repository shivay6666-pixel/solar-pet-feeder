/*
  Module 08 - Motor jam detection  *** FUTURE WORK, UNTESTED ***
  -------------------------------------------------------------
  A kibble jam stalls the motor, and a stalled DC motor draws far more
  current than a freely spinning one. This sketch watches motor current
  while feeding. If current stays high, it stops the motor, backs up, and
  retries. After MAX_RETRIES it gives up and flags a fault.

  Parts to add:
    * ACS712-05B current sensor module (185 mV per amp, output = VCC/2 at 0 A)
    * ADS1115 16-bit ADC module (the NodeMCU's single A0 is already used by
      the battery monitor, so current sensing goes through I2C instead)

  Wiring:
    Motor supply (+) -> ACS712 IP+ ... IP- -> L298N 12V terminal  (in series)
    ACS712 VCC -> 5 V, GND -> GND
    ACS712 OUT -> 10 k -> ADS1115 A0 -> 20 k -> GND   (divider x0.667, because
                  ACS712 swings up to 5 V and the ADS1115 runs on 3.3 V)
    ADS1115 VDD -> 3V3, GND -> GND, ADDR -> GND (address 0x48)
    ADS1115 SDA -> D2, SCL -> D1   (shared with the LCD, which is 0x27)
    Motor driver: ENB D5, IN3 D6, IN4 D7 (same as the rest of the project)

  Scale: ACS712 185 mV/A * 0.667 divider = 123.3 mV/A at the ADS1115.
  With gain +/-4.096 V, one ADS1115 count = 0.125 mV, so about 1 mA/count.

  How to tune (do this with real hardware before trusting it):
    1. Run with the Serial Monitor open. Note "peak A" for normal feeds
       (JAM_THRESHOLD_A must sit comfortably above it).
    2. Block the screw by hand (carefully, motor is strong) and note the
       stall current. Set JAM_THRESHOLD_A between normal peak and stall.
    3. INRUSH_IGNORE_MS hides the brief start-up current spike.

  Not implemented yet: reporting the fault to Blynk (see
  10_blynk_app/blynk_notifications for the notification pattern).
*/

#include <Wire.h>

#define ENB D5
#define IN3 D6
#define IN4 D7

#define ADS_ADDR 0x48

// ---- Tunables ----
const float ACS_MV_PER_A        = 185.0;
const float DIVIDER_GAIN        = 0.6667;
const float JAM_THRESHOLD_A     = 1.5;     // above this = stalled (TUNE ME)
const unsigned long INRUSH_IGNORE_MS = 300;
const unsigned long JAM_CONFIRM_MS   = 250; // must stay high this long
const unsigned long FEED_MS          = 1200;
const unsigned long REVERSE_MS       = 1500;
const int MAX_RETRIES = 2;

float zeroOffsetMv = 0;     // sensor output at 0 A, measured at boot

// ---- ADS1115 (raw I2C, no library needed) ----
bool adsPresent() {
  Wire.beginTransmission(ADS_ADDR);
  return Wire.endTransmission() == 0;
}

// Single-shot read of channel A0 vs GND, gain +/-4.096 V, 860 SPS. Returns mV.
float adsReadMv() {
  Wire.beginTransmission(ADS_ADDR);
  Wire.write(0x01);            // config register
  Wire.write(0xC3);            // OS=1 start, MUX=AIN0/GND, PGA=+/-4.096 V
  Wire.write(0xE3);            // single-shot, 860 SPS, comparator off
  Wire.endTransmission();
  delay(2);                    // conversion takes ~1.2 ms at 860 SPS

  Wire.beginTransmission(ADS_ADDR);
  Wire.write(0x00);            // conversion register
  Wire.endTransmission();
  Wire.requestFrom(ADS_ADDR, 2);
  int16_t raw = (Wire.read() << 8) | Wire.read();
  return raw * 0.125;          // 0.125 mV per count at +/-4.096 V
}

float motorCurrentA() {
  float sensorMv = adsReadMv() / DIVIDER_GAIN;     // undo the divider
  return fabs(sensorMv - zeroOffsetMv) / ACS_MV_PER_A;
}

void calibrateZero() {
  float sum = 0;
  for (int i = 0; i < 64; i++) { sum += adsReadMv() / DIVIDER_GAIN; delay(5); }
  zeroOffsetMv = sum / 64.0;
  Serial.printf("Zero-current offset: %.0f mV (expect ~2500)\n", zeroOffsetMv);
}

// ---- Motor ----
void motorStop() { analogWrite(ENB, 0); digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); }
void motorRun(bool fwd) {
  digitalWrite(IN3, fwd ? HIGH : LOW);
  digitalWrite(IN4, fwd ? LOW : HIGH);
  analogWrite(ENB, 1023);
}

// Runs one portion forward. Returns true if it finished without a jam.
bool feedOnce() {
  Serial.println("Feeding...");
  motorRun(true);
  unsigned long start = millis(), highSince = 0;
  float peak = 0;

  while (millis() - start < FEED_MS) {
    float a = motorCurrentA();
    if (millis() - start > INRUSH_IGNORE_MS) {
      if (a > peak) peak = a;
      if (a > JAM_THRESHOLD_A) {
        if (!highSince) highSince = millis();
        if (millis() - highSince >= JAM_CONFIRM_MS) {
          motorStop();
          Serial.printf("JAM detected at %.2f A\n", a);
          return false;
        }
      } else {
        highSince = 0;
      }
    }
    delay(10);
  }
  motorStop();
  Serial.printf("Feed OK, peak %.2f A\n", peak);
  return true;
}

// Feed with automatic jam recovery. Returns true on success.
bool feedWithRecovery() {
  for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
    if (feedOnce()) return true;
    Serial.printf("Reversing to clear jam (retry %d of %d)\n", attempt + 1, MAX_RETRIES);
    motorRun(false);
    delay(REVERSE_MS);
    motorStop();
    delay(500);
  }
  Serial.println("FAULT: still jammed after retries. Needs a human.");
  return false;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();                                // D2 = SDA, D1 = SCL
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  motorStop();
  Serial.println("\nModule 08: motor_jam_detect (FUTURE / UNTESTED)");

  if (!adsPresent()) {
    Serial.println("ADS1115 not found at 0x48. Check wiring. Halting.");
    while (true) delay(1000);
  }
  calibrateZero();
}

void loop() {
  feedWithRecovery();
  Serial.println("Next feed in 30 s\n");
  delay(30000);
}
