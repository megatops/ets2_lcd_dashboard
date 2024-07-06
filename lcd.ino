// ETS2 LCD Dashboard for ESP8266/ESP32C3
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

#define LAZY_UPDATE(force, value, code) \
  do { \
    static typeof(value) cached_; \
    if ((force) || (value) != cached_) { \
      code; \
      cached_ = (value); \
    } \
  } while (0)

#define BLINK_IF(cond, msg1, msg2) \
  ({ \
    static bool show_ = true; \
    show_ = (cond) ? !show_ : true; \
    const char *msg_ = show_ ? (msg1) : (msg2); \
    msg_; \
  })

static LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);
static LargeDigit digit(&lcd);
static LcdMode lcd_mode = LCD_UNKNOWN;

static void update_backlight(bool force, int level) {
  LAZY_UPDATE(force, level, {
    analogWrite(LCD_LED, level);
    DEBUG("Update backlight: %d\n", level);
  });
}

// +----+----+----+----
// 00:00 >8888 km 00:00
// CruisBigBigBig Limit
// [888]NumNumNum [888]
// Fuel ####...... 8888
// +----+----+----+----

static void update_speed(bool force, int speed) {
  LIMIT(speed, 199);
  LAZY_UPDATE(force, speed, {
    digit.print(5, 1, speed, 3, false);
    DEBUG("Update speed: %d\n", speed);
  });
}

static void update_ets_dist(bool force, int eta_dist) {
  LIMIT(eta_dist, 9999);
  LAZY_UPDATE(force, eta_dist, {
    lcd.setCursor(8, 0);
    if (eta_dist >= 1000) {
      lcd.printf("%4d", eta_dist);  // 8888km
    } else {
      lcd.printf("%3d ", eta_dist);  // 888 km
    }
    DEBUG("Update ETA distance: %d\n", eta_dist);
  });
}

static void update_eta_time(bool force, int eta_time) {
  LIMIT(eta_time, 99 * 60 + 59);
  LAZY_UPDATE(force, eta_time, {
    lcd.setCursor(15, 0);
    lcd.printf("%02d:%02d", eta_time / 60, eta_time % 60);
    DEBUG("Update ETA time: %d\n", eta_time);
  });
}

static void update_cruise(bool force, int cruise) {
  LIMIT(cruise, 999);
  LAZY_UPDATE(force, cruise, {
    lcd.setCursor(1, 2);
    if (cruise > 0) {
      lcd.printf("%3d", cruise);
    } else {
      lcd.print("---");
    }
    DEBUG("Update cruise: %d\n", cruise);
  });
}

static void update_limit(bool force, int limit, bool speeding) {
  LIMIT(limit, 999);
  LAZY_UPDATE(force, limit, {
    lcd.setCursor(16, 2);
    if (limit > 0) {
      lcd.printf("%3d", limit);
    } else {
      lcd.print("---");
    }
    DEBUG("Update speed limit: %d\n", limit);
  });

  // blink the label as speeding warning
  auto label = BLINK_IF(speeding, "Limit", "     ");
  LAZY_UPDATE(force, label, {
    lcd.setCursor(15, 1);
    lcd.printf("%s", label);
  });
}

static void update_fuel(bool force, bool is_ev, int fuel, int fuel_dist, bool warn) {
  LIMIT(fuel, 100);
  LIMIT(fuel_dist, 9999);

  LAZY_UPDATE(force, fuel_dist, {
    if (fuel_dist < 0) {
      if (!force) {
        break;  // no need to update
      }
      fuel_dist = cached_;  // keep the old display
    }
    lcd.setCursor(16, 3);
    lcd.printf("%4d", fuel_dist);
    DEBUG("Update fuel distance: %d\n", fuel_dist);
  });

  int seg = round(fuel / 10.0);
  LAZY_UPDATE(force, seg, {
    char bar[] = "\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    lcd.setCursor(5, 3);
    lcd.print(bar);
    DEBUG("Update fuel: %d%%\n", fuel);
  });

  // blink the label as fuel warning
  auto label = BLINK_IF(warn, is_ev ? "Batt" : "Fuel", "    ");
  LAZY_UPDATE(force, label, {
    lcd.setCursor(0, 3);
    lcd.printf("%s", label);
  });
}

static void update_dash_clock(bool force, int hour, int minute) {
  LAZY_UPDATE(force, hour, {
    lcd.setCursor(0, 0);
    lcd.printf("%02d", hour);
  });

  LAZY_UPDATE(force, minute, {
    lcd.setCursor(3, 0);
    lcd.printf("%02d", minute);
  });

  auto label = BLINK_IF(CLOCK_BLINK, ":", " ");
  LAZY_UPDATE(force, label, {
    lcd.setCursor(2, 0);
    lcd.printf("%s", label);
  });
}

static void dashboard_init(void) {
  lcd.setCursor(0, 0);
  lcd.printf("%s \x7e     %s   :  ", CLOCK_ENABLE ? "     " : "Navi:", SHOW_MILE ? "mi" : "km");
  lcd.setCursor(0, 1);
  lcd.print("Cruis               ");
  lcd.setCursor(0, 2);
  lcd.print("[   ]          [   ]");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
}

void dashboard_update(EtsState *state, time_t time) {
  bool force = (lcd_mode != LCD_DASHBOARD);  // mode change needs a full update
  lcd_mode = LCD_DASHBOARD;

  // no backlight when engine off, dim when headlight on
  update_backlight(force, state->on ? (state->headlight ? BACKLIGHT_NIGHT : BACKLIGHT_DAY) : BACKLIGHT_OFF);

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
  update_fuel(force, state->is_ev, state->fuel, state->fuel_dist, state->fuel_warn);
}

// +----+----+----+----
//   BigBig.BigBig am
//   NumNum.NumNum 00
//   -----------------
//   Sun, Jan 17, 2024
// +----+----+----+----

static void update_date(bool force, int year, int month, int day, int weekday) {
  LAZY_UPDATE(force, weekday, {
    lcd.setCursor(2, 3);
    lcd.printf("%3.3s", dayShortStr(weekday));
  });

  LAZY_UPDATE(force, month, {
    lcd.setCursor(7, 3);
    lcd.printf("%3.3s", monthShortStr(month));
  });

  LAZY_UPDATE(force, day, {
    lcd.setCursor(10, 3);
    lcd.printf("%s%2d", (day < 10) ? "." : " ", day);
  });

  LAZY_UPDATE(force, year, {
    lcd.setCursor(15, 3);
    lcd.printf("%d", year);
  });
}

static void update_time(bool force, int hour, int minute, int second, bool pm) {
  LAZY_UPDATE(force, hour, digit.print(2, 0, hour, 2, false));
  LAZY_UPDATE(force, minute, digit.print(9, 0, minute, 2, true));

  LAZY_UPDATE(force, second, {
    lcd.setCursor(16, 1);
    lcd.printf("%02d", second);
  });

  LAZY_UPDATE(force, pm, {
    lcd.setCursor(16, 0);
    lcd.print(pm ? "pm" : "am");
  });
}

static void clock_init(void) {
  lcd.setCursor(0, 0);
  lcd.print("        \xA5           ");
  lcd.setCursor(0, 1);
  lcd.print("        \xA5           ");
  lcd.setCursor(0, 2);
  lcd.print("  ----------------- ");
  lcd.setCursor(0, 3);
  lcd.print("     ,       ,      ");
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
