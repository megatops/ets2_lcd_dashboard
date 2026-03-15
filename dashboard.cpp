// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <Wire.h>
#include "board.h"
#include "telemetry.hpp"
#include "dashboard.hpp"
#include "display.hpp"

DashMode dashMode = DASH_UNKNOWN;
Display display(I2C_SDA, I2C_SCL, I2C_FREQ, LCD_LED_PWM, RGB_LED_PIN, LCD_ADDR);
