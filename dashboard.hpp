// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include "large_digit.hpp"
#include "telemetry.hpp"
#include "config.h"
#include "display.hpp"
#include "utils.hpp"

enum DashMode {
  DASH_UNKNOWN,
  DASH_ETS2,
  DASH_FORZA,
  DASH_CLOCK,
};

extern DashMode dashMode;
extern Display display;

void ClockUpdate(time_t time);
void Ets2DashboardUpdate(const EtsState *st, time_t time);
void ForzaDashboardUpdate(const ForzaState *st);
