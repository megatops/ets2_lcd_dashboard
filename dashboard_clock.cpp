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

#include "dashboard_clock.hpp"
#include <TimeLib.h>

ClockDashboard::ClockDashboard(Display &display)
  : Dashboard(display) {}

void ClockDashboard::updateDate(time_t time) {
  int y = year(time), m = month(time), d = day(time), wd = weekday(time);

  LAZY_UPDATE(wd, {
    display_.setCursor(2, 3);
    display_.printf("%3.3s", dayShortStr(wd));
  });

  LAZY_UPDATE(m, {
    display_.setCursor(7, 3);
    display_.printf("%3.3s", monthShortStr(m));
  });

  LAZY_UPDATE(d, {
    display_.setCursor(10, 3);
    display_.printf("%s%2d", (d < 10) ? "." : " ", d);
  });

  LAZY_UPDATE(y, {
    display_.setCursor(15, 3);
    display_.printf("%d", y);
  });
}

void ClockDashboard::updateTime(time_t time) {
  int h = hourFormat12(time), m = minute(time), s = second(time);
  LAZY_UPDATE(h, display_.printLarge(2, 0, h, 2, false));
  LAZY_UPDATE(m, display_.printLarge(9, 0, m, 2, true));
  LAZY_UPDATE(s, {
    display_.setCursor(16, 1);
    display_.printf("%02d", s);
  });

  bool pm = isPM(time);
  LAZY_UPDATE(pm, {
    display_.setCursor(16, 0);
    display_.print(pm ? "pm" : "am");
  });
}

void ClockDashboard::clockInit() {
  if (!force_) {
    return;
  }
  display_.ledOFF();
  display_.setCursor(0, 0);
  display_.print("        \xA5           ");
  display_.setCursor(0, 1);
  display_.print("        \xA5           ");
  display_.setCursor(0, 2);
  display_.print("  ----------------- ");
  display_.setCursor(0, 3);
  display_.print("     ,       ,      ");
}

void ClockDashboard::noClock() {
  if (!force_) {
    return;
  }
  display_.ledOFF();
  display_.setCursor(0, 0);
  display_.print("                    ");
  display_.setCursor(0, 1);
  display_.print("ETS2 Forza Dashboard");
  display_.setCursor(0, 2);
  display_.print("Waiting for game ...");
  display_.setCursor(0, 3);
  display_.print("                    ");
}

GameState ClockDashboard::getGameData() {
  return GameState::READY;
}

void ClockDashboard::freshDisplay(time_t time) {
  // mode change needs a full update
  force_ = (display_.mode != DisplayMode::CLOCK);
  display_.mode = DisplayMode::CLOCK;

  if (!CLOCK_ENABLE) {
    // time is not available
    noClock();
    display_.backlightUpdate(force_, BACKLIGHT_CLOCK);
    return;
  }

  // dim the clock backlight as night light
  display_.backlightUpdate(force_, CLOCK_DIM_HOURS[hour(time)] ? BACKLIGHT_CLOCK_DIM : BACKLIGHT_CLOCK);

  clockInit();
  updateTime(time);
  updateDate(time);
}
