// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2024-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <cstdint>
#include "board.h"

// Wi-Fi and API server
constexpr const char *SSID = "YOUR WIFI SSID";
constexpr const char *PASSWORD = "YOUR WIFI PASSWORD";
constexpr const char *ETS_API = "http://YOUR_PC_IP:25555/api/ets2/telemetry";

// Clock settings
constexpr bool CLOCK_ENABLE = true;                   // false to disable the clock feature
constexpr const char *NTP_SERVER = "pool.ntp.org";    // "ntp.ntsc.ac.cn" for mainland China
constexpr int TIME_ZONE = 8 * 60;                     // local time zone in minutes
constexpr bool DST = false;                           // daylight saving time

// Dashboard configurations
constexpr bool CLOCK_BLINK = true;  // blink the ":" mark in dashboard clock
constexpr bool CLOCK_12H = true;    // display dashboard clock in 12 hour
constexpr bool SHOW_MILE = false;   // display with mile instead of km

// Backlight levels
constexpr int BACKLIGHT_MAX = 255;
constexpr int BACKLIGHT_OFF = 0;

// Backlight levels for dashboard
constexpr int BACKLIGHT_DAY = 200;    // for daytime, brighter
constexpr int BACKLIGHT_NIGHT = 128;  // when headlight on, dimmer

// RGB bar brightness level
constexpr uint8_t RGB_LEVEL_DAY = 2;
constexpr uint8_t RGB_LEVEL_NIGHT = 1;
constexpr uint8_t RGB_LEVEL_OFF = 0;

// Backlight levels for clock
constexpr int BACKLIGHT_CLOCK = 128;     // normal time
constexpr int BACKLIGHT_CLOCK_DIM = 24;  // night light

// Hours to dim the clock (1:00am ~ 5:59am by default)
constexpr bool CLOCK_DIM_HOURS[24]{
  [0] = false,
  [1] = true,
  [2] = true,
  [3] = true,
  [4] = true,
  [5] = true,
  [6] = false,
  [7] = false,
  [8] = false,
  [9] = false,
  [10] = false,
  [11] = false,
  [12] = false,
  [13] = false,
  [14] = false,
  [15] = false,
  [16] = false,
  [17] = false,
  [18] = false,
  [19] = false,
  [20] = false,
  [21] = false,
  [22] = false,
  [23] = false,
};

// ETS2: electric truck models
constexpr const char *EV_TRUCKS[]{
  "E-Tech T",
  "S BEV",
  "XF Electric",
  nullptr,
};

// ETS2: fallback fuel capacity on data error (Iveco S-Way, etc.)
constexpr double DEFAULT_TANK_SIZE = 1200;

// Engine load to RGB LED map
constexpr float RGB_LOAD_MAP[RGB_LED_NUM] = {
  40.0, 50.0, 60.0,  // green
  70.0, 75.0, 80.0,  // yellow
  85.0, 85.0,        // red
};

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

constexpr RgbColor RGB_COLOR_MAP[RGB_LED_NUM] = {
  { 0, 255, 0 },  // green
  { 0, 255, 0 },
  { 0, 255, 0 },
  { 255, 255, 0 },  // yellow
  { 255, 255, 0 },
  { 255, 255, 0 },
  { 255, 0, 0 },  // red
  { 255, 0, 0 },
};

// Forza: shift zone
constexpr float FORZA_SHIFT_ZONE = 85.0;
constexpr float FORZA_RED_ZONE = 90.0;

#endif
