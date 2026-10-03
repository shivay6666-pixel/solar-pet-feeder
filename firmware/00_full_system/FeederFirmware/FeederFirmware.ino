/*
  FeederFirmware - integrated solar pet feeder firmware (tested on hardware)
  ----------------------------------------------------------------------
  Everything below runs together on one NodeMCU ESP8266:
    motor (L298N) + Blynk iPhone app + 3-slot on-device scheduler + NTP clock
    + HX711 food tray weight + 16x2 I2C LCD with rotating status screens
    + reverse/unjam button.

  The numbered folders next to this one (01_network ... 10_blynk_app) split
  each part into a small standalone sketch for testing and learning.
  Not yet merged in here: IR sensor (04), buzzer (06), battery monitor (07),
  jam detection (08). Those are reference code awaiting hardware testing.

  Libraries: Blynk, LiquidCrystal_I2C, HX711 (Rob Tillaart).
  Board: NodeMCU 1.0 (ESP-12E Module), upload speed 115200.
*/
#define BLYNK_PRINT Serial
#include "secrets.h"   // WiFi + Blynk credentials (copy secrets.example.h to secrets.h)

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>
#include <HX711.h>
#include <time.h>

char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

#define ENB D5
#define IN3 D6
#define IN4 D7

#define HX711_DT  D3
#define HX711_SCK D4

// Run 05_food_tray_weight/hx711_calibration first and paste YOUR value here
#define LOADCELL_CALIBRATION_FACTOR -7050

// India Standard Time: UTC+5:30, no daylight saving
#define IST_OFFSET_SEC (5 * 3600 + 30 * 60)

// ============================================================================
// BLYNK APP WIDGET SETUP — add a widget in the app for each pin below.
// Use the exact "Label" text as the widget's name. Pins marked (already set
// up) exist and work today; the rest need a widget created before they'll
// do anything (the firmware default listed still applies until you do).
// ============================================================================
//  Pin | Label                | Type              | Range     | Default
// -----|----------------------|-------------------|-----------|------------------
//  V0  | Motor Button         | Button (PUSH)     | 0-1       |
//  V1  | Feed 1 - hour        | same as V1 today  | 0-23      |
//  V2  | Feed 1 - minute      | same as V2 today  | 0-59      |
//  V3  | Bowl Weight (g)      | Value display     | read-only |
//  V4  | Tare Scale           | Button (PUSH)     | 0-1       |
//  V5  | Feed 2 - hour        | Integer input     | 0-23      | unset = slot off
//  V6  | Feed 2 - minute      | Integer input     | 0-59      | unset = slot off
//  V7  | Feed 3 - hour        | Integer input     | 0-23      | unset = slot off
//  V8  | Feed 3 - minute      | Integer input     | 0-59      | unset = slot off
//  V9  | Feed Days            | Integer input     | 0-127     | 127 (every day)
//  V10 | Portion Size (sec)   | Slider            | 1-10      | 1 (1.2s actual)
//  V11 | Auto-Feed Enabled    | Switch            | 0-1       | 1 (on)
//  V12 | Reverse / Unjam      | Button (PUSH)     | 0-1       | 
//
//  V9 "Feed Days" bitmask — add up the days you want auto-feeding active:
//    Sun=1  Mon=2  Tue=4  Wed=8  Thu=16  Fri=32  Sat=64
//    Examples: every day = 127, weekdays only = 62, weekends only = 65
// ============================================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;
HX711 scale;
float bowlWeight = 0;
bool scaleReady = false;

int rotateTime = 1200;   // portion size in ms — now settable from the app via V10
bool motorRunning = false;

// ---- Feed schedule: up to 3 times per day ----
// Slot 0 = V1/V2 (kept for backward compat with everything already set up),
// Slot 1 = V5/V6, Slot 2 = V7/V8. Each slot is independent: -1/-1 means
// "not set", same sentinel the LCD already used for the single-slot version.
#define NUM_SLOTS 3
int slotHour[NUM_SLOTS] = { -1, -1, -1 };
int slotMin[NUM_SLOTS]  = { -1, -1, -1 };

// Which days of the week auto-feeding is allowed at all, as a bitmask —
// bit0=Sunday .. bit6=Saturday (matches struct tm's tm_wday numbering).
// Set from the app via V9. Default: every day, until the app says otherwise.
uint8_t feedDayMask = 0x7F;

