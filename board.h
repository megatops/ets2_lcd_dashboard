// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifndef __BOARD_H__
#define __BOARD_H__

// 2004 LCD PCF8574 I2C address
constexpr int LCD_ADDR = 0x27;

// I2C IO pins
constexpr int I2C_SDA = 4;
constexpr int I2C_SCL = 5;

// LCD backlight control pin (PWM)
constexpr int LCD_LED = 2;

// Bus frequencies
constexpr int SERIAL_BAUDRATE = 921600;
constexpr uint32_t I2C_FREQ = 400000;  // 400kHz fast mode

#endif
