# Solar-Powered Automatic Pet Feeder

Firmware and hardware documentation for an off-grid, solar-powered automatic
feeder built for outdoor animal shelter use. Built as part of a research
project on autonomous outdoor feeding systems for shelter dogs, developed in
collaboration with the Tako Foundation.

- NodeMCU ESP8266 microcontroller, 10 W solar panel + PWM charge controller + Li-ion pack
- DC gearbox motor through an L298N driver, turning a screw to dispense kibble
- HX711 load cell under the tray (food weight)
- 16x2 I2C LCD status display
- Blynk IoT iPhone app: manual feed, 3 daily feeding times, day selection, portion size, pause
- NTP clock sync, so the feeder keeps its own schedule without a real-time-clock chip

System block diagram: [docs/block_diagram.png](docs/block_diagram.png).
Wiring: [docs/pin_map.md](docs/pin_map.md).

## Code layout

Every folder is a small, self-contained Arduino sketch you can upload and test
on its own. `00_full_system` is the integrated firmware that runs the real feeder.

| Folder | Sketch | What it does | Status |
|---|---|---|---|
| `00_full_system` | `FeederFirmware` | Everything together: motor, Blynk, scheduler, weight, LCD, unjam | Running on the real feeder |
| `01_network` | `wifi_connect` | WiFi with timeout and auto-retry | Reference |
| | `wifi_scan` | List visible networks (proves 2.4 GHz vs 5 GHz) | Reference |
| | `ntp_time_sync` | Internet clock, India time zone | Proven in full firmware |
| `02_esp8266_board` | `board_info` | Chip info, reset reason, LED blink, A0 read | Reference |
| `03_motor` | `motor_basic_test` | Forward / stop / reverse loop | Proven in full firmware |
| | `motor_serial_control` | Drive motor from Serial Monitor, tune PWM and portion time | Reference |
| `04_ir_sensor` | `ir_presence_detect` | Animal-at-bowl detection with visit timing | Reference, not yet wired |
| `05_food_tray_weight` | `hx711_calibration` | Find the load cell calibration factor | Reference (factor in the full firmware is still a placeholder) |
| | `hx711_weight_read` | Tray weight, tare, low-food flag | Proven in full firmware |
| `06_buzzer` | `buzzer_patterns` | Non-blocking alert beep patterns | Reference, not yet wired |
| `07_battery_monitor` | `battery_monitor` | Battery voltage, % and charging state | Reference, not yet wired |
| `08_jam_sensor_FUTURE` | `motor_jam_detect` | Current-based jam detection, auto-reverse | Future work, untested |
| `09_lcd_display` | `i2c_scanner` | Find the LCD I2C address | Reference |
| | `lcd_status_demo` | The rotating status screens with fake data | Proven in full firmware |
| `10_blynk_app` | `blynk_manual_feed` | Smallest Blynk project: one Feed Now button | Proven in full firmware |
| | `blynk_scheduler` | iPhone-set scheduler: 3 slots, days, portion, pause | Proven in full firmware |
| | `blynk_notifications` | Push alerts: fed, low food, low battery, jam | Reference |
| | `blynk_device_health` | WiFi signal and uptime to the app | Reference |

"Proven in full firmware" means the same logic runs on the real feeder inside `00_full_system`; the standalone sketch is a smaller extract of it. "Reference" means it compiles for the NodeMCU (checked with `arduino-cli`) and follows the
known-good patterns in the full firmware, but it has not been run on its own. Modules 04, 06, 07 and 08 are still to be built
and merged into the full firmware.

`legacy/` holds earlier versions kept for the project history.

## Quick start

1. Install the Arduino IDE (or `arduino-cli`) and the **ESP8266** board package.
   Board: *NodeMCU 1.0 (ESP-12E Module)*, upload speed 115200.
2. Libraries (Library Manager): **Blynk**, **LiquidCrystal_I2C**, **HX711** (Rob Tillaart).
3. Open a sketch from a folder above. For any sketch containing `secrets.example.h`:
   copy it to `secrets.h` in the same folder and fill in your own WiFi name,
   password and Blynk values. `secrets.h` is git-ignored and must never be committed.
4. Upload. Open the Serial Monitor at **115200** baud.

Suggested bring-up order on new hardware: `02` board, `01` wifi scan and connect,
`09` LCD, `03` motor, `05` load cell, then `10` Blynk, then `00` full system.

## Hard-won lessons

- **The ESP8266 cannot use 5 GHz WiFi.** If the router has a `_5g` twin of your network name, use the plain 2.4 GHz one.
- **L298N motor "clicks" but does not spin:** the logic-power jumper is missing.
- **HX711 library can hang forever** if the chip is unplugged. Every read here is guarded with a timeout so the board still reaches WiFi.
- **Blynk Automations were unreliable and hard to debug**, so scheduling runs on the board itself.
- **Quick connectivity check:** `curl -L "https://blynk.cloud/external/api/isHardwareConnected?token=YOUR_TOKEN"` prints `true`/`false`.

## License

MIT, see [LICENSE](LICENSE).
