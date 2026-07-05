#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Solar Powered Pet Feeder"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

#define ENB D5
#define IN3 D6
#define IN4 D7

#define TOKEN "YOUR_BLYNK_AUTH_TOKEN"

LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

int rotateTime = 1200;
bool motorRunning = false;

// Next feed time set from Blynk widgets V1 (hour 0-23) and V2 (minute 0-59)
int nextHour = -1;
int nextMin  = -1;

void updateIdleLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feeder Ready");
  lcd.setCursor(0, 1);
  if (nextHour >= 0 && nextMin >= 0) {
    // Format: "Next: 5:30 PM"
    int h = nextHour;
    String ampm = (h >= 12) ? "PM" : "AM";
    if (h == 0) h = 12;
    else if (h > 12) h -= 12;
    char buf[16];
    sprintf(buf, "Next: %d:%02d %s", h, nextMin, ampm.c_str());
    lcd.print(buf);
  } else {
    lcd.print("No time set");
  }
}

void stopMotor() {
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motorRunning = false;
  Blynk.virtualWrite(V0, 0);
  updateIdleLCD();
}

void rotateMotor() {
  if (motorRunning) return;
  motorRunning = true;
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  digitalWrite(ENB, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Feeding...");
  timer.setTimeout(rotateTime, stopMotor);
}

// V0: manual feed button
BLYNK_WRITE(V0) {
  if (param.asInt() == 1) rotateMotor();
}

// V1: next feed hour (0-23) — set this to match your automation time
BLYNK_WRITE(V1) {
  nextHour = param.asInt();
  if (!motorRunning) updateIdleLCD();
}

// V2: next feed minute (0-59)
BLYNK_WRITE(V2) {
  nextMin = param.asInt();
  if (!motorRunning) updateIdleLCD();
}

// Poll V0 every 30s via plain HTTP — picks up automation-set values
void pollV0() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://blynk.cloud/external/api/get?token=" TOKEN "&V0");
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

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Restore saved next-feed time and clear any leftover motor trigger
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.virtualWrite(V0, 0);

  updateIdleLCD();

  timer.setInterval(30000L, pollV0);
}

void loop() {
  Blynk.run();
  timer.run();
}
