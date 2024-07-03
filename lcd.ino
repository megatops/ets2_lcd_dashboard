// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "ets.h"
#include "config.h"

#define LIMIT(var, max) ((var) = ((var) > (max)) ? (max) : (var))

static LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);
static LargeDigit digit(&lcd);
static LcdMode lcd_mode = LCD_UNKNOWN;

static void update_backlight(bool force, int level) {
  static int old_level;
  if (force || level != old_level) {
    analogWrite(LCD_LED, level);
    DEBUG("Update backlight: %d\n", level);
    old_level = level;
  }
}

// +----+----+----+----
// 00:00 >8888 km 00:00
// CruisBigBigBig Limit
// [888]NumNumNum [888]
// Fuel ####...... 8888
// +----+----+----+----

static void update_speed(bool force, int speed) {
  static int old = -1;
  LIMIT(speed, 199);

  if (force || speed != old) {
    digit.print(5, 1, speed, 3, false);
    DEBUG("Update speed: %d\n", speed);
    old = speed;
  }
}

static void update_ets_dist(bool force, int eta_dist) {
  static int old = -1;
  LIMIT(eta_dist, 9999);

  if (force || eta_dist != old) {
    lcd.setCursor(8, 0);
    if (eta_dist >= 1000) {
      lcd.printf("%4d", eta_dist);  // 8888km
    } else {
      lcd.printf("%3d ", eta_dist);  // 888 km
    }
    DEBUG("Update ETA distance: %d\n", eta_dist);
    old = eta_dist;
  }
}

static void update_eta_time(bool force, int eta_time) {
  static int old = -1;
  LIMIT(eta_time, 99 * 60 + 59);

  if (force || eta_time != old) {
    lcd.setCursor(15, 0);
    lcd.printf("%02d:%02d", eta_time / 60, eta_time % 60);
    DEBUG("Update ETA time: %d\n", eta_time);
    old = eta_time;
  }
}

static void update_cruise(bool force, int cruise) {
  static int old = -1;
  LIMIT(cruise, 999);

  if (force || cruise != old) {
    lcd.setCursor(1, 2);
    if (cruise > 0) {
      lcd.printf("%3d", cruise);
    } else {
      lcd.print("---");
    }
    DEBUG("Update cruise: %d\n", cruise);
    old = cruise;
  }
}

static void update_limit(bool force, int limit, bool speeding) {
  static int old_limit = -1;
  LIMIT(limit, 999);

  if (force || limit != old_limit) {
    lcd.setCursor(16, 2);
    if (limit > 0) {
      lcd.printf("%3d", limit);
    } else {
      lcd.print("---");
    }
    DEBUG("Update speed limit: %d\n", limit);
    old_limit = limit;
  }

  if (WARN_SPEEDING) {
    static bool old_speeding;
    if (force || speeding || speeding != old_speeding) {
      static bool show = true;
      show = speeding ? !show : true;
      lcd.setCursor(15, 1);
      lcd.print(show ? "Limit" : "     ");
      old_speeding = speeding;
    }
  }
}

static void update_fuel(bool force, int fuel, int fuel_dist) {
  LIMIT(fuel, 100);
  LIMIT(fuel_dist, 9999);

  static int old_dist = -1;
  if (fuel_dist < 0) {
    // keep the old display, if old < 0, set to 0
    fuel_dist = old_dist < 0 ? 0 : old_dist;
  }
  if (force || fuel_dist != old_dist) {
    lcd.setCursor(16, 3);
    lcd.printf("%4d", fuel_dist);
    DEBUG("Update fuel distance: %d\n", fuel_dist);
    old_dist = fuel_dist;
  }

  static int old_seg = -1;
  int seg = round(fuel / 10.0);
  if (force || seg != old_seg) {
    char bar[] = "\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    lcd.setCursor(5, 3);
    lcd.print(bar);
    DEBUG("Update fuel: %d%%\n", fuel);
    old_seg = seg;
  }
}

static void update_dash_clock(bool force, int hour, int minute) {
  static int old_hour = -1;
  if (force || hour != old_hour) {
    lcd.setCursor(0, 0);
    lcd.printf("%02d", hour);
    old_hour = hour;
  }

  static int old_minute = -1;
  if (force || minute != old_minute) {
    lcd.setCursor(3, 0);
    lcd.printf("%02d", minute);
    old_minute = minute;
  }

  if (CLOCK_BLINK) {
    static bool show;
    lcd.setCursor(2, 0);
    lcd.print(show ? ":" : " ");
    show = !show;
  }
}

