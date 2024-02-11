// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifndef __ETS_H__
#define __ETS_H__

typedef struct {
  // lights
  bool engine;
  bool light_dash;
  bool light_head;

  // navigation
  int eta_dist;
  int eta_time;  // minutes

  // truck status
  int speed;
  int cruise;  // 0 for off
  int limit;   // 0 for no limit
  int fuel;    // percentage
  int fuel_dist;
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

#endif