// Master pause switch from the app (V11) — when false, no slot fires,
// but the saved times/days are untouched so resuming needs no re-entry.
bool autoFeedEnabled = true;

// Timestamp (epoch seconds) of the last time a feed actually ran (any slot,
// manual button, or schedule). 0 = never fed yet since boot. Only
// meaningful once NTP has synced.
time_t lastFeedEpoch = 0;

// Day-of-year (0-365) each slot last auto-fired, tracked per-slot so one
// slot firing in the morning doesn't block a different slot from firing
// later the same day. -1 = hasn't fired this boot.
int slotLastFedYday[NUM_SLOTS] = { -1, -1, -1 };

// ---- LCD idle-screen rotation ----
// While the motor isn't running, the LCD cycles through these screens.
// Feeding always overrides whatever screen is showing (see rotateMotor()).
enum LcdScreen { SCR_CLOCK = 0, SCR_NEXT_FEED = 1, SCR_LAST_FED = 2, SCR_COUNT = 3 };
LcdScreen currentScreen = SCR_CLOCK;

// NTP gives us epoch time; anything before this is "not synced yet"
// (roughly Nov 2023 — comfortably after any real sync, but boot-time
// default epoch is 1970, so this is a safe/simple sanity check).
const time_t NTP_SYNCED_THRESHOLD = 1700000000;

// Format hour(0-23)+minute into 12-hour "H:MM AM/PM"
void format12h(int h24, int mIn, char *buf, size_t bufSize) {
  int h = h24 % 24;
  const char *ampm = (h >= 12) ? "PM" : "AM";
  if (h == 0) h = 12;
  else if (h > 12) h -= 12;
  snprintf(buf, bufSize, "%d:%02d %s", h, mIn, ampm);
}

void renderClockScreen() {
  time_t now = time(nullptr);
  lcd.clear();
  lcd.setCursor(0, 0);
  if (now < NTP_SYNCED_THRESHOLD) {
    lcd.print("Pet Feeder Ready");
    lcd.setCursor(0, 1);
    lcd.print("Syncing time...");
    return;
  }
  struct tm tmNow;
  localtime_r(&now, &tmNow);
  char timeBuf[10];
  format12h(tmNow.tm_hour, tmNow.tm_min, timeBuf, sizeof(timeBuf));
  char dateBuf[17];
  strftime(dateBuf, sizeof(dateBuf), "%a %b %d", &tmNow);
  lcd.print(timeBuf);
  lcd.setCursor(0, 1);
  lcd.print(dateBuf);
}

void renderNextFeedScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Next Feed:");
  lcd.setCursor(0, 1);

  if (!autoFeedEnabled) {
    lcd.print("Paused");
    return;
  }

  // Show the earliest time-of-day among whichever slots are actually set.
  // (This is a simple chronological soonest, not a full day-mask-aware
  // lookahead — if that slot's day is skipped today, the schedule logic
  // itself still respects the mask correctly, this is just the LCD preview.)
  int bestHour = -1, bestMin = -1;
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (slotHour[i] < 0 || slotMin[i] < 0) continue;
    if (bestHour < 0 || slotHour[i] < bestHour || (slotHour[i] == bestHour && slotMin[i] < bestMin)) {
      bestHour = slotHour[i];
      bestMin = slotMin[i];
    }
  }

  if (bestHour < 0) {
    lcd.print("No time set");
    return;
  }
  char timeBuf[10];
  format12h(bestHour, bestMin, timeBuf, sizeof(timeBuf));
  lcd.print(timeBuf);
}

void renderLastFedScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Last Fed:");
  lcd.setCursor(0, 1);
  // lastFeedEpoch is only meaningful once NTP has synced — if a feed
  // happened before that (first few seconds after boot), there's no real
  // timestamp for it, so just fall back to "Not fed yet".
  if (lastFeedEpoch < NTP_SYNCED_THRESHOLD) {
    lcd.print("Not fed yet");
    return;
  }
  struct tm tmFed;
  localtime_r(&lastFeedEpoch, &tmFed);
  char timeBuf[10];
  format12h(tmFed.tm_hour, tmFed.tm_min, timeBuf, sizeof(timeBuf));
  lcd.print(timeBuf);
}

