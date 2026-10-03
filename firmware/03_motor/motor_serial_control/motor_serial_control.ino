/*
  Module 03b - Motor control from the Serial Monitor
  --------------------------------------------------
  Same wiring as motor_basic_test. Open the Serial Monitor at 115200 baud
  (line ending: "No line ending" or "Newline") and type single letters:

    f   run forward until stopped      +   faster (PWM +100)
    r   run reverse until stopped      -   slower (PWM -100)
    s   stop                           p   feed one portion (forward, timed)
    ?   show help

  Handy for finding the PWM value and run time that dispense one good
  portion of kibble, and for testing a jam-clearing reverse.
*/

#define ENB D5
#define IN3 D6
#define IN4 D7

int pwm = 1023;                 // current speed, 0-1023
int portionMs = 1200;           // duration used by the 'p' command
bool running = false;
unsigned long stopAt = 0;       // 0 = no timed stop pending

void applyStop() {
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  running = false;
  stopAt = 0;
}

void run(bool forward) {
  digitalWrite(IN3, forward ? HIGH : LOW);
  digitalWrite(IN4, forward ? LOW : HIGH);
  analogWrite(ENB, pwm);
  running = true;
}

void help() {
  Serial.println("f=forward r=reverse s=stop p=portion +/-=speed ?=help");
}

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  applyStop();
  Serial.println("\nModule 03b: motor_serial_control");
  help();
}

void loop() {
  if (stopAt && (long)(millis() - stopAt) >= 0) {
    applyStop();
    Serial.println("Portion done.");
  }

  if (!Serial.available()) return;
  char c = Serial.read();

  switch (c) {
    case 'f': run(true);  Serial.println("forward");  break;
    case 'r': run(false); Serial.println("reverse");  break;
    case 's': applyStop(); Serial.println("stopped"); break;
    case 'p':
      run(true);
      stopAt = millis() + portionMs;
      Serial.printf("portion: %d ms at PWM %d\n", portionMs, pwm);
      break;
    case '+': pwm = min(1023, pwm + 100); Serial.printf("PWM %d\n", pwm); if (running) analogWrite(ENB, pwm); break;
    case '-': pwm = max(200,  pwm - 100); Serial.printf("PWM %d\n", pwm); if (running) analogWrite(ENB, pwm); break;
    case '?': help(); break;
    default: break;   // ignore newlines etc.
  }
}
