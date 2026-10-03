/*
  Module 07 - Battery voltage and charging monitor
  ------------------------------------------------
  STATUS: reference code, not yet verified on the built feeder. The divider
  values below are a worked example: CHECK them against your actual pack.

  Measures the battery through a resistor divider on A0, smooths it, estimates
  percent charge, and works out whether the solar charger is charging,
  full, or the battery is discharging, by watching the voltage trend.

  Wiring:
    Battery (+) --- R1 ---+--- NodeMCU A0
                          |
                          R2
                          |
    Battery (-) ----------+--- NodeMCU GND  (common ground!)
    Add a 100 nF capacitor from A0 to GND to steady the reading.

  The NodeMCU's A0 pin accepts 0 to 3.2 V (it has its own on-board divider).
  Pick R1/R2 so that your FULL battery voltage lands just under 3.2 V:

      V_pin = V_bat * R2 / (R1 + R2)

    Pack            Full voltage   R1      R2      V_pin at full
    1S Li-ion       4.2 V          100 k   300 k   3.15 V
    3S Li-ion      12.6 V          300 k   100 k   3.15 V   (default below)
    12 V lead-acid ~14.4 V         390 k   100 k   2.94 V

  Because the on-board divider has ~320 k input resistance, large external
  resistors cause a small error. Fix it by measuring the real battery with a
  multimeter and adjusting VBAT_CAL until the printed voltage matches.

  Charge detection is trend based: every TREND_INTERVAL_MS the smoothed
  voltage is compared with the previous one. Rising = CHARGING. Near the
  full voltage and flat = FULL. Falling = DISCHARGING. It is an estimate,
  not a coulomb counter. A dedicated charge-status pin on the charger
  would be more precise if you have one.
*/

// ---- Pack configuration (edit for your battery) ----
#define BATTERY_CELLS      3          // Li-ion cells in series (1S, 3S ...)
const float R1_OHMS        = 300000.0;
const float R2_OHMS        = 100000.0;
const float VBAT_CAL       = 1.00;    // multimeter_voltage / printed_voltage

// ---- Thresholds ----
const float LOW_BATT_PER_CELL  = 3.50;   // warn below this (per cell)
const float FULL_PER_CELL      = 4.15;   // "full" at or above this (per cell)
const float TREND_DEADBAND_V   = 0.02;   // pack volts; smaller changes = "flat"
const unsigned long SAMPLE_MS         = 1000;
const unsigned long TREND_INTERVAL_MS = 60000;

const float ADC_FULL_SCALE_V = 3.2;       // NodeMCU A0: 1023 counts = 3.2 V

// Li-ion open-circuit voltage -> percent (per cell). Rest voltage is the
// accurate one; under load or while charging it reads a bit off.
const float OCV_V[]   = { 3.30, 3.45, 3.68, 3.74, 3.77, 3.79, 3.82, 3.87, 3.92, 3.98, 4.06, 4.20 };
const float OCV_PCT[] = {    0,    5,   10,   20,   30,   40,   50,   60,   70,   80,   90,  100 };
const int   OCV_N     = sizeof(OCV_V) / sizeof(OCV_V[0]);

float smoothedV = 0;
float lastTrendV = 0;
const char* chargeState = "STARTING";

float readBatteryVolts() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogRead(A0); delay(2); }
  float pinV = (sum / 16.0) / 1023.0 * ADC_FULL_SCALE_V;
  return pinV * (R1_OHMS + R2_OHMS) / R2_OHMS * VBAT_CAL;
}

int percentFromVoltage(float packV) {
  float v = packV / BATTERY_CELLS;
  if (v <= OCV_V[0]) return 0;
  if (v >= OCV_V[OCV_N - 1]) return 100;
  for (int i = 1; i < OCV_N; i++) {
    if (v <= OCV_V[i]) {                   // linear interpolation in this segment
      float t = (v - OCV_V[i - 1]) / (OCV_V[i] - OCV_V[i - 1]);
      return (int)(OCV_PCT[i - 1] + t * (OCV_PCT[i] - OCV_PCT[i - 1]) + 0.5);
    }
  }
  return 100;
}

void updateChargeState() {
  float delta = smoothedV - lastTrendV;
  lastTrendV = smoothedV;
  bool nearFull = smoothedV / BATTERY_CELLS >= FULL_PER_CELL;

  if (delta > TREND_DEADBAND_V)       chargeState = "CHARGING";
  else if (nearFull)                  chargeState = "FULL";
  else if (delta < -TREND_DEADBAND_V) chargeState = "DISCHARGING";
  else                                chargeState = "STEADY";
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nModule 07: battery_monitor");
  smoothedV = readBatteryVolts();
  lastTrendV = smoothedV;
}

void loop() {
  static unsigned long lastSample = 0, lastTrend = 0;

  if (millis() - lastSample >= SAMPLE_MS) {
    lastSample = millis();
    float v = readBatteryVolts();
    smoothedV += 0.1 * (v - smoothedV);          // exponential moving average

    bool low = smoothedV / BATTERY_CELLS < LOW_BATT_PER_CELL;
    Serial.printf("Battery %5.2f V  %3d%%  %-11s %s\n",
                  smoothedV, percentFromVoltage(smoothedV), chargeState,
                  low ? "[LOW BATTERY]" : "");
  }

  if (millis() - lastTrend >= TREND_INTERVAL_MS) {
    lastTrend = millis();
    updateChargeState();
  }
}
