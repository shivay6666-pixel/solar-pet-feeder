/*
  Module 04 - IR obstacle sensor (animal at the bowl)
  ---------------------------------------------------
  STATUS: reference code, not yet verified on the built feeder.

  Reads a common 3-pin IR obstacle-avoidance module (FC-51 style). It pulls
  its OUT pin LOW when something is in front of it. This sketch debounces
  that signal and reports each visit with how long the animal stayed, which
  is useful data for the research paper (feeding behaviour per day).

  Wiring:
    IR module VCC -> 3V3    (3.3 V keeps OUT safe for the ESP8266 pin)
    IR module GND -> GND
    IR module OUT -> NodeMCU D0 (GPIO16)

  Tuning: turn the small potentiometer on the module to set detection
  distance. The module's own LED lights when it detects something.

  Why D0: D8 must be LOW at boot and D3/D4 are used by the load cell, so D0
  is the free pin that is safe for a plain digital input. (D0 has no
  interrupt support, so this sketch polls.)
*/

#define IR_PIN D0

const unsigned long DEBOUNCE_MS   = 80;     // signal must be stable this long
const unsigned long MIN_VISIT_MS  = 1000;   // ignore blips shorter than this

bool stableState = false;         // true = object present
bool lastRaw = false;
unsigned long lastChange = 0;
unsigned long visitStart = 0;
unsigned long visitCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  Serial.println("\nModule 04: ir_presence_detect");
}

void loop() {
  bool raw = (digitalRead(IR_PIN) == LOW);   // LOW = detected

  if (raw != lastRaw) {
    lastRaw = raw;
    lastChange = millis();
  }

  if (raw != stableState && millis() - lastChange >= DEBOUNCE_MS) {
    stableState = raw;
    if (stableState) {
      visitStart = millis();
      Serial.println("Animal arrived");
    } else {
      unsigned long dwell = millis() - visitStart;
      if (dwell >= MIN_VISIT_MS) {
        visitCount++;
        Serial.printf("Animal left. Visit #%lu lasted %.1f s\n", visitCount, dwell / 1000.0);
      } else {
        Serial.println("Ignored short blip");
      }
    }
  }
}
