/*
  Module 01c - NTP clock sync (NodeMCU ESP8266)
  ---------------------------------------------
  The feeder has no real-time-clock chip. It gets the time from the internet
  after WiFi connects, which is what makes on-device feeding schedules work.

  Uses the ESP8266 core's built-in configTime(), so no extra library needed.

  Setup: copy secrets.example.h to secrets.h and fill in your own values.
*/

#include <ESP8266WiFi.h>
#include <time.h>
#include "secrets.h"

// India Standard Time = UTC+5:30, no daylight saving.
// For US Eastern use: configTime(-5 * 3600, 3600, ...) (UTC-5, +1h DST).
#define TZ_OFFSET_SEC (5 * 3600 + 30 * 60)
#define DST_OFFSET_SEC 0

// Anything earlier than this means the clock hasn't synced yet
// (the ESP8266 boots at 1970).
const time_t NTP_SYNCED_THRESHOLD = 1700000000;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nModule 01c: ntp_time_sync");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
  }
  Serial.println("\nWiFi connected, requesting time...");

  configTime(TZ_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");

  uint32_t start = millis();
  while (time(nullptr) < NTP_SYNCED_THRESHOLD) {
    if (millis() - start > 20000) {
      Serial.println("NTP sync timed out (check internet access).");
      return;
    }
    delay(250);
  }
  Serial.println("Clock synced.");
}

void loop() {
  time_t now = time(nullptr);
  if (now >= NTP_SYNCED_THRESHOLD) {
    struct tm t;
    localtime_r(&now, &t);
    char buf[40];
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &t);
    Serial.printf("%s  (tm_wday=%d, tm_yday=%d)\n", buf, t.tm_wday, t.tm_yday);
  } else {
    Serial.println("Not synced yet");
  }
  delay(1000);
}
