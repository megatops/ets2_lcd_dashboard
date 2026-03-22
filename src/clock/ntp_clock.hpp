// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "../display/display.hpp"

class NtpClock : public Dashboard {
public:
  NtpClock(Display &display, const char *server, long timeOffset, unsigned long updateInterval);

  inline void start() {
    ntp_.begin();
  }

  inline void stop() {
    ntp_.end();
  }

  inline unsigned long time() {
    return ntp_.getEpochTime();
  }

  void initialSync();
  void tick();
  void freshDisplay();

private:
  void clockInit();
  void updateDateTime(time_t time);
  void noClock();

private:
  String server_;
  NTPClient ntp_;
  WiFiUDP udp_{};

  time_t lastUpdate_{ -1 };
};
