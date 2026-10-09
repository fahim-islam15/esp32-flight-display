# ESP32-C3 Primary Flight Display (PFD)

A Primary Flight Display on an **ESP32-C3 Super Mini** and a **GC9A01 1.28" 240×240 round IPS LCD**, controlled from a phone web page with a cockpit-style UI (yoke, levers, heading knob). No IMU is needed.

## Features

- Artificial horizon with pitch ladder and roll scale
- Airspeed tape (left), altitude tape (right), heading tape (bottom)
- Phone web UI over the ESP32's own WiFi hotspot
- Pitch (-30 to 30) and roll (-90 to 90) from the yoke, or from the phone sensor (Android Chrome)
- Speed, altitude and heading from levers and a knob
- Auto demo animation on boot

## Files

| File | Purpose |
|---|---|
| `main.cpp` | Complete firmware including the web page |
| `README.md` | These instructions |

## 1. Hardware wiring

| Display pin | ESP32-C3 Super Mini |
|---|---|
| VCC | 3V3 |
| GND | GND |
| RST | GPIO 0 |
| CS | GPIO 1 |
| DC | GPIO 10 |
| SDA (MOSI) | GPIO 3 |
| SCL (SCLK) | GPIO 4 |

- Power the display from **3V3**, not 5V.
- If your module has a BL pin, connect it to 3V3. Many modules have no BL pin.
- Keep SPI wires short and firmly seated.

## 2. Software setup

Pick **one** of the two options.

### Option A: Arduino IDE (worked in testing)

1. Install the **esp32 by Espressif** board package (Boards Manager).
2. Install the library **GFX Library for Arduino** (by moononournation) via Library Manager.
3. Tools settings:
   - Board: **ESP32C3 Dev Module**
   - USB CDC On Boot: **Enabled**
4. Create a new sketch, paste the contents of `main.cpp` into it (it is fine to keep it as `.ino`).

### Option B: VS Code + PlatformIO

1. Install VS Code, then the **PlatformIO IDE** extension.
2. New Project: name `pfd`, board **Espressif ESP32-C3-DevKitM-1**, framework **Arduino**.
3. Replace `platformio.ini`:

```ini
[env:esp32c3]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
lib_deps = moononournation/GFX Library for Arduino
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
```

4. Replace `src/main.cpp` with the provided `main.cpp`.
5. Build (`Ctrl+Alt+B`) and upload (`Ctrl+Alt+U`).

> The newest GFX library needs Arduino-ESP32 core 3.x. The stock `espressif32` platform ships core 2.x and fails with `esp32-hal-periman.h` / `esp_private/periph_ctrl.h` not found. Use the pioarduino platform above, or pin the library to an older version (for example `@ 1.4.9`) on the stock platform.

## 3. Upload

1. Plug in the Super Mini via USB.
2. Upload. If no port is found: unplug, hold **BOOT**, plug in while holding, release, then upload again.
3. Press **RST** after the upload.
4. The display should show the horizon moving on its own (demo mode).

## 4. Connect your phone

1. Join WiFi **PFD**, password `12345678`.
2. Open `http://192.168.4.1` (must be `http`, not `https`).
3. Use the controls:

| Control | Function |
|---|---|
| Yoke (center circle) | Drag for roll (left/right) and pitch (up/down). **CENTER** levels it |
| Left lever | Airspeed, 0 to 300 |
| Right lever | Altitude, 0 to 5000 |
| Heading knob | Drag around the dial, 0 to 359 |
| PITCH/ROLL SENSOR switch | Pitch and roll follow the phone sensor |
| HEADING SENSOR switch | Heading follows the phone compass |
| ZERO P/R | Hold phone upright, tap to set level |
| AUTO DEMO | Back to the animation |

## 5. iPhone vs Android

- **Touch controls (yoke, levers, knob) work on both** iPhone and Android over plain HTTP.
- **Phone sensors** are blocked by browsers on plain HTTP pages.
  - Android Chrome: open `chrome://flags`, enable **Insecure origins treated as secure**, add `http://192.168.4.1`, relaunch Chrome.
  - iPhone Safari: no workaround over HTTP. Use the touch controls, or see the options below.
- Phone drops the connection: turn off **Auto switch to mobile data** for the PFD network, and choose to stay connected when it says "no internet".
- If pitch or roll moves the wrong way in sensor mode, in the page script change `cur.p=90-e.beta` to `cur.p=e.beta-90`, or `cur.r=e.gamma` to `cur.r=-e.gamma`.

Options for real sensor data on iPhone: an HTTPS page plus Bluetooth (BLE) in the Bluefy browser, a sensor-streaming app, or adding an MPU6050 IMU to the ESP32.

## 6. Troubleshooting

| Symptom | Fix |
|---|---|
| Black screen | Recheck pins against the table above. Run a simple color-fill test first. Match labels: SCL = SCK/CLK, SDA = MOSI/DIN |
| Static noise on display | SPI speed too high or canvas allocation failed. This firmware uses 20 MHz and a 30-row strip buffer to avoid both |
| `BLACK` / `RED` not declared | Your GFX version has no short color names. Define colors with `RGB565(r,g,b)` as this code does |
| `periman` / `periph_ctrl.h` errors | Library and core mismatch. See the PlatformIO note in section 2 |
| `MissingPackageManifestError` | Close VS Code, delete `~/.platformio/packages`, `platforms`, `.cache` and the project `.pio`, then reinstall: `platformio pkg install -g -p espressif32` |
| `GFX_SKIP_OUTPUT_BEGIN` not found | In `setup()` replace it with `-1` in `g->begin(...)` |
| WiFi "PFD" not visible or `AP FAILED` | Try another USB cable or port. Lower TX power: `WIFI_POWER_5dBm` or `WIFI_POWER_2dBm` |
| Page won't load | Make sure the address is `http://192.168.4.1`, not `https` |
| Slow screen updates | Raise strip height `SH` (for example 40 or 60) if RAM allows |

## 7. Tuning

- Pitch sensitivity on the display: `K` (pixels per degree)
- Airspeed tape scale: `2.5f` in `tape(14, spd, 2.5f, ...)`
- Altitude tape scale: `0.5f` in `tape(190, alt, 0.5f, ...)`
- WiFi name and password: `WiFi.softAP("PFD", "12345678", ...)`
- Serial Monitor (115200 baud) prints `AP started`, the IP address, and init errors

## 8. Next steps

- Add an MPU6050 for real pitch and roll
- Feed live data from a flight simulator over WiFi UDP
- BLE control for iPhone sensor support
