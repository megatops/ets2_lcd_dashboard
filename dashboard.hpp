// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifndef __DASHBOARD_H__
#define __DASHBOARD_H__

#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include "large_digit.hpp"
#include "telemetry.hpp"
#include "config.h"

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

enum DashMode {
  DASH_UNKNOWN,
  DASH_ETS2,
  DASH_FORZA,
  DASH_CLOCK,
};

extern DashMode dashMode;
extern LiquidCrystal_I2C lcd;
extern LargeDigit digit;
extern Adafruit_NeoPixel rgbBar;

void LcdInit(int sda, int scl, uint32_t freq);
void BacklightUpdate(bool force, int level);
void RgbOFF(void);
void RgbSet(int i, const RgbColor &color);
bool RgbLevelUpdate(bool force, uint8_t level);

void ClockUpdate(time_t time);
void Ets2DashboardUpdate(const EtsState *st, time_t time);
void ForzaDashboardUpdate(const ForzaState *st);

#endif
