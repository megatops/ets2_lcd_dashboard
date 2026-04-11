// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include "ntp_clock.hpp"
#include <TimeLib.h>
#include "../../config.h"
#include "../utils.hpp"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

NtpClock::NtpClock(ClockDashboard &dash, const char* server, long timeOffset, unsigned long updateInterval)
  : dash_(dash), ntp_(NTPClient(udp_, server, timeOffset, updateInterval)) {}

void NtpClock::initialSync() {
  Serial.printf("NTP syncing with %s .", server_.c_str());
  while (!ntp_.forceUpdate()) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" Local Time: %s\n", ntp_.getFormattedTime().c_str());
}

void NtpClock::tick() {
  if (!CLOCK_ENABLE || !inDisplay()) {
    return;  // avoid sync in game to prevent unexpected latency
  }

  if (ntp_.getEpochTime() == lastUpdate_) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED && ntp_.update()) {
    Serial.printf("NTP sync success: %s.\n", ntp_.getFormattedTime().c_str());
  }

  freshDisplay();
}

void NtpClock::freshDisplay() {
  dash_.fresh(this, CLOCK_ENABLE ? ntp_.getEpochTime() : 0);
}
