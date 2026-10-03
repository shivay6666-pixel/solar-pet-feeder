/*
  Module 10b - Blynk "Feed Now" button (the smallest working Blynk project)
  -------------------------------------------------------------------------
  One button in the iPhone app runs the motor for a set time. Start here
  when setting up Blynk for the first time: if this works, WiFi, the auth
  token, and the motor wiring are all good.

  Blynk setup:
    1. In the Blynk console create a template (name: "Solar Powered Pet
       Feeder") with a datastream  V0  "Motor Button", Integer, min 0, max 1.
    2. Add a Button widget on V0, mode PUSH.
    3. Create a device from the template and copy its Template ID and
       Auth Token into secrets.h (copy secrets.example.h first).

  Wiring: ENB D5, IN3 D6, IN4 D7 (see 03_motor).

  Tip: if "nothing happens", add Serial prints in the handler to tell apart
  "the press never reached the board" from "the board never moved the motor".
*/

#define BLYNK_PRINT Serial
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define ENB D5
#define IN3 D6
#define IN4 D7

const unsigned long RUN_MS = 1200;
BlynkTimer timer;
bool running = false;

void stopMotor() {
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  running = false;
  Serial.println("Motor stopped");
}

BLYNK_WRITE(V0) {
  Serial.printf("V0 received: %d\n", param.asInt());
  if (param.asInt() == 1 && !running) {
    running = true;
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    digitalWrite(ENB, HIGH);
    Serial.println("Motor running");
    timer.setTimeout(RUN_MS, stopMotor);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotor();
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
}

void loop() {
  Blynk.run();
  timer.run();
}
