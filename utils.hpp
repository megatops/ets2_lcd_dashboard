// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include "config.h"

constexpr bool DEBUG_ENABLE = false;  // verbose serial debug info

#define DEBUG(...) \
  do { \
    if (DEBUG_ENABLE) { \
      Serial.printf(__VA_ARGS__); \
    } \
  } while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#define LIMIT(var, max) ((var) = ((var) > (max)) ? (max) : (var))

#define LAZY_UPDATE(value, code) \
  do { \
    static typeof(value) cached_; \
    if (force_ || (value) != cached_) { \
      code; \
      cached_ = (value); \
    } \
  } while (0)

#define dispPrintf(x, y, fmt, ...) \
  do { \
    disp_.setCursor(x, y); \
    disp_.printf(fmt, ##__VA_ARGS__); \
  } while (0)

#define dispPrint(x, y, str) \
  do { \
    disp_.setCursor(x, y); \
    disp_.print(str); \
  } while (0)

static inline double KmConv(double km) {
  return SHOW_MILE ? (km / 1.61) : km;
}
