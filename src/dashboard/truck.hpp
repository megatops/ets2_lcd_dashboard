// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <Arduino.h>
#include "dashboard.hpp"

struct TruckState {
  // electric truck
  bool isEV;

  // for brightness control
  bool on;
  bool headlight;  // low beam

  // for LED indicators
  bool parkingLight;
  bool highBeam;
  bool leftBlinker;
  bool rightBlinker;
  bool beacon;
  bool brake;
  bool parkBrake;
  bool airWarn;
  bool airEmerg;

  // truck status
  bool fuelWarn;
  int fuelDist;
  int fuel;    // percentage
  int cruise;  // 0 for off
  int speed;

  // navigation
  int etaDist;
  int etaTime;  // minutes
  int limit;    // 0 for no limit
};

class TruckDashboard : public Dashboard {
public:
  TruckDashboard(Display &display)
    : Dashboard(display){};

  void fresh(void *owner, time_t time, const TruckState *state);

public:
  static constexpr int FPS = 2;  // must be called @ 2FPS

private:
  void dashboardInit();
  void updateSpeed(const TruckState *state);
  void updateEta(const TruckState *state);
  void updateFuel(const TruckState *state);
  void updateLEDs(const TruckState *state);
  void updateClock(time_t time);
};
