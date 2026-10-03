#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Solar Powered Pet Feeder"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// Right motor pins (L298N)
#define ENB D5
#define IN3 D6
#define IN4 D7

BlynkTimer timer;

int motorSpeed = 800;    // 0 to 1023
int rotateTime = 1200;   // milliseconds — increase to 1500 or 2000 if not rotating enough

bool motorRunning = false;

void stopMotor() {
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motorRunning = false;
}

void rotateMotor() {
  if (motorRunning) return;
  motorRunning = true;
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, motorSpeed);
  timer.setTimeout(rotateTime, stopMotor);
}

// Blynk button on V0 — set to PUSH mode in the app
BLYNK_WRITE(V0) {
  if (param.asInt() == 1) {
    rotateMotor();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotor();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();
  timer.run();
}
