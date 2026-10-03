/*
  Module 10d - Device health telemetry
  ------------------------------------
  STATUS: reference code, not yet verified on the built feeder.

  Reports how healthy the feeder's connection is, so you can tell from the
  iPhone app whether it is placed too far from the router or is rebooting:

    V15  WiFi signal   dBm  (about -50 great, -70 okay, -80 flaky)
    V16  Uptime        minutes since last boot (drops to 0 after a reset)

  The app can also show Blynk's built-in "Device online/offline" status.
  For a quick check without the app, call
    https://blynk.cloud/external/api/isHardwareConnected?token=YOUR_TOKEN
  which returns plain true or false (use curl -L, it redirects).
*/

#define BLYNK_PRINT Serial
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

BlynkTimer timer;
unsigned int connectCount = 0;

BLYNK_CONNECTED() {
  connectCount++;
  Serial.printf("Connected to Blynk (connection #%u)\n", connectCount);
}

void sendHealth() {
  int rssi = WiFi.RSSI();
  unsigned long uptimeMin = millis() / 60000UL;
  Blynk.virtualWrite(V15, rssi);
  Blynk.virtualWrite(V16, uptimeMin);
  Serial.printf("RSSI %d dBm, uptime %lu min, heap %u\n", rssi, uptimeMin, ESP.getFreeHeap());
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  timer.setInterval(60000L, sendHealth);          // once a minute is plenty
}

void loop() {
  Blynk.run();
  timer.run();
}
