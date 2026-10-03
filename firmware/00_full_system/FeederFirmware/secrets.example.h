// secrets.example.h
// ------------------------------------------------------------------
// 1. Copy this file to "secrets.h" in the same folder as the sketch.
// 2. Replace the placeholders with your own values.
// 3. NEVER commit secrets.h (it is listed in .gitignore).
// ------------------------------------------------------------------

// WiFi. The ESP8266 supports 2.4 GHz networks ONLY (not the "_5g" one).
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// Blynk (console -> Developer Zone -> your template / device -> Device Info).
// These three defines must appear before the Blynk #include in the sketch.
#define BLYNK_TEMPLATE_ID   "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Solar Powered Pet Feeder"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"
