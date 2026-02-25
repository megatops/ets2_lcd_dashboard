# ETS2 / Forza LCD Dashboard

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/megatops/ets2_lcd_dashboard) ![GitHub License](https://img.shields.io/github/license/megatops/ets2_lcd_dashboard?logo=license) ![GitHub top language](https://img.shields.io/github/languages/top/megatops/ets2_lcd_dashboard) ![GitHub contributors](https://img.shields.io/github/contributors-anon/megatops/ets2_lcd_dashboard) ![GitHub forks](https://img.shields.io/github/forks/megatops/ets2_lcd_dashboard) ![GitHub Repo stars](https://img.shields.io/github/stars/megatops/ets2_lcd_dashboard)

ETS2 / Forza LCD dashboard can be used with [ETS2 Telemetry Web Server](https://github.com/Funbit/ets2-telemetry-server) and Forza Motorsport / Horizon, made with ESP8266 / ESP32-C3. It can connect to the game via Wi-Fi, work as a wireless digital dashboard for your ETS2 / ATS trucks and Forza racing cars. Theoretically, the code should be able to port to any MCU boards with integrated Wi-Fi support (such as Raspberry Pi Pico W, other ESP32 series boards).

![photo](media/forza.jpg)

## Display Modes

The dashboard has 4 display modes based on the game status:

![modes](media/dashboard.jpg)

- Idle (not driving):
  - Time and date (sync with NTP).
- Euro Truck Simulator 2 / American Truck Simulator:
  - Current time (in the real world),
  - Estimated distance and time,
  - Current speed, cruise control speed and speed limit (kph/mph),
  - Fuel and estimated fuel distance,
  - (Optional) LED indicators: left blinker, air pressure, brake, low beam, high beam, beacon, park brake, right blinker.
- Forza series:
  - RPM bar (linear and converging style),
  - Best lap time, last lap time, current lap time,
  - Current speed (kph/mph), gear, fuel level,
  - Current lap number, race position.
  - Professional dashboard style for S+ class,
  - (Optional) LED shift indicators.

Check the videos to see how it looks in action:

- ETS2 / ATS:

  [![video](https://img.youtube.com/vi/SX45oxnS2IU/0.jpg)](https://www.youtube.com/watch?v=SX45oxnS2IU)

- Forza Horizon:

  [![video](https://img.youtube.com/vi/D59rC5C5DTI/0.jpg)](https://www.youtube.com/watch?v=D59rC5C5DTI)

## Hardware

The prices listed below are the retail prices I purchased in China in October 2023, only for reference.

| Part                                     | Price                         |
| ---------------------------------------- | ----------------------------- |
| LuatOS ESP32C3-CORE development board    | CNY 9.9 (with free shipping)  |
| LCD 2004 (20x4) with I2C daughter board  | CNY 12.7 (with free shipping) |
| 40pcs female to female jumper wires      | CNY 2 (with free shipping)    |
| LCD 2004 acrylic display case (optional) | CNY 2.8 + CNY 2 shipping fee  |
| 8x WS2812 LED stick (optional)           | CNY 2 (with free shipping)    |
|                                          | Total: CNY 31.4 (USD 4.5)     |

![connections](media/ets2_lcd_dashboard.png)

Pin connections:

| ESP8266 / ESP32-C3 | LCD 2004 I2C                                             |
| ------------------ | -------------------------------------------------------- |
| `VBUS` / `5V`      | `VCC`                                                    |
| `GND`              | `GND`                                                    |
| `GPIO2`            | `LED` (optional, refer the "Adaptive Backlight" section) |
| `GPIO4`            | `SDA`                                                    |
| `GPIO5`            | `SCL`                                                    |

(Optional) LED indicators:

| ESP8266 / ESP32-C3 | 8x WS2812 LED Stick |
| ------------------ | ------------------- |
| `VBUS` / `5V`      | `VCC`               |
| `GND`              | `GND`               |
| `GPIO0`            | `IN`                |

## Configuration

The Wi-Fi information and ETS2 telemetry web server address are hardcoded in `config.h`. You will need to update below lines to match your configuration (make sure to assign a static IPv4 address to your PC):

```cpp
// Wi-Fi and API server
constexpr const char *SSID = "YOUR WIFI SSID";
constexpr const char *PASSWORD = "YOUR WIFI PASSWORD";
constexpr const char *ETS_API = "http://YOUR_PC_IP:25555/api/ets2/telemetry";
```

Setup NTP server, your local time zone and daylight saving time if you enabled the clock feature (need Internet access):

```cpp
// Clock settings
constexpr bool CLOCK_ENABLE = true;                 // false to disable the clock feature
constexpr const char *NTP_SERVER = "pool.ntp.org";  // "ntp.ntsc.ac.cn" for mainland China
constexpr int TIME_ZONE = 8 * 60;                   // local time zone in minutes
constexpr bool DST = false;                         // daylight saving time
```

> ℹ The clock will sync with NTP server every hour to keep accurate time. Choose the fastest server to speedup the initial sync:
>
> - Mainland China: `ntp.ntsc.ac.cn`
> - Other regions: `pool.ntp.org`
>
> If you don't want the dashboard access the Internet, set `CLOCK_ENABLE` to `false` to completely disable the clock feature.

In most case, the default I2C address of LCD 2004 should be `0x27`. But if you cannot get the LCD work, try to change the address in `board.h` to `0x3F` (PCF8574AT).

```cpp
// 2004 LCD PCF8574 I2C address
constexpr int LCD_ADDR = 0x27;
```

More dashboard features could be customized with below settings (`config.h`):

```cpp
// Dashboard configurations
constexpr bool SHOW_MILE = false;   // display with mile instead of km
constexpr bool CLOCK_BLINK = true;  // blink the ":" mark in ETS2 dashboard clock
constexpr bool CLOCK_12H = true;    // display ETS2 dashboard clock in 12 hour

// Forza: the car class to use performance dashboard style.
// 0:D 1:C 2:B 3:A 4:S1 5:S2 6:X ...
// Set to 0 to always use performance dashboard,
// Set to 10 to disable performance dashboard.
constexpr int FORZA_PRO_CLASS = 4;

// Forza: shift zone
constexpr float FORZA_SHIFT_ZONE = 85.0;
constexpr float FORZA_RED_ZONE = 90.0;

// Forza: shift indicator engine load map
constexpr float RGB_LOAD_MAP[RGB_LED_NUM]{ ... };

// Forza: shift indicator color map
constexpr RgbColor RGB_COLOR_MAP[RGB_LED_NUM]{ ... };
```

## Build and Upload

The project is an Arduino IDE Sketch. You will need to add ESP32-C3 board support first:

1. Install Arduino IDE: https://www.arduino.cc/en/software/
2. Add board manager URL: https://dl.espressif.com/dl/package_esp32_index.json
3. Install `esp32` by Espressif Systems from board manager.

Then install below libraries:

- `Adafruit_NeoPixel` by Adafruit
- `ArduinHttpClient` by Arduino
- `ArduinoJson` by Benoit Blanchon
- `LiquidCrystal I2C` by Frank de Brabander
- `NTPClient` by Fabrice Weinberg
- `SoftwareTimer` by ILoveMemes
- `Time` by Michael Margolis

To build and upload the firmware:

 1. Connect your ESP32C3 board with USB cable.

 2. Choose board: `esp32 / ESP32C3 Dev Module` (or the type specified by your vendor)

    > ⚠ For LuatOS ESP32C3-CORE board, the flash mode must be set to `DIO`.

 3. Open the `ets2_lcd_dashboard` folder, click "Upload" button to build and upload.

It is highly recommended to open the "Serial Monitor" (`2000000` baud) in Arduino IDE for troubleshooting during the first run.

## Adaptive Backlight

By default, the backlight of 2004 I2C LCD can only be set to on or off. To let the firmware control the backlight brightness, the jumper on the I2C daughter board should be removed, and the top jumper pin (labeled with `LED`) should be connected to `GPIO2`. Then the backlight will be controlled as below:

- Dashboard mode (ETS2/ATS):
  - Completely off when the truck engine is stopped;
  - Dimmer when headlight is on (for night).
- Clock mode:
  - Dimmer as night light (1:00am ~ 5:59am by default).

All the backlight levels could be customized with below settings, values from `0` to `255` (100%):

```cpp
// Backlight levels for dashboard
constexpr int BACKLIGHT_DAY = 200;    // for daytime, brighter
constexpr int BACKLIGHT_NIGHT = 128;  // when headlight on, dimmer

// Backlight levels for clock
constexpr int BACKLIGHT_CLOCK = 128;     // normal time
constexpr int BACKLIGHT_CLOCK_DIM = 24;  // night light

// Hours to dim the clock (1:00am ~ 5:59am by default)
constexpr bool CLOCK_DIM_HOURS[24]{ ... };
```

## Forza Data Out Setup

Refer [Forza Motorsport "Data Out" Documentation](https://support.forzamotorsport.net/hc/en-us/articles/21742934024211-Forza-Motorsport-Data-Out-Documentation) to setup the Data Out feature in Forza. Ensure to assign a static IPv4 address for ESP32-C3 in your router (typically in the DHCP reservation settings) to receive the telemetry data packets. The default port is `8888`.

If you are playing Forza on Windows, the setup will be more complicated than Xbox. The Windows version can only send data to the localhost address `127.0.0.1` (other IPs will not work). So you have to use an UDP forwarder to forward the data packets to the ESP32-C3.

Here are some tested open source UDP forwarders:

- [Sokit](https://github.com/sinpolib/sokit): With an easy to use GUI.
- [udpfwd](https://github.com/D0x45/udpfwd): Very simple command line tool.

## Release History

**2026-2-23**

- Support ETS2/ATS LED indicators.

**2026-2-15**

- Support Forza Motorsport 7 & 2023, Forza Horizon 4 & 5.

**2026-1-17**

- Battery display for DAF XF Electric.
- Set default fuel capacity for Iveco S-Way, etc.

**2024-9-30**

- More accurate speed display in ATS.

**2024-7-6**

- Display "Batt" instead of "Fuel" for Renault E-Tech T and Scania S BEV;
- Fuel warning.

**2024-7-3**

- ESP8266 support, tested with LOLIN D1 Mini (only cost CNY 7.6, about $1);
- Changed LED pin from `GPIO6` to `GPIO2` to compatible with ESP8266;
- 12/24 hour dashboard clock format switch (`CLOCK_12H`).

**2024-2-11**

- Adaptive backlight.

**2024-1-26**

- Full screen clock when not driving;
- Kilometer/mile switch (`SHOW_MILE`);
- Speeding warning.

**2023-11-26**

- Initial release.

## External Links

Forums

- [Discussion on the official SCS forum](https://forum.scssoft.com/viewtopic.php?p=1881413)

Tutorial

- [给美卡模拟/欧卡模拟2手工撸个硬件行车电脑 (Chinese)](https://post.smzdm.com/p/al8ee28e/)
