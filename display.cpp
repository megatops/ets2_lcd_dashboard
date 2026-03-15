// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include "display.hpp"
#include "utils.hpp"
#include "board.h"

Display::Display(int lcdSDA, int lcdSCL, int lcdFreq, int lcdPWM, int rgbLedPin, int lcdAddr)
  : lcdSda_(lcdSDA),
    lcdScl_(lcdSCL),
    lcdFreq_(lcdFreq),
    lcdPwm_(lcdPWM),
    rgbLedPin_(rgbLedPin),
    lcdAddr_(lcdAddr),
    lcd_(LiquidCrystal_I2C(lcdAddr_, 20, 4)),
    digit_(LargeDigit(lcd_)),
    rgb_(Adafruit_NeoPixel(RGB_LED_NUM, rgbLedPin_, NEO_GRB + NEO_KHZ800)) {}

void Display::start() {
  pinMode(lcdPwm_, OUTPUT);
  pinMode(rgbLedPin_, OUTPUT);

  // RGB LED bar
  rgb_.begin();
  ledBrightnesslUpdate(true, RGB_LEVEL_DAY);
  ledOFF();

  // I2C LC2004
  Wire.begin(lcdSda_, lcdScl_, lcdFreq_);
  lcd_.init();
  lcd_.backlight();
  backlightUpdate(true, BACKLIGHT_MAX);
  lcd_.clear();
  digit_.begin();

  // display initial info
  lcd_.setCursor(0, 1);
  lcd_.print("ETS2 Forza Dashboard");
  lcd_.setCursor(1, 2);
  lcd_.print("Setting up network");
}

void Display::backlightUpdate(bool force, int level) {
  LAZY_UPDATE(force, level, {
    analogWrite(lcdPwm_, level);
    DEBUG("Update backlight: %d\n", level);
  });
}

// NeoPixel brightness changes is buggy, the caller should redraw all the pixels
// or the colors may be changed or lost after set.
bool Display::ledBrightnesslUpdate(bool force, uint8_t level) {
  bool changed = false;
  LAZY_UPDATE(force, level, {
    changed = true;
    rgb_.setBrightness(level);
    DEBUG("Update LED brightness: %d\n", level);
  });
  return changed;
}
