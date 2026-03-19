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

#include "ntp_clock.hpp"
#include "config.h"
#include <TimeLib.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

NtpClock::NtpClock(Display& display, const char* server, long timeOffset, unsigned long updateInterval)
  : disp_(display), ntp_(NTPClient(udp_, server, timeOffset, updateInterval)) {}

void NtpClock::initialSync() {
  Serial.printf("NTP syncing with %s .", server_.c_str());
  while (!ntp_.forceUpdate()) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" Local Time: %s\n", ntp_.getFormattedTime().c_str());
}

void NtpClock::tick() {
  if (!CLOCK_ENABLE || disp_.mode != DisplayMode::CLOCK) {
    return;
  }

  if (ntp_.getEpochTime() == lastUpdate_) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED && ntp_.update()) {
    Serial.printf("NTP sync success: %s.\n", ntp_.getFormattedTime().c_str());
  }

  freshDisplay();
}

void NtpClock::updateDate(time_t time) {
  int y = year(time), m = month(time), d = day(time), wd = weekday(time);

  LAZY_UPDATE(wd, {
    PRINTF_XY(2, 3, "%3.3s", dayShortStr(wd));
  });

  LAZY_UPDATE(m, {
    PRINTF_XY(7, 3, "%3.3s", monthShortStr(m));
  });

  LAZY_UPDATE(d, {
    PRINTF_XY(10, 3, "%s%2d", (d < 10) ? "." : " ", d);
  });

  LAZY_UPDATE(y, {
    PRINTF_XY(15, 3, "%d", y);
  });
}

void NtpClock::updateTime(time_t time) {
  int h = hourFormat12(time), m = minute(time), s = second(time);
  LAZY_UPDATE(h, disp_.printLarge(2, 0, h, 2, false));
  LAZY_UPDATE(m, disp_.printLarge(9, 0, m, 2, true));
  LAZY_UPDATE(s, {
    PRINTF_XY(16, 1, "%02d", s);
  });

  bool pm = isPM(time);
  LAZY_UPDATE(pm, {
    PRINT_XY(16, 0, pm ? "pm" : "am");
  });
}

void NtpClock::clockInit() {
  if (!force_) {
    return;
  }
  disp_.ledOFF();
  PRINT_XY(0, 0, "        \xA5           ");
  PRINT_XY(0, 1, "        \xA5           ");
  PRINT_XY(0, 2, "  ----------------- ");
  PRINT_XY(0, 3, "     ,       ,      ");
}

void NtpClock::noClock() {
  if (!force_) {
    return;
  }
  disp_.ledOFF();
  PRINT_XY(0, 0, "                    ");
  PRINT_XY(0, 1, "ETS2 Forza Dashboard");
  PRINT_XY(0, 2, "Waiting for game ...");
  PRINT_XY(0, 3, "                    ");
}

void NtpClock::freshDisplay() {
  // mode change needs a full update
  force_ = (disp_.mode != DisplayMode::CLOCK);
  disp_.mode = DisplayMode::CLOCK;

  if (!CLOCK_ENABLE) {
    // time is not available
    noClock();
    disp_.backlightUpdate(force_, BACKLIGHT_CLOCK);
    return;
  }

  auto time = ntp_.getEpochTime();

  // dim the clock backlight as night light
  disp_.backlightUpdate(force_, CLOCK_DIM_HOURS[hour(time)] ? BACKLIGHT_CLOCK_DIM : BACKLIGHT_CLOCK);

  clockInit();
  updateTime(time);
  updateDate(time);
}
