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

LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);
LargeDigit digit(lcd);
Adafruit_NeoPixel rgbBar(RGB_LED_NUM, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
DashMode dashMode = DASH_UNKNOWN;

void BacklightUpdate(bool force, int level) {
  LAZY_UPDATE(force, level, {
    analogWrite(LCD_LED_PWM, level);
    DEBUG("Update backlight: %d\n", level);
  });
}

// NeoPixel brightness changes is buggy, the caller should redraw all the pixels
// or the colors may be changed or lost after set.
bool RgbLevelUpdate(bool force, uint8_t level) {
  bool changed = false;
  LAZY_UPDATE(force, level, {
    changed = true;
    rgbBar.setBrightness(level);
    DEBUG("Update RGB brightness: %d\n", level);
  });
  return changed;
}

void RgbOFF(void) {
  rgbBar.clear();
  rgbBar.show();
}

void RgbSet(int i, const RgbColor &color) {
  rgbBar.setPixelColor(RGB_LED_NUM - i - 1, rgbBar.Color(color.r, color.g, color.b));
}

void LcdInit(int sda, int scl, uint32_t freq) {
  rgbBar.begin();
  static_cast<void>(RgbLevelUpdate(true, RGB_LEVEL_DAY));
  RgbOFF();

  Wire.begin(sda, scl, freq);
  lcd.init();
  lcd.backlight();
  BacklightUpdate(true, BACKLIGHT_MAX);
  lcd.clear();
  digit.begin();

  lcd.setCursor(0, 1);
  lcd.print("ETS2 Forza Dashboard");
  lcd.setCursor(1, 2);
  lcd.print("Setting up network");
}
