// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// +----+----+----+----
//   BigBig.BigBig am
//   NumNum.NumNum 00
//   -----------------
//   Sun, Jan 17, 2024
// +----+----+----+----

#include <TimeLib.h>
#include "dashboard.hpp"

static void UpdateDate(bool force, int year, int month, int day, int weekday) {
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

static void UpdateTime(bool force, int hour, int minute, int second, bool pm) {
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

static void ClockInit(void) {
  RgbOFF();
  lcd.setCursor(0, 0);
  lcd.print("        \xA5           ");
  lcd.setCursor(0, 1);
  lcd.print("        \xA5           ");
  lcd.setCursor(0, 2);
  lcd.print("  ----------------- ");
  lcd.setCursor(0, 3);
  lcd.print("     ,       ,      ");
}

void ClockUpdate(time_t time) {
  bool force = (dashMode != DASH_CLOCK);  // mode change needs a full update
  dashMode = DASH_CLOCK;

  // dim the clock backlight as night light
  BacklightUpdate(force, CLOCK_DIM_HOURS[hour(time)] ? BACKLIGHT_CLOCK_DIM : BACKLIGHT_CLOCK);

  if (force) {
    ClockInit();
  }
  UpdateTime(force, hourFormat12(time), minute(time), second(time), isPM(time));
  UpdateDate(force, year(time), month(time), day(time), weekday(time));
}
