# Beyblade X RPM Analyzer

An ESP32-S3-Zero launch-speed analyzer with an analog QRE1113 optical sensor and a 200 × 200 e-paper display. The sensor reads a half-white, half-black disk (one period per revolution). The firmware detects launches automatically, calculates average and peak RPM, and stores the last 20 results.

[Project website](https://luciandraw.github.io/Beyblade-Dragoon-RPM/) · [3D assembly](https://luciandraw.github.io/Beyblade-Dragoon-RPM/model.html) · [Wiring diagram](https://luciandraw.github.io/Beyblade-Dragoon-RPM/wiring.html)

## Website and firmware

The English website includes an interactive wiring diagram, a 3D viewer with exploded assembly and clean wireframe modes, and a browser-based USB installer.

## Pinout

| Signal | ESP32-S3-Zero |
|---|---|
| QRE1113 OUT through 1 kΩ | GPIO1 (ADC1_CH0) |
| E-paper RES | GPIO4 |
| E-paper D/C | GPIO5 |
| E-paper SCL | GPIO6 |
| E-paper BUSY | GPIO7 |
| E-paper SDA | GPIO8 |
| E-paper CS | GPIO9 |
| Blade-present button → GND | GPIO10 |
| Central button → GND | GPIO11 |

Power the QRE1113 and e-paper from `3V3` in this build, with a common ground. Display `SCL` and `SDA` mean **SPI SCK and MOSI**, not I²C. Keep the display in 4-line SPI mode.

Connect the LiPo to `B+`/`B-` on the charge-and-boost module. Before connecting the ESP32, use a multimeter to adjust the output between `VO+` and `VO-` to **5.0 V**. Connect `VO+` through the power switch to ESP32 `5V`, and `VO-` directly to GND. USB-C charging remains available with the device switched off. Keep the power switch off when connecting the ESP32 to a computer over USB.

## Arduino IDE

1. Install **esp32 by Espressif Systems** and **GxEPD2 by Jean-Marc Zingg**. The packaged export uses ESP32 core **3.3.11**.
2. Open [main/main.ino](main/main.ino) and select `Waveshare ESP32-S3-Zero`.
3. Select USB Mode `Hardware CDC and JTAG`, `USB CDC On Boot: Enabled`, Flash Mode `QIO 80MHz`, PSRAM `Disabled` (matching the tested build), and Partition Scheme `Default 4MB with spiffs`.
4. When switching from another board profile for the first time, enable `Erase All Flash Before Sketch Upload`, upload, then return it to `Disabled`. This clears saved data.
5. Select the COM port, upload the sketch, and open Serial Monitor at 115200 baud if diagnostics are needed.

The project also includes a PlatformIO configuration: `platformio.ini` sets `src_dir = main`. The packaged release is the Arduino IDE export, not a separately verified PlatformIO build.

## Operation

Startup uses thresholds measured on the assembled device and stored in `config.h`; a separate calibration step is no longer required. Manual calibration remains available if sensor position or lighting changes. In `SENSOR ALIGN`, adjust the gap to approximately 0.5–1.5 mm and shield the sensor from ambient light.

The device initially shows `INSERT BLADE`. The blade-present button on GPIO10 must connect to GND when a blade is installed; the display then shows `READY`. The armed state is latched, so releasing the button during launch does not cancel measurement.

A launch starts after three same-direction QRE1113 transitions within 200 ms. Measurement ends after 250 ms without transitions. This limits very slow rotation measurements regardless of the filter's 10 RPM lower bound. The display and NVS are not updated during measurement. Wait for the READY refresh to finish before pulling: synchronous e-paper refresh blocks ADC processing for approximately 1.5 seconds.

### Central button

- Short press: open the menu or move to the next item.
- Double-click (up to 350 ms between clicks): confirm selection.
- Hold approximately 1 second: go back or recalibrate from READY.
- Hold approximately 3 seconds: open the service screen.

The service screen does not refresh automatically, so e-paper updates do not interfere with fast transition counting. Pull the launcher, then press briefly to refresh readings. Hold approximately one second to exit.

The menu includes launch history, the latest launch graph, calibration, and history clearing with confirmation.

## Configuration and measurement assumptions

Parameters in [main/config.h](main/config.h) include GPIO assignments, ADC rate, contrast, hysteresis, `SENSOR_INVERTED`, filters, RPM limits, periods per revolution, and `DRIVE_RATIO`.

The calculation uses LOW → HIGH transitions only:

```text
RPM = 60000000 × DRIVE_RATIO / (periodUs × ENCODER_PERIODS_PER_REV)
```

For the half-white, half-black disk, there is one period per revolution and the drive ratio is 1:1.

ADC sampling uses ESP-IDF continuous DMA configured for 80 kHz. This is a configured rate, not an independently measured rate. If initial DMA setup fails, sampling falls back to slower `analogRead()`.

Measurement stability measures:

- ADC filtering and hysteresis with 75 µs transition confirmation. The timestamp is recorded at the start of confirmation.
- An impossibly early pulse does not shift the start of the next period.
- Sharp acceleration is not rejected solely because of a percentage RPM jump.
- A DMA timestamp slightly ahead of the clock does not trigger a false timeout.
- `PEAK` is the median of the three fastest individual revolutions; with two revolutions, it uses the maximum. Current speed and the graph use a median of up to five periods. `AVG` uses the total duration of accepted periods.
- A 64-frame DMA buffer. Overflow, read errors, or long pauses break the stream: unfinished measurements are reset, and no revolution is calculated across missing data.

The firmware does not automatically double RPM after a long period: a single optical channel cannot unambiguously distinguish a missed revolution from slowing.