// Draws whatever the current idle screen is. Never called while feeding —
// the feeding message always wins and idle redraws must not stomp on it.
void renderIdleScreen() {
  if (motorRunning) return;
  switch (currentScreen) {
    case SCR_CLOCK:     renderClockScreen();    break;
    case SCR_NEXT_FEED: renderNextFeedScreen(); break;
    case SCR_LAST_FED:  renderLastFedScreen();  break;
  }
}

// Called every 5s: advances to the next idle screen (skipped entirely while feeding)
void advanceIdleScreen() {
  if (motorRunning) return;
  currentScreen = (LcdScreen)((currentScreen + 1) % SCR_COUNT);
  renderIdleScreen();
}

// Called every 1s: keeps the clock screen's minute display fresh without
// waiting for the next full rotation, but only while it's the active screen.
void tickClockScreen() {
  if (!motorRunning && currentScreen == SCR_CLOCK) {
    renderClockScreen();
  }
}

// Runs the actual schedule: fires a feed on-device the moment the synced
// clock matches any set slot's hour:minute, on an enabled day, once per
// slot per day. No Blynk Automation involved at all — this is fully
// self-contained on the board, so changing times/days/pause from the app
// "just works" without ever needing a reflash.
void checkScheduledFeed() {
  if (motorRunning) return;
  if (!autoFeedEnabled) return;   // master pause switch (V11)

  time_t now = time(nullptr);
  if (now < NTP_SYNCED_THRESHOLD) return;    // can't safely compare until clock is real

  struct tm tmNow;
  localtime_r(&now, &tmNow);

  bool todayIsFeedDay = feedDayMask & (1 << tmNow.tm_wday);

  // Debug visibility, throttled to every 10s so it doesn't spam a 1s timer.
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 10000) {
    lastDebugPrint = millis();
    Serial.print("checkScheduledFeed: now="); Serial.print(tmNow.tm_hour); Serial.print(":"); Serial.print(tmNow.tm_min);
    Serial.print(" wday="); Serial.print(tmNow.tm_wday);
    Serial.print(" dayEnabled="); Serial.print(todayIsFeedDay ? "Y" : "N");
    Serial.print(" slots=");
    for (int i = 0; i < NUM_SLOTS; i++) {
      Serial.print(slotHour[i]); Serial.print(":"); Serial.print(slotMin[i]);
      Serial.print(i < NUM_SLOTS - 1 ? "," : "");
    }
    Serial.println();
  }

  if (!todayIsFeedDay) return;   // not a feeding day at all today, skip every slot

  for (int i = 0; i < NUM_SLOTS; i++) {
    if (slotHour[i] < 0 || slotMin[i] < 0) continue;   // this slot not set
    bool alreadyFedToday = (slotLastFedYday[i] == tmNow.tm_yday);
    if (tmNow.tm_hour == slotHour[i] && tmNow.tm_min == slotMin[i] && !alreadyFedToday) {
      slotLastFedYday[i] = tmNow.tm_yday;
      Serial.print("Scheduled feed triggered (slot "); Serial.print(i + 1); Serial.println(")");
      rotateMotor();
      return;   // one feed is enough even if two slots coincidentally match the same minute
    }
  }
}

void stopMotor() {
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motorRunning = false;
  Blynk.virtualWrite(V0, 0);
  renderIdleScreen();   // resume idle rotation right where it left off
}

void rotateMotor() {
  Serial.println("rotateMotor() called");
  if (motorRunning) {
    Serial.println("  -> already running, ignored");
    return;
  }
  motorRunning = true;
  lastFeedEpoch = time(nullptr);   // 0 (unsynced) until NTP catches up; harmless
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  digitalWrite(ENB, HIGH);
  Serial.println("  -> IN3=HIGH IN4=LOW ENB=HIGH, motor should be spinning now");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Feeding Now!");
  lcd.setCursor(0, 1);
  lcd.print("Enjoy your meal");
  timer.setTimeout(rotateTime, stopMotor);
}

// Fixed duration for the reverse/unjam action, independent of the adjustable
// portion size (V10) — clearing a jam is a different job than dispensing a
// normal portion, so it shouldn't scale with that setting.
#define REVERSE_TIME_MS 1500

void stopReverseMotor() {
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motorRunning = false;
  renderIdleScreen();   // resume idle rotation right where it left off
}

