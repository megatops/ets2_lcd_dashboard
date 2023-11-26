// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BigFont02_I2C.h>
#include "ets.h"

#define LIMIT(var, max) ((var) = ((var) > (max)) ? (max) : (var))

static LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);
static BigFont02_I2C big(&lcd);

// +----+----+----+----
// ETA: 8888 km | 00:00
// CruisBigBigBig Limit
// [888]NumNumNum [888]
// Fuel ####...... 8888
// +----+----+----+----

static void update_speed(int speed) {
  static int old = -1;
  LIMIT(speed, 199);

  if (speed != old) {
    big.writeint(1, 5, speed, 3, false);

    // clear the remaining numbers
    if (speed < 100 && old >= 100) {
      // BigFont will ignore spaces, not sure why. So write '-' instead.
      big.writechar(1, 5, '-');
    }
    if (speed < 10 && old >= 10) {
      big.writechar(1, 8, '-');
    }
    Serial.printf("Update speed: %d\n", speed);
    old = speed;
  }
}

static void update_ets_dist(int eta_dist) {
  static int old = -1;
  LIMIT(eta_dist, 9999);

  if (eta_dist != old) {
    lcd.setCursor(5, 0);
    lcd.printf("%4d", eta_dist);
    Serial.printf("Update ETA distance: %d\n", eta_dist);
    old = eta_dist;
  }
}

static void update_eta_time(int eta_time) {
  static int old = -1;
  LIMIT(eta_time, 99 * 60 + 59);

  if (eta_time != old) {
    lcd.setCursor(15, 0);
    lcd.printf("%02d:%02d", eta_time / 60, eta_time % 60);
    Serial.printf("Update ETA time: %d\n", eta_time);
    old = eta_time;
  }
}

static void update_cruise(int cruise) {
  static int old = -1;
  LIMIT(cruise, 999);

  if (cruise != old) {
    lcd.setCursor(1, 2);
    if (cruise > 0) {
      lcd.printf("%3d", cruise);
    } else {
      lcd.print("---");
    }
    Serial.printf("Update cruise: %d\n", cruise);
    old = cruise;
  }
}

static void update_limit(int limit) {
  static int old = -1;
  LIMIT(limit, 999);

  if (limit != old) {
    lcd.setCursor(16, 2);
    if (limit > 0) {
      lcd.printf("%3d", limit);
    } else {
      lcd.print("---");
    }
    Serial.printf("Update speed limit: %d\n", limit);
    old = limit;
  }
}

static void update_fuel(int fuel, int fuel_dist) {
  LIMIT(fuel, 100);
  LIMIT(fuel_dist, 9999);

  static int old_dist = -1;
  if (fuel_dist != old_dist && fuel_dist != -1) {
    lcd.setCursor(16, 3);
    lcd.printf("%4d", fuel_dist);
    Serial.printf("Update fuel distance: %d\n", fuel_dist);
    old_dist = fuel_dist;
  }

  static int old_seg = -1;
  int seg = round(fuel / 10.0);
  if (seg != old_seg) {
    char bar[] = "\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    lcd.setCursor(5, 3);
    lcd.print(bar);
    Serial.printf("Update fuel: %d%%\n", fuel);
    old_seg = seg;
  }
}

void dashboard_update(EtsState *stat) {
  update_ets_dist(stat->eta_dist);
  update_eta_time(stat->eta_time);
  update_cruise(stat->cruise);
  update_limit(stat->limit);
  update_fuel(stat->fuel, stat->fuel_dist);
  update_speed(stat->speed);
}

void dashboard_reset(void) {
  EtsState stat = {};
  dashboard_update(&stat);
}

static void dashboard_init(void) {
  lcd.setCursor(0, 0);
  lcd.print("ETA:      km |      ");
  lcd.setCursor(0, 1);
  lcd.print("Cruis          Limit");
  lcd.setCursor(0, 2);
  lcd.print("[   ]          [   ]");
  lcd.setCursor(0, 3);
  lcd.print("Fuel                ");

  dashboard_reset();
}

void lcd_init(int sda, int scl) {
  Wire.begin(sda, scl);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  big.begin();
  dashboard_init();
}
