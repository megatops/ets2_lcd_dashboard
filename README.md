# ETS2 LCD Dashboard

ETS2 LCD dashboard is a client of [ETS2 Telemetry Web Server](https://github.com/Funbit/ets2-telemetry-server) made with ESP32-C3. It can connect to the telemetry server via Wi-Fi, work as a wireless digital dashboard for your ETS2/ATS trucks. Theoretically, the code should be able to port to any MCU boards with integrated Wi-Fi support (such as Raspberry Pi Pico W, ESP8266, ESP32 series boards).

![connections](./ets2_lcd_dashboard.png)

The dashboard can display:

- Estimated distance and time,
- Current speed, cruise control speed and speed limit,
- Fuel and estimated fuel distance.

## Hardware

The prices listed below are the retail prices I purchased in China in October 2023, only for reference.

| Part                                     | Price                         |
| ---------------------------------------- | ----------------------------- |
| LuatOS ESP32C3-CORE development board    | CNY 9.9 (with free shipping)  |
| LCD 2004 (20x4) with I2C daughter board  | CNY 12.7 (with free shipping) |
| 40pcs female to female jumper wires      | CNY 2 (with free shipping)    |
| LCD 2004 acrylic display case (optional) | CNY 2.8 + CNY 2 shipping fee  |
|                                          | Total: CNY 29.4 (USD 4.2)     |

Pin connections:

| LCD 2004 I2C | ESP32C3-CORE |
| ------------ | ------------ |
| VCC          | VBUS         |
| GND          | GND          |
| SDA          | GPIO4        |
| SCL          | GPIO5        |

## Configuration

The Wi-Fi information and ETS2 telemetry web server address are hardcoded in `ets2_lcd_dashboard.ino`. You will need to update below lines to match your configuration (make sure to assign a static IPv4 address to your PC):

```c
// Wi-Fi and API server
static const char *SSID = "YOUR WIFI SSID";
static const char *PASSWORD = "YOUR WIFI PASSWORD";
static const char *ETS_API = "http://YOUR_PC_IP:25555/api/ets2/telemetry";
```

In most case, the default I2C address of LCD 2004 should be `0x27`. But if you cannot get the LCD work, try to change this address to `0x3F` (PCF8574AT).

```c
// 2004 LCD PCF8574 I2C address
static const int LCD_ADDR = 0x27;
```

## Build and Upload

The project is an Arduino IDE Sketch. You will need to add ESP32-C3 board support first:

1. Install Arduino IDE: https://www.arduino.cc/en/software/
2. Add board manager URL: https://dl.espressif.com/dl/package_esp32_index.json
3. Install `esp32` by Espressif Systems from board manager.

Then install below libraries:

- `ArduinHttpClient` by Arduino

- `Arduino_JSON` by Arduino

- `LiquidCrystal I2C` by Frank de Brabander

- `BigFont02_I2C` by Harald Coeleveld

  > ⚠ When installing `BigFont02_I2C`, it may occur "Library 'LiquidCrystal_I2C' not found" error. You can download it from: https://www.arduino.cc/reference/en/libraries/bigfont02_i2c/, then refer [this link](https://learn.adafruit.com/adafruit-all-about-arduino-libraries-install-use/how-to-install-a-library) to install it manually.

To build and upload the firmware:

 1. Connect your ESP32C3 board with USB cable.

 2. Choose board: `esp32 / ESP32C3 Dev Module` (or the type specified by your vendor)

    > ⚠ For LuatOS ESP32C3-CORE board, the flash mode must be set to `DIO`.

 3. Open the `ets2_lcd_dashboard` folder, click "Upload" button to build and upload.

It is highly recommended to open the "Serial Monitor" in Arduino IDE for troubleshooting during the first run.

## Big Font Tweak (Optional)

Apply below patch to `BigFont02_I2C` to tweak the font design of `1` and `7`:

```diff
--- a/BigFont02_I2C.cpp
+++ b/BigFont02_I2C.cpp
@@ -116,3 +116,3 @@
        1,      7,      0,      1,      5,      0,              //      0x30    0
-       32,     32,     0,      32,     32,     1,              //      0x31    1
+       32,     1,      32,     32,     1,      32,             //      0x31    1
        4,      2,      0,      1,      5,      5,              //      0x32    2
@@ -122,3 +122,3 @@
        1,      2,      3,      1,      5,      0,              //      0x36    6
-       1,      7,      0,      32,     32,     1,              //      0x37    7
+       1,      7,      0,      32,     32,     0,              //      0x37    7
        1,      2,      0,      1,      5,      0,              //      0x38    8
```

Rebuild and upload.
