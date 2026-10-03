# iPhone feeding scheduler (Blynk)

The feeder is controlled from the **Blynk IoT** iPhone app. The schedule lives
on the feeder itself (see `blynk_scheduler/`), not in Blynk's cloud
"Automations", so feeding still happens on time with the phone off, and
changing a setting in the app takes effect immediately with no re-flashing.

## One-time setup

1. Create a free account at blynk.cloud and open **Developer Zone > Templates >
   New Template**. Name it `Solar Powered Pet Feeder`, hardware `ESP8266`,
   connection `WiFi`.
2. Under **Datastreams**, add the virtual pins in [`datastreams.csv`](datastreams.csv).
   Start with V0 to V12; the rest are for the optional modules.
   *Datastream names accept letters and numbers only (no spaces or hyphens),
   which is why they read `Feed1Hour`. The widget titles in the app can have
   spaces.*
3. Create a **Device** from the template. Copy the Template ID and Auth Token
   into your local `secrets.h` (never commit it).
4. Install the **Blynk IoT** app on the iPhone, log in, and the template's
   web dashboard appears there. Add the widgets below.

## iPhone app layout

| Widget (iOS name) | Title | Datastream | Setting |
|---|---|---|---|
| Button | Feed Now | V0 | Mode: Push, on = 1, off = 0 |
| Value Display | Food in Tray (g) | V3 | read-only |
| Button | Tare Scale | V4 | Push |
| Numeric Input | Feed 1 Hour / Minute | V1 / V2 | 0-23 / 0-59 |
| Numeric Input | Feed 2 Hour / Minute | V5 / V6 | 0-23 / 0-59 |
| Numeric Input | Feed 3 Hour / Minute | V7 / V8 | 0-23 / 0-59 |
| Numeric Input | Feed Days | V9 | 0-127 (see table) |
| Slider | Portion Size (sec) | V10 | 1-10 |
| Switch | Auto-Feed On | V11 | on = 1, off = 0 |
| Button | Reverse / Unjam | V12 | Push |

Leave a slot's hour and minute unset to switch that slot off.

### Feed Days (V9) is a bitmask

Add up the days you want:

| Sun | Mon | Tue | Wed | Thu | Fri | Sat |
|---|---|---|---|---|---|---|
| 1 | 2 | 4 | 8 | 16 | 32 | 64 |

Every day = **127** (default), weekdays = **62**, weekends = **65**,
Mon/Wed/Fri = **42**.

## Sketches in this folder

| Sketch | What it shows | Status |
|---|---|---|
| `blynk_manual_feed` | Smallest project: one button runs the motor. Start here. | proven in full firmware |
| `blynk_scheduler` | Full scheduler: 3 slots, days, portion, pause, NTP clock | proven in full firmware |
| `blynk_notifications` | Push notifications for fed / low food / low battery / jam | reference, untested |
| `blynk_device_health` | WiFi signal and uptime reporting | reference, untested |

## Troubleshooting

* **Device stays offline:** the ESP8266 cannot join 5 GHz WiFi. Use the 2.4 GHz
  network name, not the one ending in `_5g`. `01_network/wifi_scan` shows
  what the board can see.
* **Quick online check without the app:**
  `curl -L "https://blynk.cloud/external/api/isHardwareConnected?token=YOUR_TOKEN"`
  prints `true` or `false`.
* **Times fire at the wrong hour:** the time zone is set by `TZ_OFFSET_SEC` in
  the sketch (India = 5h30m). Change it for your location.
