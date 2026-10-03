/*
  Module 01a - WiFi connection (NodeMCU ESP8266)
  ----------------------------------------------
  Connects to WiFi with a timeout, prints link details, and reconnects with
  backoff if the router drops. Does NOT use Blynk, so it isolates "can the
  board reach my router?" from every other possible problem.

  Setup:
    1. Copy firmware/secrets.example.h to firmware/secrets.h, fill it in, run firmware/setup_secrets.sh.
    2. Board: NodeMCU 1.0 (ESP-12E Module), upload speed 115200.

  IMPORTANT: the ESP8266 only has a 2.4 GHz radio. If your router broadcasts
  two names (e.g. "MyWifi" and "MyWifi_5g"), you must use the 2.4 GHz one.
  Use 01_network/wifi_scan to see which networks the board can actually see.
*/

#include <ESP8266WiFi.h>
#include "secrets.h"

const uint32_t CONNECT_TIMEOUT_MS = 20000;   // give up on one attempt after 20 s
const uint32_t RETRY_INTERVAL_MS  = 10000;   // wait between attempts

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);       // don't wear out flash re-saving credentials
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting");
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > CONNECT_TIMEOUT_MS) {
      Serial.println("\nTimed out.");
      return false;
    }
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  return true;
}

void printLinkInfo() {
  Serial.println("--- WiFi connected ---");
  Serial.print("IP address : "); Serial.println(WiFi.localIP());
  Serial.print("Gateway    : "); Serial.println(WiFi.gatewayIP());
  Serial.print("MAC        : "); Serial.println(WiFi.macAddress());
  Serial.print("Channel    : "); Serial.println(WiFi.channel());
  Serial.print("Signal     : "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nModule 01a: wifi_connect");
  if (connectWiFi()) printLinkInfo();
}

void loop() {
  static uint32_t lastTry = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastTry > RETRY_INTERVAL_MS) {
    lastTry = millis();
    Serial.println("WiFi lost, retrying...");
    if (connectWiFi()) printLinkInfo();
  }
}
