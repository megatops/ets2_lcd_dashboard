// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2024-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// Fonts are from BigFont02_I2C by Harald Coeleveld.
// ref: https://coeleveld.com/bigfont/

#include "large_digit.hpp"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

LargeDigit::LargeDigit(LiquidCrystal_I2C &lcd)
  : lcd_(lcd) {}

void LargeDigit::begin() {
  static constexpr uint8_t STROKES[][8]{
    [0] = { 0b11100,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11100 },
    [1] = { 0b00111,
            0b01111,
            0b01111,
            0b01111,
            0b01111,
            0b01111,
            0b01111,
            0b00111 },
    [2] = { 0b11111,
            0b11111,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b11111,
            0b11111 },
    [3] = { 0b11110,
            0b11100,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b11000,
            0b11100 },
    [4] = { 0b01111,
            0b00111,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00011,
            0b00111 },
    [5] = { 0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b11111,
            0b11111 },
    [6] = { 0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00111,
            0b01111 },
    [7] = { 0b11111,
            0b11111,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00000,
            0b00000 }
  };

  for (int i = 0; i < (int)ARRAY_SIZE(STROKES); i++) {
    lcd_.createChar(i, (uint8_t *)STROKES[i]);
  }
}

void LargeDigit::writeDigit(int x, int y, int digit) {
  // 0-7: index of Strokes
  static constexpr uint8_t FONT[][CHAR_HEIGHT][CHAR_WIDTH]{
    [0] = { { 1, 7, 0 },
            { 1, 5, 0 } },
    [1] = { { ' ', 1, ' ' },
            { ' ', 1, ' ' } },
    [2] = { { 4, 2, 0 },
            { 1, 5, 5 } },
    [3] = { { 4, 2, 0 },
            { 6, 5, 0 } },
    [4] = { { 1, 5, 0 },
            { ' ', ' ', 0 } },
    [5] = { { 1, 2, 3 },
            { 6, 5, 0 } },
    [6] = { { 1, 2, 3 },
            { 1, 5, 0 } },
    [7] = { { 1, 7, 0 },
            { ' ', ' ', 0 } },
    [8] = { { 1, 2, 0 },
            { 1, 5, 0 } },
    [9] = { { 1, 2, 0 },
            { 6, 5, 0 } }
  };

  for (int row = 0; row < CHAR_HEIGHT; row++) {
    lcd_.setCursor(x, y + row);
    for (int i = 0; i < CHAR_WIDTH; i++) {
      lcd_.write(FONT[digit][row][i]);
    }
  }
}

void LargeDigit::writeSpace(int x, int y) {
  for (int row = 0; row < CHAR_HEIGHT; row++) {
    lcd_.setCursor(x, y + row);
    lcd_.print("   ");
  }
}

void LargeDigit::clear(int x, int y, int count) {
  for (int i = 0; i < count; i++) {
    writeSpace(x + i * CHAR_WIDTH, y);
  }
}

void LargeDigit::print(int x, int y, unsigned int num, int width, bool leadingZero) {
  int numStr[width];
  for (int i = width - 1; i >= 0; i--) {
    numStr[i] = num % 10;
    num /= 10;
  }

  for (int i = 0; i < width; i++) {
    int digit = numStr[i], col = x + i * CHAR_WIDTH;
    if (digit == 0 && !leadingZero && i < width - 1) {
      writeSpace(col, y);
    } else {
      writeDigit(col, y, digit);
      leadingZero = true;
    }
  }
}