// Runs the motor briefly in the opposite direction from a normal feed, to
// back stuck food out of the mechanism. Same motorRunning guard as
// rotateMotor() so a feed and a reverse can never overlap.
void reverseMotor() {
  Serial.println("reverseMotor() called (unjam)");
  if (motorRunning) {
    Serial.println("  -> already running, ignored");
    return;
  }
  motorRunning = true;
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);   // opposite of rotateMotor()'s HIGH/LOW
  digitalWrite(ENB, HIGH);
  Serial.println("  -> IN3=LOW IN4=HIGH ENB=HIGH, motor reversing to clear jam");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clearing Jam...");
  lcd.setCursor(0, 1);
  lcd.print("Reversing");
  timer.setTimeout(REVERSE_TIME_MS, stopReverseMotor);
}

// App widget: "Motor Button" — Button (PUSH), range 0-1 — already set up
BLYNK_WRITE(V0) {
  Serial.print("BLYNK_WRITE(V0) fired, value=");
  Serial.println(param.asInt());
  if (param.asInt() == 1) rotateMotor();
}

// App widget: "Reverse / Unjam" — Button (PUSH), range 0-1
// Runs the motor backward for 1.5s to help clear a jammed mechanism
BLYNK_WRITE(V12) {
  Serial.print("BLYNK_WRITE(V12) fired, value=");
  Serial.println(param.asInt());
  if (param.asInt() == 1) {
    reverseMotor();
    Blynk.virtualWrite(V12, 0);   // auto-reset, same PUSH-button pattern as V0/V4
  }
}

// Board watches its own clock and auto-feeds when it matches any set slot
// below (see checkScheduledFeed()) — no Blynk Automation needed, changing
// any of these from the app takes effect immediately, no reflash required.

// App widget: "Feed 1 - hour" — Integer input, range 0-23 — already set up
BLYNK_WRITE(V1) {
  slotHour[0] = param.asInt();
  Serial.print("BLYNK_WRITE(V1) fired, slot1 hour=");
  Serial.println(slotHour[0]);
  if (!motorRunning) renderIdleScreen();
}

// App widget: "Feed 1 - minute" — Integer input, range 0-59 — already set up
BLYNK_WRITE(V2) {
  slotMin[0] = param.asInt();
  Serial.print("BLYNK_WRITE(V2) fired, slot1 min=");
  Serial.println(slotMin[0]);
  if (!motorRunning) renderIdleScreen();
}

// App widget: "Feed 2 - hour" — Integer input, range 0-23
BLYNK_WRITE(V5) {
  slotHour[1] = param.asInt();
  Serial.print("BLYNK_WRITE(V5) fired, slot2 hour=");
  Serial.println(slotHour[1]);
  if (!motorRunning) renderIdleScreen();
}

// App widget: "Feed 2 - minute" — Integer input, range 0-59
BLYNK_WRITE(V6) {
  slotMin[1] = param.asInt();
  Serial.print("BLYNK_WRITE(V6) fired, slot2 min=");
  Serial.println(slotMin[1]);
  if (!motorRunning) renderIdleScreen();
}

// App widget: "Feed 3 - hour" — Integer input, range 0-23
BLYNK_WRITE(V7) {
  slotHour[2] = param.asInt();
  Serial.print("BLYNK_WRITE(V7) fired, slot3 hour=");
  Serial.println(slotHour[2]);
  if (!motorRunning) renderIdleScreen();
}

// App widget: "Feed 3 - minute" — Integer input, range 0-59
BLYNK_WRITE(V8) {
  slotMin[2] = param.asInt();
  Serial.print("BLYNK_WRITE(V8) fired, slot3 min=");
  Serial.println(slotMin[2]);
  if (!motorRunning) renderIdleScreen();
}

// App widget: "Feed Days" — Integer input, range 0-127
// Bitmask: Sun=1 Mon=2 Tue=4 Wed=8 Thu=16 Fri=32 Sat=64 — add up the days
// you want (every day = 127, weekdays only = 62, weekends only = 65)
BLYNK_WRITE(V9) {
  feedDayMask = (uint8_t)param.asInt();
  Serial.print("BLYNK_WRITE(V9) fired, feedDayMask=");
  Serial.println(feedDayMask);
}

