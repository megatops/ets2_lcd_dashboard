// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include "../display/display.hpp"
#include "../utils.hpp"

class Dashboard {
public:
  virtual ~Dashboard() {}

  inline const Display &getDisplay() const {
    return disp_;
  }

protected:
  Dashboard(Display &display)
    : disp_(display) {}

  void dispPrint(int x, int y, const char *str) {
    disp_.setCursor(x, y);
    disp_.print(str);
  }

protected:
  Display &disp_;
  bool force_{};      // force update (bypass cache)
  bool blinkShow_{};  // synchronize the blinks
};

#define dispPrintf(x, y, fmt, ...) \
  do { \
    disp_.setCursor(x, y); \
    disp_.printf(fmt, ##__VA_ARGS__); \
  } while (0)

#define LAZY_UPDATE(value, code) \
  do { \
    static REMOVE_CVREF(decltype(value)) _cached_; \
    LAZY_EXEC(force_, value, _cached_, code); \
  } while (0)

#define BLINK_IF(cond, msg1, msg2) ((!(cond) || blinkShow_) ? (msg1) : (msg2))
#define BLINK(msg1, msg2) (blinkShow_ ? (msg1) : (msg2))
