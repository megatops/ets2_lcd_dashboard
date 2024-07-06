// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifndef __ETS_H__
#define __ETS_H__

typedef struct {
  // electric truck
  bool is_ev;

  // for brightness control
  bool on;
  bool headlight;

  // truck status
  bool fuel_warn;
  int fuel_dist;
  int fuel;    // percentage
  int cruise;  // 0 for off
  int speed;

  // navigation
  int eta_dist;
  int eta_time;  // minutes
  int limit;     // 0 for no limit
} EtsState;

typedef enum {
  GAME_SERVER_DOWN,
  GAME_NOT_START,

  GAME_READY,
  GAME_DRIVING,  // driving the truck
} GameState;

typedef enum {
  LCD_UNKNOWN,
  LCD_DASHBOARD,
  LCD_CLOCK,
} LcdMode;

constexpr bool debug = false;  // verbose serial debug info

#define DEBUG(...) \
  do { \
    if (debug) { \
      Serial.printf(__VA_ARGS__); \
    } \
  } while (0)

#endif