// App widget: "Portion Size (sec)" — Slider, range 1-10
// How long the motor spins per feed, in seconds (default 1.2s if never set)
BLYNK_WRITE(V10) {
  int seconds = param.asInt();
  if (seconds < 1) seconds = 1;      // guard against 0/negative from a misconfigured widget
  rotateTime = seconds * 1000;
  Serial.print("BLYNK_WRITE(V10) fired, rotateTime=");
  Serial.print(rotateTime);
  Serial.println("ms");
}

// V11: master pause switch — disables all scheduled feeding without
// touching the saved times/days, so resuming needs no re-entry
BLYNK_WRITE(V11) {
  autoFeedEnabled = param.asInt() == 1;
  Serial.print("BLYNK_WRITE(V11) fired, autoFeedEnabled=");
  Serial.println(autoFeedEnabled ? "true" : "false");
  if (!motorRunning) renderIdleScreen();
}

// Read the load cell and push to Blynk V3 — food bowl weight in grams
void updateWeight() {
  if (!scale.is_ready()) return;
  bowlWeight = scale.get_units(5);   // average of 5 reads
  if (bowlWeight < 0) bowlWeight = 0; // clamp noise/drift below zero
  Blynk.virtualWrite(V3, bowlWeight);
  Serial.print("Bowl weight: ");
  Serial.print(bowlWeight, 1);
  Serial.println(" g");
}

// V4: manual re-tare button — zero the scale on demand (bowl must be empty
// when you press it), no re-flash needed
BLYNK_WRITE(V4) {
  if (param.asInt() == 1) {
    // Non-blocking: HX711 read() has no internal timeout, so only tare if
    // the chip is actually signaling ready. A disconnected/unpowered load
    // cell must never be able to hang the board.
    if (scale.wait_ready_timeout(500, 50)) {
      scale.tare();
      scaleReady = true;
      Serial.println("Manual tare via V4");
    } else {
      Serial.println("Manual tare via V4 skipped: HX711 not ready");
    }
    Blynk.virtualWrite(V4, 0);
  }
}

// Poll V0 every 30s via plain HTTP — picks up automation-set values
void pollV0() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://blynk.cloud/external/api/get?token=" BLYNK_AUTH_TOKEN "&V0");
  int code = http.GET();
  String val = "";
  if (code == HTTP_CODE_OK) {
    val = http.getString();
    val.trim();
  }
  http.end();

  Serial.println("Poll V0=" + val);
  if (val == "1") {
    rotateMotor();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  // doReset=false skips HX711's built-in reset(), which otherwise calls a
  // read() that blocks forever (no timeout) if the chip isn't wired/powered
  // yet. Init must never be able to stall setup() before Blynk connects.
  scale.begin(HX711_DT, HX711_SCK, false, false);
  scale.set_scale(LOADCELL_CALIBRATION_FACTOR);
  if (scale.wait_ready_timeout(1000, 100)) {
    scale.tare();   // zero with an empty bowl at boot — make sure bowl is empty on power-up
    scaleReady = true;
    Serial.println("HX711 ready, tared at boot");
  } else {
    Serial.println("HX711 not ready at boot — skipping tare, will retry on demand (V4)");
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Kick off NTP sync now that WiFi is up (IST, no DST). Runs in the
  // background — renderClockScreen() shows "Syncing time..." until it lands.
  configTime(IST_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");

  // Restore saved schedule (all 3 slots, day mask, portion size, pause
  // switch) and clear any leftover motor trigger. Pins the app hasn't been
  // configured with yet simply won't sync anything, leaving the firmware's
  // own defaults in place (every day enabled, 1.2s portion, not paused).
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V5);
  Blynk.syncVirtual(V6);
  Blynk.syncVirtual(V7);
  Blynk.syncVirtual(V8);
  Blynk.syncVirtual(V9);
  Blynk.syncVirtual(V10);
  Blynk.syncVirtual(V11);
  Blynk.virtualWrite(V0, 0);

  renderIdleScreen();

  timer.setInterval(30000L, pollV0);
  timer.setInterval(5000L, updateWeight);      // push weight to V3 every 5s
  timer.setInterval(5000L, advanceIdleScreen); // rotate idle LCD screens
  timer.setInterval(1000L, tickClockScreen);   // keep clock screen fresh
  timer.setInterval(1000L, checkScheduledFeed); // on-device auto-feed check
}

void loop() {
  Blynk.run();
  timer.run();
}
