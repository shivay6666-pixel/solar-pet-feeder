/*
  Module 10c - iPhone push notifications (Blynk events)
  -----------------------------------------------------
  STATUS: reference code, not yet verified on the built feeder.

  Shows how the feeder tells your phone something needs attention: feeding
  done, food low, battery low, motor jam. The rest of the project (battery
  monitor, load cell, jam detection) calls notify() when its condition trips.

  Blynk console setup (Template -> Events tab). Create one event per row,
  using exactly these Event Codes, and turn on "Push notification":

      Event code     Suggested name      Type
      feed_done      Feeding complete    Info
      low_food       Food tray low       Warning
      low_battery    Battery low         Warning
      jam            Motor jam           Critical

  Then enable notifications for the device in the iPhone app.

  Blynk limits how often one event may fire, so notify() keeps its own
  per-event cooldown. Low-food and low-battery repeat every few hours, not
  every few seconds.

  This demo has a button on V19 "Send test alert" that cycles through the
  four events so you can check each one reaches the phone.
*/

#define BLYNK_PRINT Serial
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

struct EventDef {
  const char* code;
  unsigned long cooldownMs;
  unsigned long lastSent;
};

EventDef events[] = {
  { "feed_done",   0UL,                    0 },   // always send
  { "low_food",    4UL * 3600UL * 1000UL,  0 },   // at most every 4 h
  { "low_battery", 4UL * 3600UL * 1000UL,  0 },
  { "jam",         10UL * 60UL * 1000UL,   0 },   // at most every 10 min
};
const int NUM_EVENTS = sizeof(events) / sizeof(events[0]);

// Sends a push notification unless the same event fired too recently.
// Returns true if it was actually sent.
bool notify(int idx, const char* message) {
  EventDef& e = events[idx];
  if (e.lastSent != 0 && millis() - e.lastSent < e.cooldownMs) return false;
  e.lastSent = millis() ? millis() : 1;           // 0 means "never sent"
  Blynk.logEvent(e.code, message);
  Serial.printf("Event sent: %s - %s\n", e.code, message);
  return true;
}

BLYNK_WRITE(V19) {                                // test button cycles events
  if (param.asInt() != 1) return;
  static int next = 0;
  const char* msgs[] = {
    "Dinner was served.",
    "Food tray is nearly empty. Please refill.",
    "Battery is low. Check the solar panel.",
    "Motor jammed and could not clear itself."
  };
  events[next].lastSent = 0;                      // let the test bypass cooldown
  notify(next, msgs[next]);
  next = (next + 1) % NUM_EVENTS;
  Blynk.virtualWrite(V19, 0);
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
}

void loop() {
  Blynk.run();
}
