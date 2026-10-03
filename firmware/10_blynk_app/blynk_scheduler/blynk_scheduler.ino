/*
  Module 10a - iPhone feeding scheduler (Blynk + on-device clock)
  ---------------------------------------------------------------
  This is the scheduling brain of the feeder, on its own. You set feeding
  times in the Blynk iPhone app; the board stores them, watches its own
  NTP-synced clock, and runs the motor at the right moment. It does NOT use
  Blynk's cloud "Automations" feature, so a feeding still happens on time
  even if the phone is off or the app is closed, and changes made in the
  app take effect immediately with no re-flashing.

  Features
    * 3 feeding slots per day (hour + minute each), any slot can be unset
    * Day-of-week mask (feed only on chosen days)
    * Adjustable portion size (motor run time)
    * Master pause switch (keeps your saved times)
    * Manual "Feed Now" button
    * Each slot fires at most once per day

  Blynk datastreams (see ../datastreams.csv and ../README.md for the
  iPhone widget layout):
    V0   Feed Now        button (push), 0-1
    V1   Feed 1 hour     integer 0-23        V2  Feed 1 minute  integer 0-59
    V5   Feed 2 hour     integer 0-23        V6  Feed 2 minute  integer 0-59
    V7   Feed 3 hour     integer 0-23        V8  Feed 3 minute  integer 0-59
    V9   Feed Days       integer 0-127 bitmask:
                           Sun=1 Mon=2 Tue=4 Wed=8 Thu=16 Fri=32 Sat=64
                           every day = 127, weekdays = 62, weekends = 65
    V10  Portion Size    integer 1-10, seconds of motor run
    V11  Auto-Feed On    switch 0-1

  Setup: copy secrets.example.h to secrets.h and fill in your own values.
  Wiring: motor driver ENB D5, IN3 D6, IN4 D7 (see 03_motor).
*/

#define BLYNK_PRINT Serial
#include "secrets.h"            // defines BLYNK_* and WIFI_* BEFORE the Blynk include
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <time.h>

#define ENB D5
#define IN3 D6
#define IN4 D7

#define NUM_SLOTS 3
#define TZ_OFFSET_SEC (5 * 3600 + 30 * 60)         // India (UTC+5:30), no DST
const time_t NTP_SYNCED_THRESHOLD = 1700000000;

BlynkTimer timer;

int slotHour[NUM_SLOTS] = { -1, -1, -1 };          // -1 = slot not set
int slotMin[NUM_SLOTS]  = { -1, -1, -1 };
int slotLastFedYday[NUM_SLOTS] = { -1, -1, -1 };   // once-per-day guard

uint8_t feedDayMask = 0x7F;                        // default: every day
bool autoFeedEnabled = true;
unsigned long portionMs = 1200;
bool motorRunning = false;

// ---------- motor ----------
void stopMotor() {
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motorRunning = false;
  Blynk.virtualWrite(V0, 0);                       // pop the button back up in the app
}

void dispense() {
  if (motorRunning) return;                        // never overlap two feeds
  motorRunning = true;
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  digitalWrite(ENB, HIGH);
  Serial.printf("Dispensing for %lu ms\n", portionMs);
  timer.setTimeout(portionMs, stopMotor);
}

// ---------- schedule check, runs every second ----------
void checkSchedule() {
  if (motorRunning || !autoFeedEnabled) return;

  time_t now = time(nullptr);
  if (now < NTP_SYNCED_THRESHOLD) return;          // clock not real yet

  struct tm t;
  localtime_r(&now, &t);
  if (!(feedDayMask & (1 << t.tm_wday))) return;   // not a feeding day

  for (int i = 0; i < NUM_SLOTS; i++) {
    if (slotHour[i] < 0 || slotMin[i] < 0) continue;
    if (t.tm_hour == slotHour[i] && t.tm_min == slotMin[i] &&
        slotLastFedYday[i] != t.tm_yday) {
      slotLastFedYday[i] = t.tm_yday;
      Serial.printf("Scheduled feed, slot %d\n", i + 1);
      dispense();
      return;                                      // one feed even if two slots match
    }
  }
}

// ---------- Blynk handlers ----------
BLYNK_WRITE(V0)  { if (param.asInt() == 1) dispense(); }

BLYNK_WRITE(V1)  { slotHour[0] = param.asInt(); }
BLYNK_WRITE(V2)  { slotMin[0]  = param.asInt(); }
BLYNK_WRITE(V5)  { slotHour[1] = param.asInt(); }
BLYNK_WRITE(V6)  { slotMin[1]  = param.asInt(); }
BLYNK_WRITE(V7)  { slotHour[2] = param.asInt(); }
BLYNK_WRITE(V8)  { slotMin[2]  = param.asInt(); }

BLYNK_WRITE(V9)  { feedDayMask = (uint8_t)param.asInt(); }

BLYNK_WRITE(V10) {
  int sec = param.asInt();
  if (sec < 1) sec = 1;                            // guard against a 0 from the widget
  portionMs = sec * 1000UL;
}

BLYNK_WRITE(V11) { autoFeedEnabled = (param.asInt() == 1); }

// When the board (re)connects, ask the cloud for the last saved value of each
// setting so schedules survive a power cut.
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1, V2, V5, V6, V7, V8, V9, V10, V11);
  Blynk.virtualWrite(V0, 0);
}

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotor();                                     // safe state

  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  configTime(TZ_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");

  timer.setInterval(1000L, checkSchedule);
}

void loop() {
  Blynk.run();
  timer.run();
}
