# Solar-Powered Automatic Dog Feeder

Firmware and hardware documentation for an off-grid, solar-powered automatic pet feeder built for outdoor animal shelter use. Built as part of a research project on autonomous outdoor feeding systems for shelter dogs, developed in collaboration with the Tako Foundation.

## Overview

- NodeMCU ESP8266 microcontroller
- 10 W monocrystalline solar panel, PWM charge controller, and Li-ion battery pack for off-grid power
- DC gearbox motor driven through an L298N-style H-bridge, actuating a rotary valve to dispense kibble
- 16x2 I2C LCD for status display
- Blynk IoT app for manual feed triggering and scheduled morning/evening feed times, with NTP time sync for accurate scheduling

## Hardware

| Component | Notes |
|---|---|
| NodeMCU ESP8266 | main controller |
| L298N motor driver | ENB → D5, IN3 → D6, IN4 → D7 |
| 16x2 I2C LCD | I2C address `0x27` |
| 10 W monocrystalline solar panel | primary power source |
| Li-ion battery pack | charged via PWM solar charge controller |
| DC-DC buck converter | steps battery voltage down to 5 V logic supply |

See `docs/block_diagram.png` for the full system block diagram covering power, dispensing, control, and communication paths.

## Blynk setup

1. Create a Blynk template (the firmware defaults to naming it "Solar Powered Pet Feeder", edit as you like).
2. Add these datastreams:
   - `V0` "Motor Button" (Integer, 0-1) - manual feed trigger, also used to fire scheduled automations
   - `V1` morning feed time (Time Input widget, sends seconds since midnight)
   - `V2` evening feed time (Time Input widget, sends seconds since midnight)
3. Copy your template ID and device auth token from the Blynk console.

## Firmware setup

1. Open `Final_Blynk_motor_code/Final_Blynk_motor_code.ino` in the Arduino IDE.
2. Install libraries via Library Manager: Blynk (ESP8266 core), LiquidCrystal_I2C.
3. Replace the placeholders at the top of the file with your own values:

   ```cpp
   #define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
   #define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"
   ...
   char ssid[] = "YOUR_WIFI_SSID";
   char pass[] = "YOUR_WIFI_PASSWORD";
   ```

   Keep your real credentials out of version control. If you fork this, add your filled-in copy to a local, gitignored file rather than committing real values over these placeholders.
4. Board settings: NodeMCU 1.0 (ESP-12E Module), upload speed 115200.

## License

MIT, see [LICENSE](LICENSE).
