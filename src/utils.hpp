// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <type_traits>
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

#define REMOVE_CVREF(T) \
  typename std::remove_cv<typename std::remove_reference<T>::type>::type

#define LAZY_EXEC(force, value, cache, code) \
  do { \
    if ((force) || (value) != (cache)) { \
      code; \
      (cache) = (value); \
    } \
  } while (0)

#define PACKED __attribute__((packed))

static inline double KmConv(double km) {
  return SHOW_MILE ? (km / 1.61) : km;
}
