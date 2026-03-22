// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include "../config.h"

#define DEBUG(...) \
  do { \
    if (DEBUG_ENABLE) { \
      Serial.printf(__VA_ARGS__); \
    } \
  } while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

static inline double KmConv(double km) {
  return SHOW_MILE ? (km / 1.61) : km;
}
