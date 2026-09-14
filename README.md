# Beyblade X RPM Analyzer

An ESP32-S3-Zero launch-speed analyzer with an analog QRE1113 optical sensor and a 200 × 200 e-paper display. The sensor reads a half-white, half-black disk (one period per revolution). The firmware detects launches automatically, calculates average and peak RPM, and stores the last 20 results.

[Project website](https://luciandraw.github.io/Beyblade-Dragoon-RPM/) · [3D assembly](https://luciandraw.github.io/Beyblade-Dragoon-RPM/model.html) · [Wiring diagram](https://luciandraw.github.io/Beyblade-Dragoon-RPM/wiring.html)

## Website and firmware

The English website includes an interactive wiring diagram, a 3D viewer with exploded assembly and clean wireframe modes, and a browser-based USB installer.

## Components and purchase links

Choose **one sourcing option per component**. These are separate shopping lists, not a combined basket. Check dimensions, connector pitch and pack size before ordering. Prices are indicative; see the [cost summary](#component-cost-summary).

### Poland

Prices below use Polish suppliers only. No Polish source was supplied for the assembly board.

| Component | Purchase unit | Buy in Poland | PLN |
|---|---|---|---:|
| ESP32-S3-Zero | 1 board | [Elektroweb][pl-esp] / [Allegro alternative](https://allegro.pl/produkt/sterownik-elektroweb-esp32-s3-zero-waveshare-wi-fi-2-4ghz-ble-ef6b7981-b6df-4aea-857e-138e2bef4ac1?offerId=18479170722) | 34.90 |
| Charge + DC-DC boost module | 1 module | [Allegro][pl-boost] | 4.90 |
| QRE1113 analog module | 1 module | [Botland][pl-sensor] | 21.90 |
| WeAct 1.54-inch, 200 × 200 e-paper | 1 display | [Elektroweb][pl-screen] / [Allegro alternative](https://allegro.pl/produkt/wyswietlacz-elektroweb-1-54-od-weact-spi-3-3v-5v-91174db3-afa0-4ec1-974b-52413aeab759?offerId=17700495465) | 42.70 |
| Assembly board | 1 board, 60 × 80 mm | Source needed | — |
| Central button, 6 × 6 × 13 mm | Pack of 5; one used | [Botland][pl-central] | 0.99 |
| BeySense blade-present button | Pack of 10; one used | [Allegro][pl-beysense] | 5.37 |
| SS22T35 power slide switch | Pack of 5; one used | [Botland][pl-switch] | 5.90 |
| Female header, 2.54 mm | 1 strip, 40 pins | [Allegro][pl-female] | 3.35 |
| Male header, 2.54 mm | 1 strip, 40 pins | [Allegro][pl-male] | 4.40 |
| LiPo battery, 602020, 3.7 V, 200 mAh | 1 battery | [Allegro][pl-battery] | 22.90 |
| **Subtotal — 10 priced items, assembly board excluded** | | | **147.31 zł ≈ $39.53** |

Prices refer to the first supplier in each row; alternative sellers are not counted again.

### Worldwide / AliExpress

All links below are AliExpress alternatives. Delivery availability, taxes and displayed prices depend on the destination country. USD values are conversions of PLN source prices, not US checkout quotes. Prices marked **~*** are historical price values embedded in the supplied AliExpress URLs, not verified current checkout prices. Their variant, pack size and discount eligibility are unconfirmed. Unmarked prices come from the supplied screenshots. The ESP32 screenshot price is a displayed special offer for one board with unsoldered headers; Polish prices are not substituted.

| Component | Buy on AliExpress / variant | PLN | USD equivalent |
|---|---|---:|---:|
| ESP32-S3-Zero | [1 board, unsoldered headers](https://www.aliexpress.com/item/1005006963045909.html) | 17.92 | $4.81 |
| Charge + DC-DC boost module | [Module](https://www.aliexpress.com/item/1005009566821602.html) | ~4.73* | ~$1.27* |
| QRE1113 analog module | [Sensor](https://www.aliexpress.com/item/1005008965289337.html) | ~4.89* | ~$1.31* |
| WeAct 1.54-inch, 200 × 200 e-paper | [Display](https://www.aliexpress.com/item/1005006291142235.html) | ~32.09* | ~$8.61* |
| Assembly board | [1 board, 60 × 80 mm](https://www.aliexpress.com/item/1005007118438888.html) | 1.99 | $0.53 |
| Central momentary button | [6 × 6 × 12 mm](https://www.aliexpress.com/item/1005007324455095.html) | ~10.20* | ~$2.74* |
| BeySense blade-present button | [Button](https://www.aliexpress.com/item/1005005473521127.html) | ~5.92* | ~$1.59* |
| Power slide switch | [Switch](https://www.aliexpress.com/item/1005008240336192.html) / [MSK-12D19 alternative](https://www.aliexpress.com/item/4001202080623.html) | ~13.01* | ~$3.49* |
| Female pin headers | [Select female variant](https://www.aliexpress.com/item/1005006181782542.html) | ~4.69* | ~$1.26* |
| Male pin headers | [Select male variant](https://www.aliexpress.com/item/1005006181782542.html) | ~4.69* | ~$1.26* |
| LiPo battery | [1 battery, 602020, 3.7 V, 200 mAh](https://www.aliexpress.com/item/32664248150.html) | 21.89 | $5.87 |
| **Indicative total — all 11 listed rows** | | **~122.02 zł** | **~$32.74** |

*Saved URL prices may apply to a different variant or promotional offer. The estimate counts each saved listing price once per row, not a verified pack quantity. Male and female headers share one listing; 4.69 zł is provisionally used for each, but both variant prices need confirmation. The MSK-12D19 alternative has a saved price of **~7.39 zł (~$1.98)** and is not added to the subtotal.*

### Before ordering

- Switch alternatives may differ in footprint and contact arrangement. Confirm fit and identify the common/switched contacts before wiring.
- Check the battery's permitted charge current against the charger configuration; do not assume a module's default setting is suitable for a small 200 mAh cell. Set the boost output to **5.0 V before connecting the ESP32**.
- Also allow for the 1 kΩ sensor resistor, wire, solder, fasteners and printed enclosure. These were not priced in the supplied shopping list.

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

## Component cost summary

Prices recorded on **14 September 2026**. Polish prices come from the linked listings, with the Allegro battery confirmed by a user-provided screenshot. AliExpress ESP32, assembly board and battery prices come from user-provided screenshots; eight other row prices are unverified historical values embedded in the supplied URLs (original capture date unknown). Prices and stock may change, and indexed listings may lag behind checkout.

| Sourcing option | Priced items | Amount PLN | Amount USD | Status / limitations |
|---|---|---:|---:|---|
| **Poland** | 10 of 11 | **147.31 zł** | **$39.53** | Polish assembly board source and price |
| **Worldwide / AliExpress** | 11 of 11; 8 URL estimates | **~122.02 zł** | **~$32.74** | All rows included; estimated variants/pack sizes need confirmation |

**Poland is a subtotal excluding the assembly board; AliExpress is an indicative total for all 11 listed rows, not a verified checkout total.** Missing prices are not zero. The two shopping lists are independent: do not add their amounts together. Full packs are counted where specified, rather than just the individual parts consumed in one build.

Conversion: **1 USD = 3.7267 PLN**, [NBP table 177/A/NBP/2026, 11 September 2026](https://api.nbp.pl/api/exchangerates/rates/a/usd/2026-09-11/?format=json). USD amounts are calculated from PLN and rounded independently to two decimal places; displayed row values may differ from the subtotal by a cent. Card/payment-provider exchange rates may differ.

Shipping (including the AliExpress board's displayed 6.60 zł delivery), additional checkout charges, tools, enclosure printing and unlisted assembly supplies are excluded. AliExpress screenshot prices include displayed VAT for the selected destination; the ESP32's displayed special-offer price is used, but no additional coupons or extra discounts are applied.

[pl-esp]: https://elektroweb.pl/pl/minikomputery/2250-esp32-s3-zero-waveshare-wi-fi-24ghz-ble-spi-i2c-uart-i2s-adc-micropython.html
[pl-boost]: https://allegro.pl/produkt/ladowarka-li-ion-tp4056-z-przetwornica-dc-4-3-28v-97401841-c49c-4884-a7b6-5f43c8eaeeaa?offerId=15794012707
[pl-sensor]: https://botland.com.pl/czujniki-odbiciowe/7684-qre1113-czujnik-odbiciowy-analogowy-sparkfun-rob-09453-5904422304867.html
[pl-screen]: https://elektroweb.pl/pl/e-paper/2072-wyswietlacz-e-paper-200x200-czarno-bialy-154-od-weact-spi-33v-5v.html
[pl-central]: https://botland.com.pl/tact-switch/380-tact-switch-6x6mm-13mm-tht-5szt-5904422307608.html
[pl-beysense]: https://allegro.pl/produkt/tact-switch-smd-6x6mm-h-3-4mm-czerwony-niski-3d75d2bf-592b-4ecc-8c0d-1ee1b312e8ed?offerId=7883778097
[pl-switch]: https://botland.com.pl/przelaczniki-suwakowe-i-przesuwne/8052-przelacznik-suwakowy-ss22t35-2-pozycyjny-5szt-5904422336172.html
[pl-female]: https://allegro.pl/produkt/zlacze-precyzyjne-sip-1x40-2-54-proste-do-druku-1-244d18c7-68eb-4f0a-8e26-c71f5595ef4f?offerId=15862835670
[pl-male]: https://allegro.pl/produkt/zlacze-precyzyjne-goldpin-1x40-2-54-proste-do-dru-e65e7809-90b8-46c4-83e3-86fa24ef9e71?offerId=15862768801
[pl-battery]: https://allegro.pl/oferta/akumulator-pryzmatyczny-3-7v-200mah-lipo-602020-4404-18813108295


