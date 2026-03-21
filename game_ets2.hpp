// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include "game.hpp"

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

struct EtsState {
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

class Ets2Game : public Game {
public:
  Ets2Game(Display &display, const char *api);
  GameState getTelemetry() override;
  void freshDashboard(time_t time) override;

  inline const char *name() const override {
    return "ETS2";
  }

private:
  void dashboardInit();
  void updateSpeed();
  void updateEtaDist();
  void updateEtaTime();
  void updateCruise();
  void updateLimit();
  void updateFuel();
  void updateLEDs();
  void updateClock(time_t time);

  GameState ets2TelemetryParse(String &json);

private:
  String api_;

  HTTPClient http_{};
  WiFiClient client_{};
  EtsState state_{};
};
