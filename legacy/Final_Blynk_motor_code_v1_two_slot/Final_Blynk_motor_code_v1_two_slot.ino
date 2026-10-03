#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Solar Powered Pet Feeder"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

#define ENB D5
#define IN3 D6
#define IN4 D7

LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

int rotateTime = 1200;
bool motorRunning = false;

// Seconds since midnight (Time Input widget format); -1 = disabled
long morningTime = -1;
long eveningTime = -1;
int lastFedMorningDay = -1;
int lastFedEveningDay = -1;

void stopMotor() {
  digitalWrite(ENB, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motorRunning = false;
  Blynk.virtualWrite(V0, 0);  // reset so next automation trigger fires BLYNK_WRITE
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feeder");
  lcd.setCursor(0, 1);
  lcd.print("Ready");
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

// V1: morning feed time (Time Input widget sends seconds since midnight)
BLYNK_WRITE(V1) {
  morningTime = param.asLong();
  Serial.printf("Morning feed set to %02ld:%02ld\n", morningTime/3600, (morningTime%3600)/60);
}

// V2: evening feed time
BLYNK_WRITE(V2) {
  eveningTime = param.asLong();
  Serial.printf("Evening feed set to %02ld:%02ld\n", eveningTime/3600, (eveningTime%3600)/60);
}

void checkSchedule() {
  time_t now = time(nullptr);
  if (now < 1000000000) return;  // NTP not ready

  struct tm* t = localtime(&now);
  long currentSecs = (long)t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
  int d = t->tm_yday;

  if (morningTime >= 0 && abs(currentSecs - morningTime) < 60 && lastFedMorningDay != d) {
    lastFedMorningDay = d;
    Serial.println("Scheduled morning feed!");
    rotateMotor();
  }
  if (eveningTime >= 0 && abs(currentSecs - eveningTime) < 60 && lastFedEveningDay != d) {
    lastFedEveningDay = d;
    Serial.println("Scheduled evening feed!");
    rotateMotor();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Eastern time (UTC-5 standard, +1hr DST)
  configTime(-5 * 3600, 3600, "pool.ntp.org", "time.nist.gov");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Syncing time...");
  while (time(nullptr) < 1000000000) {
    delay(500);
  }
  Serial.println("NTP synced");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feeder");
  lcd.setCursor(0, 1);
  lcd.print("Ready");

  timer.setInterval(30000L, checkSchedule);
  Blynk.virtualWrite(V0, 0);  // always clear motor button on boot so automations can trigger
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
}

void loop() {
  Blynk.run();
  timer.run();
}
