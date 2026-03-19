// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <time.h>
#include "config.h"
#include "display.hpp"
#include "utils.hpp"

enum GameState {
  SERVER_DOWN,
  NOT_START,

  READY,
  DRIVING,  // driving the truck
};

enum TimerState {
  INIT,
  IDLE,

  ETS2_ACIVE,
  FORZA_ACTIVE,
  STATE_MAX,
};

#define BLINK_IF(cond, msg1, msg2) ((!(cond) || blinkShow_) ? (msg1) : (msg2))
#define BLINK(msg1, msg2) (blinkShow_ ? (msg1) : (msg2))

class Dashboard {
public:
  Dashboard(Display &display)
    : disp_(display) {}
  virtual ~Dashboard() {}

  virtual GameState getGameData();
  virtual void freshDisplay(time_t time);

protected:
  Display &disp_;
  bool blinkShow_{};    // synchronize the blinks
  int blinkCounter_{};  // count blink duration
  bool force_{};        // force update (bypass cache)
};

#define PRINTF_XY(x, y, ...) \
  do { \
    disp_.setCursor(x, y); \
    disp_.printf(__VA_ARGS__); \
  } while (0)

#define PRINT_XY(x, y, str) \
  do { \
    disp_.setCursor(x, y); \
    disp_.print(str); \
  } while (0)
