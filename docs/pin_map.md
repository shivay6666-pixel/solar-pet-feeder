# Pin map and wiring summary (NodeMCU ESP8266)

| NodeMCU pin | GPIO | Connected to | Module | Notes |
|---|---|---|---|---|
| D1 | 5 | LCD SCL (I2C) | 09 | shared I2C bus (also future ADS1115) |
| D2 | 4 | LCD SDA (I2C) | 09 | |
| D3 | 0 | HX711 DT | 05 | must be HIGH at boot, HX711 idles high, so OK |
| D4 | 2 | HX711 SCK | 05 | also the on-board LED, so it flickers |
| D5 | 14 | L298N ENB | 03 | PWM speed / enable |
| D6 | 12 | L298N IN3 | 03 | direction |
| D7 | 13 | L298N IN4 | 03 | direction |
| D0 | 16 | IR sensor OUT | 04 | plain input, no interrupts |
| D8 | 15 | Buzzer (via NPN) | 06 | must be LOW at boot, keep the transistor base resistor |
| A0 | ADC0 | Battery divider | 07 | 0-3.2 V max |

Modules 04, 06, 07 and 08 are reference designs that have not been
wired up on the finished feeder yet.

## Power

* 10 W solar panel > PWM charge controller > Li-ion pack > buck converter > 5 V
  for the NodeMCU, LCD and logic. Motor runs from the pack through the L298N.
* All grounds (NodeMCU, L298N, sensors, battery) must be common.
* If a USB power bank is used during testing, it may switch itself off because
  the NodeMCU draws too little current. A dummy load resistor in parallel across
  its output keeps it awake.

## Motor driver (L298N) checklist

* Logic-power jumper fitted (otherwise the motor only clicks).
* ENB jumper removed if PWM speed control is wanted.
* Motor wires swapped = direction reversed.
