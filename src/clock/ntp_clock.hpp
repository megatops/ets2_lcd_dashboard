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
#include "../dashboard/clock.hpp"

class NtpClock {
public:
  NtpClock(ClockDashboard &dash, const char *server, long timeOffset, unsigned long updateInterval);

  inline void start() {
    ntp_.begin();
  }

  inline void stop() {
    ntp_.end();
  }

  inline unsigned long time() {
    return ntp_.getEpochTime();
  }

  inline bool inDisplay() {
    return dash_.getDisplay().isOwnedBy(this);
  }

  void initialSync();
  void tick();
  void freshDisplay();

private:
  ClockDashboard &dash_;
  String server_;
  NTPClient ntp_;
  WiFiUDP udp_{};

  time_t lastUpdate_{ -1 };
};