static void dashboard_init(void) {
  lcd.setCursor(0, 0);
  lcd.printf("%s \x7e     %s   :  ", CLOCK_ENABLE ? "  :  " : "Navi:", SHOW_MILE ? "mi" : "km");
  lcd.setCursor(0, 1);
  lcd.print("Cruis          Limit");
  lcd.setCursor(0, 2);
  lcd.print("[   ]          [   ]");
  lcd.setCursor(0, 3);
  lcd.print("Fuel                ");
}

void dashboard_update(EtsState *state, time_t time) {
  bool force = (lcd_mode != LCD_DASHBOARD);  // mode change needs a full update
  lcd_mode = LCD_DASHBOARD;

  // no backlight when engine off, dim when headlight on
  update_backlight(force, state->started ? (state->headlight ? BACKLIGHT_NIGHT : BACKLIGHT_DAY) : BACKLIGHT_OFF);

  if (force) {
    dashboard_init();
  }

  if (CLOCK_ENABLE) {
    update_dash_clock(force, CLOCK_12H ? hourFormat12(time) : hour(time), minute(time));
  }
  update_limit(force, state->limit, (state->limit > 0) && (state->speed > state->limit));
  update_speed(force, state->speed);
  update_cruise(force, state->cruise);
  update_ets_dist(force, state->eta_dist);
  update_eta_time(force, state->eta_time);
  update_fuel(force, state->fuel, state->fuel_dist);
}

// +----+----+----+----
//   BigBig.BigBig am
//   NumNum.NumNum 00
//   -----------------
//   Sun, Jan 07, 2024
// +----+----+----+----

static void update_date(bool force, int year, int month, int day, int weekday) {
  static int old_year = -1, old_month = -1, old_day = -1;
  if (force || year != old_year || month != old_month || day != old_day) {
    lcd.setCursor(2, 3);

    // the dayShortStr() and monthShortStr() share the same string buffer,
    // so we have to make a copy before calling another one.
    char day_str[4];
    snprintf(day_str, sizeof(day_str), "%s", dayShortStr(weekday));
    lcd.printf("%s, %s%s%2d, %d", day_str, monthShortStr(month), (day < 10) ? "." : " ", day, year);
    old_year = year;
    old_month = month;
    old_day = day;
  }
}

static void update_time(bool force, int hour, int minute, int second, bool pm) {
  static int old_hour = -1;
  if (force || hour != old_hour) {
    digit.print(2, 0, hour, 2, false);
    old_hour = hour;
  }

  static int old_minute = -1;
  if (force || minute != old_minute) {
    digit.print(9, 0, minute, 2, true);
    old_minute = minute;
  }

  static int old_second = -1;
  if (force || second != old_second) {
    lcd.setCursor(16, 1);
    lcd.printf("%02d", second);
    old_second = second;
  }

  static int old_pm;
  if (force || pm != old_pm) {
    lcd.setCursor(16, 0);
    lcd.print(pm ? "pm" : "am");
    old_pm = pm;
  }
}

static void clock_init(void) {
  lcd.setCursor(0, 0);
  lcd.print("        \xA5           ");
  lcd.setCursor(0, 1);
  lcd.print("        \xA5           ");
  lcd.setCursor(0, 2);
  lcd.print("  ----------------- ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
}

void clock_update(time_t time) {
  bool force = (lcd_mode != LCD_CLOCK);  // mode change needs a full update
  lcd_mode = LCD_CLOCK;

  // dim the clock backlight as night light
  update_backlight(force, CLOCK_DIM_HOURS[hour(time)] ? BACKLIGHT_CLOCK_DIM : BACKLIGHT_CLOCK);

  if (force) {
    clock_init();
  }
  update_time(force, hourFormat12(time), minute(time), second(time), isPM(time));
  update_date(force, year(time), month(time), day(time), weekday(time));
}

LcdMode lcd_get_mode(void) {
  return lcd_mode;
}

void lcd_init(int sda, int scl, uint32_t freq) {
  Wire.begin(sda, scl, freq);
  lcd.init();
  lcd.backlight();
  update_backlight(true, BACKLIGHT_MAX);
  lcd.clear();
  digit.begin();

  lcd.setCursor(1, 1);
  lcd.print("ETS2 LCD Dashboard");
  lcd.setCursor(1, 2);
  lcd.print("Setting up network");
}
