/*
  Module 03a - DC motor test through an L298N driver
  --------------------------------------------------
  Spins the dispensing motor forward, stops, reverses, stops, forever.
  No WiFi, no Blynk. Use this first to prove the motor and wiring work.

  Wiring (channel B of the L298N):
    NodeMCU D5 -> ENB   (enable / speed)
    NodeMCU D6 -> IN3
    NodeMCU D7 -> IN4
    NodeMCU GND -> L298N GND   (grounds MUST be shared)
    Motor -> OUT3 / OUT4
    Motor supply (+) -> L298N 12V terminal, (-) -> GND

  L298N gotchas learned the hard way:
    * Remove the ENB jumper cap if you want PWM speed control from D5.
    * The separate logic-power jumper (near the 12V/GND screw terminals)
      must be fitted, or the chip's logic has no power and the motor only
      "clicks" once instead of spinning.
    * Swapping the two motor wires, or swapping IN3/IN4 values, reverses
      the "forward" direction.
*/

#define ENB D5
#define IN3 D6
#define IN4 D7

const int  SPEED_PWM  = 1023;   // 0-1023 on ESP8266 (1023 = full speed)
const int  FORWARD_MS = 1200;   // one normal feeding portion
const int  REVERSE_MS = 1500;   // unjam move

void motorStop() {
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void motorForward(int pwm) {
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, pwm);
}

void motorReverse(int pwm) {
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, pwm);
}

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  motorStop();
  Serial.println("\nModule 03a: motor_basic_test");
}

void loop() {
  Serial.println("Forward");
  motorForward(SPEED_PWM);
  delay(FORWARD_MS);

  motorStop();
  Serial.println("Stop");
  delay(2000);

  Serial.println("Reverse");
  motorReverse(SPEED_PWM);
  delay(REVERSE_MS);

  motorStop();
  Serial.println("Stop (waiting 5 s)");
  delay(5000);
}
