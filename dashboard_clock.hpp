// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include "dashboard.hpp"

class ClockDashboard : public Dashboard {
public:
  ClockDashboard(Display &display);
  GameState getGameData() override;
  void freshDisplay(time_t time) override;

private:
  void clockInit();
  void updateDate(time_t time);
  void updateTime(time_t time);
  void noClock();
};
