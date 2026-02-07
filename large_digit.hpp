// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2024-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// Fonts are from BigFont02_I2C by Harald Coeleveld.
// ref: https://coeleveld.com/bigfont/

#include <LiquidCrystal_I2C.h>

class LargeDigit {
public:
  LargeDigit(LiquidCrystal_I2C &lcd);
  void begin();
  void clear(int x, int y, int count);
  void print(int x, int y, unsigned int num, int width, bool leadingZero);

private:
  LiquidCrystal_I2C &lcd_;
  void writeDigit(int x, int y, int digit);
  void writeSpace(int x, int y);

  // each digit takes up 3 cols x 2 rows
  static constexpr int CHAR_WIDTH = 3;
  static constexpr int CHAR_HEIGHT = 2;
};
