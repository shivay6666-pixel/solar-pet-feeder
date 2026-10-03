/*
  Module 05a - Load cell calibration (HX711)
  ------------------------------------------
  Run this ONCE to find the calibration factor, then paste that number into
  LOADCELL_CALIBRATION_FACTOR in the sketches that read weight.

  Wiring (HX711 board):
    HX711 VCC -> 3V3     HX711 GND -> GND
    HX711 DT  -> NodeMCU D3
    HX711 SCK -> NodeMCU D4
    Load cell wires -> E+ / E- / A- / A+ (colours vary by cell; if the
    reading goes NEGATIVE when you add weight, swap A+ and A-, or just let
    the factor come out negative, which also works)

  Library: "HX711" by Rob Tillaart (Library Manager).

  Procedure (Serial Monitor, 115200 baud, line ending "Newline"):
    1. Empty tray. Sketch tares automatically at start. Type  t  to re-tare.
    2. Put a known weight on the tray (a phone, a bag of rice weighed on a
       kitchen scale, anything you know in grams).
    3. Type that weight in grams, e.g.  250  and press Enter.
    4. The sketch prints the calibration factor. Copy it.
    5. Remove the weight and check that other weights now read correctly.
*/

#include <HX711.h>

#define HX711_DT  D3
#define HX711_SCK D4

HX711 scale;
float factor = 1.0;

// The HX711 library's read() has no timeout. If the chip is unplugged or
// unpowered it would hang forever, so always check readiness first.
bool hx711Ready(uint32_t timeoutMs = 1000) {
  return scale.wait_ready_timeout(timeoutMs, 50);
}

void tareNow() {
  if (!hx711Ready()) {
    Serial.println("HX711 not responding. Check wiring and 3V3 power.");
    return;
  }
  scale.set_scale(1.0);
  scale.tare(15);
  Serial.println("Tared. Place a known weight, then type its weight in grams.");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nModule 05a: hx711_calibration");

  // doReset=false: skip the library's blocking reset read on startup.
  scale.begin(HX711_DT, HX711_SCK, false, false);
  tareNow();
}

void loop() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  if (line == "t") {
    tareNow();
    return;
  }

  float knownGrams = line.toFloat();
  if (knownGrams <= 0) {
    Serial.println("Type a weight in grams (e.g. 250) or t to tare.");
    return;
  }
  if (!hx711Ready()) {
    Serial.println("HX711 not responding.");
    return;
  }

  scale.set_scale(1.0);
  float raw = scale.get_units(20);      // offset-corrected, un-scaled reading
  factor = raw / knownGrams;

  Serial.printf("Raw reading : %.1f\n", raw);
  Serial.printf("Known weight: %.1f g\n", knownGrams);
  Serial.printf("\n>>> LOADCELL_CALIBRATION_FACTOR = %.2f <<<\n\n", factor);

  scale.set_scale(factor);
  Serial.printf("Check read-back: %.1f g\n", scale.get_units(10));
}
