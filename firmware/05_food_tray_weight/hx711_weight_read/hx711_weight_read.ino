/*
  Module 05b - Food tray weight reading (HX711 load cell)
  -------------------------------------------------------
  Prints the weight of food in the tray every second, with a tare command
  and a low-food flag. This is the same read logic used by the full firmware
  (00_full_system), minus WiFi/Blynk.

  Same wiring as hx711_calibration (DT -> D3, SCK -> D4).

  Serial commands (115200 baud): t = tare (tray must be EMPTY)

  Design notes from the real build:
    * The tray is tared at boot, so power the feeder up with an empty tray.
    * Readings below 0 are clamped to 0 (small drift is normal).
    * HX711 init and tare are guarded with timeouts so a missing load cell
      can never freeze the board before it connects to WiFi.
*/

#include <HX711.h>

#define HX711_DT  D3
#define HX711_SCK D4

// Put the number from hx711_calibration here. The sign can be negative.
#define LOADCELL_CALIBRATION_FACTOR -7050

const float LOW_FOOD_GRAMS = 30.0;           // below this counts as "tray low/empty"
const unsigned long READ_INTERVAL_MS = 1000;

HX711 scale;
bool scaleReady = false;

bool tareScale() {
  if (!scale.wait_ready_timeout(1000, 100)) return false;
  scale.tare(10);
  scaleReady = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nModule 05b: hx711_weight_read");

  scale.begin(HX711_DT, HX711_SCK, false, false);
  scale.set_scale(LOADCELL_CALIBRATION_FACTOR);

  if (tareScale()) Serial.println("HX711 ready, tared (tray assumed empty).");
  else             Serial.println("HX711 not ready. Check wiring; will retry on 't'.");
}

void loop() {
  if (Serial.available() && Serial.read() == 't') {
    Serial.println(tareScale() ? "Tared." : "Tare failed: HX711 not ready.");
  }

  static unsigned long last = 0;
  if (millis() - last < READ_INTERVAL_MS) return;
  last = millis();

  if (!scale.is_ready()) {
    Serial.println("HX711 not ready");
    return;
  }

  float grams = scale.get_units(5);          // average of 5 samples
  if (grams < 0) grams = 0;

  Serial.printf("Tray weight: %6.1f g  %s\n", grams,
                (scaleReady && grams < LOW_FOOD_GRAMS) ? "[LOW FOOD]" : "");
}
