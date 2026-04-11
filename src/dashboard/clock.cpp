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

#include "clock.hpp"
#include <TimeLib.h>

void ClockDashboard::updateDateTime(time_t time) {
  int h = hourFormat12(time), m = minute(time), s = second(time);
  LAZY_UPDATE(h, disp_.printLarge(2, 0, h, 2, false));
  LAZY_UPDATE(m, disp_.printLarge(9, 0, m, 2, true));
  LAZY_UPDATE(s, dispPrintf(16, 1, "%02d", s));

  bool pm = isPM(time);
  LAZY_UPDATE(pm, dispPrint(16, 0, pm ? "pm" : "am"));

  int yy = year(time), mm = month(time), dd = day(time), wd = weekday(time);
  LAZY_UPDATE(wd, dispPrintf(2, 3, "%3.3s", dayShortStr(wd)));
  LAZY_UPDATE(mm, dispPrintf(7, 3, "%3.3s", monthShortStr(mm)));
  LAZY_UPDATE(dd, dispPrintf(10, 3, "%s%2d", (dd < 10) ? "." : " ", dd));
  LAZY_UPDATE(yy, dispPrintf(15, 3, "%d", yy));
}

void ClockDashboard::clockInit() {
  if (!force_) {
    return;
  }
  disp_.ledOFF();
  dispPrint(0, 0, "        \xA5           ");
  dispPrint(0, 1, "        \xA5           ");
  dispPrint(0, 2, "  ----------------- ");
  dispPrint(0, 3, "     ,       ,      ");
}

void ClockDashboard::noClock() {
  if (!force_) {
    return;
  }
  disp_.ledOFF();
  dispPrint(0, 0, "                    ");
  dispPrint(0, 1, "ETS2 Forza Dashboard");
  dispPrint(0, 2, "Waiting for game ...");
  dispPrint(0, 3, "                    ");
}

void ClockDashboard::fresh(void *owner, time_t time) {
  // owner change needs a full update
  force_ = disp_.setOwner(owner);

  if (!CLOCK_ENABLE) {
    // time is not available
    noClock();
    disp_.backlightUpdate(force_, BACKLIGHT_CLOCK);
    return;
  }

  // dim the clock backlight as night light
  disp_.backlightUpdate(force_, CLOCK_DIM_HOURS[hour(time)] ? BACKLIGHT_CLOCK_DIM : BACKLIGHT_CLOCK);

  clockInit();
  updateDateTime(time);
}
