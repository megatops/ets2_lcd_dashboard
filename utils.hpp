// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#define LIMIT(var, max) ((var) = ((var) > (max)) ? (max) : (var))

#define LAZY_UPDATE(force, value, code) \
  do { \
    static typeof(value) cached_; \
    if ((force) || (value) != cached_) { \
      code; \
      cached_ = (value); \
    } \
  } while (0)

#define BLINK_IF(cond, msg1, msg2) ((!(cond) || blinkShow) ? (msg1) : (msg2))
#define BLINK(msg1, msg2) (blinkShow ? (msg1) : (msg2))

constexpr bool DEBUG_ENABLE = false;  // verbose serial debug info

#define DEBUG(...) \
  do { \
    if (DEBUG_ENABLE) { \
      Serial.printf(__VA_ARGS__); \
    } \
  } while (0)
