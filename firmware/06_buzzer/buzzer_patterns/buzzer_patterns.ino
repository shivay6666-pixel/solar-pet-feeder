/*
  Module 06 - Buzzer alerts
  -------------------------
  STATUS: reference code, not yet verified on the built feeder.

  Non-blocking beep patterns so the buzzer never delays feeding or WiFi.
  Each event has its own rhythm so you can tell them apart by ear:

    FEED_START   1 short beep           "dispensing now"
    FEED_DONE    2 short beeps          "done"
    LOW_FOOD     3 slow beeps           "refill the hopper"
    LOW_BATTERY  long, short            "power is low"
    JAM          fast beeping, ~3 s     "motor stuck"

  Wiring (active buzzer, the kind that beeps when given DC):
    NodeMCU D8 -> 1 kOhm -> base of NPN transistor (2N2222 / BC547)
    Transistor emitter -> GND
    Buzzer (+) -> 5 V (VIN), buzzer (-) -> transistor collector
    (Direct drive from the pin also works for tiny 3.3 V buzzers.)

  Why D8: it is the last free pin. D8 must be LOW at boot, and the
  transistor's base resistor keeps it low, so boot is not affected.

  Passive buzzer? Set BUZZER_PASSIVE to 1 and it plays a 2.7 kHz tone.

  The demo in loop() cycles through all patterns every 6 seconds.
*/

#define BUZZER_PIN D8
#define BUZZER_PASSIVE 0
#define BUZZER_FREQ_HZ 2700

// A pattern is a list of alternating ON/OFF durations in ms, ending with 0.
const uint16_t PAT_FEED_START[]  = { 150, 0 };
const uint16_t PAT_FEED_DONE[]   = { 120, 120, 120, 0 };
const uint16_t PAT_LOW_FOOD[]    = { 400, 400, 400, 400, 400, 0 };
const uint16_t PAT_LOW_BATTERY[] = { 800, 250, 150, 0 };
const uint16_t PAT_JAM[]         = { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                                     100, 100, 100, 100, 100, 100, 100, 100, 100, 0 };

const uint16_t* currentPattern = nullptr;
uint8_t patternIndex = 0;
unsigned long stepStart = 0;

void buzzerOn() {
#if BUZZER_PASSIVE
  tone(BUZZER_PIN, BUZZER_FREQ_HZ);
#else
  digitalWrite(BUZZER_PIN, HIGH);
#endif
}

void buzzerOff() {
#if BUZZER_PASSIVE
  noTone(BUZZER_PIN);
#endif
  digitalWrite(BUZZER_PIN, LOW);
}

void playPattern(const uint16_t* pattern) {
  currentPattern = pattern;
  patternIndex = 0;
  stepStart = millis();
  buzzerOn();                       // even indexes are ON, odd are OFF
}

// Call from loop() as often as possible.
void buzzerUpdate() {
  if (!currentPattern) return;
  if (millis() - stepStart < currentPattern[patternIndex]) return;

  patternIndex++;
  stepStart = millis();
  if (currentPattern[patternIndex] == 0) {   // reached the end marker
    currentPattern = nullptr;
    buzzerOff();
    return;
  }
  if (patternIndex % 2 == 0) buzzerOn(); else buzzerOff();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  buzzerOff();
  Serial.println("\nModule 06: buzzer_patterns");
}

void loop() {
  buzzerUpdate();

  static unsigned long lastDemo = 0;
  static uint8_t which = 0;
  if (millis() - lastDemo > 6000) {
    lastDemo = millis();
    switch (which++ % 5) {
      case 0: Serial.println("FEED_START");  playPattern(PAT_FEED_START);  break;
      case 1: Serial.println("FEED_DONE");   playPattern(PAT_FEED_DONE);   break;
      case 2: Serial.println("LOW_FOOD");    playPattern(PAT_LOW_FOOD);    break;
      case 3: Serial.println("LOW_BATTERY"); playPattern(PAT_LOW_BATTERY); break;
      case 4: Serial.println("JAM");         playPattern(PAT_JAM);         break;
    }
  }
}
