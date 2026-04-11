// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <Arduino.h>
#include "dashboard.hpp"

class ClockDashboard : public Dashboard {
public:
  ClockDashboard(Display &display)
    : Dashboard(display){};

  void fresh(void *owner, time_t time);

private:
  void clockInit();
  void updateDateTime(time_t time);
  void noClock();
};
