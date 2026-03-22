// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <Print.h>
#include "../../board.h"
#include "../display/large_digit.hpp"

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// Facade for the entire display complex
class Display : public Print {
public:
  Display(int lcdSDA, int lcdSCL, int lcdFreq, int lcdPWM, int rgbLedPin, int lcdAddr);

  void start();

  // LCD 2004
  virtual inline size_t write(uint8_t val) {
    return lcd_.write(val);
  }

  inline void printLarge(int x, int y, unsigned int num, int width, bool leadingZero) {
    digit_.print(x, y, num, width, leadingZero);
  }

  inline void setCursor(uint8_t col, uint8_t row) {
    lcd_.setCursor(col, row);
  }

  void backlightUpdate(bool force, int level);

  // RGB LEDs
  bool ledBrightnesslUpdate(bool force, uint8_t level);

  inline void ledSet(int i, const RgbColor &color) {
    if (i < 0 || i >= RGB_LED_NUM) {
      return;
    }
    rgb_.setPixelColor(RGB_LED_NUM - i - 1, rgb_.Color(color.r, color.g, color.b));
  }

  inline void ledFill(const RgbColor &color) {
    rgb_.fill(rgb_.Color(color.r, color.g, color.b));
  }

  inline void ledClear() {
    rgb_.clear();
  }

  inline void ledShow() {
    rgb_.show();
  }

  inline void ledOFF() {
    ledClear();
    ledShow();
  }

  // return true on owner change
  inline bool setOwner(void *owner) {
    if (owner_ != owner) {
      owner_ = owner;
      return true;
    }
    return false;
  }

  inline bool isOwnedBy(void *owner) const {
    return owner_ == owner;
  }

private:
  int lcdSda_{};
  int lcdScl_{};
  int lcdFreq_{};
  int lcdPwm_{};
  int rgbLedPin_{};
  int lcdAddr_{};

  LiquidCrystal_I2C lcd_;
  LargeDigit digit_;
  Adafruit_NeoPixel rgb_;

  void *owner_{};
};

class Dashboard {
public:
  virtual ~Dashboard() {}

protected:
  Dashboard(Display &display)
    : disp_(display) {}

  void dispPrint(int x, int y, const char *str) {
    disp_.setCursor(x, y);
    disp_.print(str);
  }

protected:
  Display &disp_;
  bool force_{};  // force update (bypass cache)
};

#define dispPrintf(x, y, fmt, ...) \
  do { \
    disp_.setCursor(x, y); \
    disp_.printf(fmt, ##__VA_ARGS__); \
  } while (0)

#define LAZY_UPDATE(value, code) \
  do { \
    static typeof(value) cached_; \
    if (force_ || (value) != cached_) { \
      code; \
      cached_ = (value); \
    } \
  } while (0)
