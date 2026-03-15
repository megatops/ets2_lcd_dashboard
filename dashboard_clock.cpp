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
    display.setCursor(2, 3);
    display.printf("%3.3s", dayShortStr(weekday));
  });

  LAZY_UPDATE(force, month, {
    display.setCursor(7, 3);
    display.printf("%3.3s", monthShortStr(month));
  });

  LAZY_UPDATE(force, day, {
    display.setCursor(10, 3);
    display.printf("%s%2d", (day < 10) ? "." : " ", day);
  });

  LAZY_UPDATE(force, year, {
    display.setCursor(15, 3);
    display.printf("%d", year);
  });
}

static void UpdateTime(bool force, int hour, int minute, int second, bool pm) {
  LAZY_UPDATE(force, hour, display.printLarge(2, 0, hour, 2, false));
  LAZY_UPDATE(force, minute, display.printLarge(9, 0, minute, 2, true));

  LAZY_UPDATE(force, second, {
    display.setCursor(16, 1);
    display.printf("%02d", second);
  });

  LAZY_UPDATE(force, pm, {
    display.setCursor(16, 0);
    display.print(pm ? "pm" : "am");
  });
}

static void ClockInit(bool force) {
  if (!force) {
    return;
  }
  display.ledOFF();
  display.setCursor(0, 0);
  display.print("        \xA5           ");
  display.setCursor(0, 1);
  display.print("        \xA5           ");
  display.setCursor(0, 2);
  display.print("  ----------------- ");
  display.setCursor(0, 3);
  display.print("     ,       ,      ");
}

static void ShowNoClock(bool force) {
  if (!force) {
    return;
  }
  display.ledOFF();
  display.setCursor(0, 0);
  display.print("                    ");
  display.setCursor(0, 1);
  display.print("ETS2 Forza Dashboard");
  display.setCursor(0, 2);
  display.print("Waiting for game ...");
  display.setCursor(0, 3);
  display.print("                    ");
}

void ClockUpdate(time_t time) {
  bool force = (dashMode != DASH_CLOCK);  // mode change needs a full update
  dashMode = DASH_CLOCK;

  if (!CLOCK_ENABLE) {
    // time is not available
    ShowNoClock(force);
    display.backlightUpdate(force, BACKLIGHT_CLOCK);
    return;
  }

  // dim the clock backlight as night light
  display.backlightUpdate(force, CLOCK_DIM_HOURS[hour(time)] ? BACKLIGHT_CLOCK_DIM : BACKLIGHT_CLOCK);

  ClockInit(force);
  UpdateTime(force, hourFormat12(time), minute(time), second(time), isPM(time));
  UpdateDate(force, year(time), month(time), day(time), weekday(time));
}
