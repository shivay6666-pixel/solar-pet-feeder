/*
  Module 01b - WiFi scanner (NodeMCU ESP8266)
  -------------------------------------------
  Lists every network the board can hear, strongest first. Needs no
  credentials, so there is no secrets.h for this sketch.

  Why this exists: the ESP8266 cannot see 5 GHz networks at all. If your
  2.4 GHz network name is missing from this list, the board is too far from
  the router (or the router has the 2.4 GHz band turned off).
*/

#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nModule 01b: wifi_scan");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop() {
  Serial.println("Scanning...");
  int n = WiFi.scanNetworks();

  if (n <= 0) {
    Serial.println("No networks found.");
  } else {
    Serial.printf("%d networks found:\n", n);
    Serial.println(" #  RSSI  CH  Security  SSID");
    for (int i = 0; i < n; i++) {
      Serial.printf("%2d  %4d  %2d  %-8s  %s\n",
                    i + 1,
                    WiFi.RSSI(i),
                    WiFi.channel(i),
                    WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "secured",
                    WiFi.SSID(i).length() ? WiFi.SSID(i).c_str() : "(hidden)");
    }
  }
  WiFi.scanDelete();
  Serial.println();
  delay(10000);
}
