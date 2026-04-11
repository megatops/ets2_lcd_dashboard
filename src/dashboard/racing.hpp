// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include "dashboard.hpp"

struct RacingState {
  // car status
  int speed;
  int gear;
  int rpmIdle;
  int rpm;
  int rpmMax;
  int fuel;    // percentage
  bool isPro;  // high performance car

  // race
  int lap;
  int pos;
  int bestLap;  // msec
  int lastLap;  // msec
  int currLap;  // msec
};

class RacingDashboard : public Dashboard {
public:
  RacingDashboard(Display &display)
    : Dashboard(display){};

  void fresh(void *owner, const RacingState *state);

public:
  static constexpr int FPS = 40;  // must be called @ 40FPS

private:
  void dashboardInit();
  void updateSpeedGear(const RacingState *state);
  void updateLapTime(const RacingState *state);
  void updateCurrTime(const RacingState *state);
  void updateLapPos(const RacingState *state);
  void updateFuel(const RacingState *state);
  void updateRpm(const RacingState *state);

  void printN(int x, int y);
  void printR(int x, int y);
  void ledProgress(float load);
  void ledRedZone();

private:
  bool isPro_{};    // performance dashboard
  bool inRed_{};    // rpm currently in red zone
  int frameCnt_{};  // count blink duration
};
