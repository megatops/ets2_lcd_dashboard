// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include "game.hpp"
#include "../dashboard/truck.hpp"

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

class Ets2Game : public Game {
public:
  Ets2Game(TruckDashboard &dash, const char *api);
  GameState getTelemetry() override;
  void freshDisplay(time_t time) override;

  inline const char *name() const override {
    return "ETS2";
  }

private:
  GameState ets2TelemetryParse(String &json);

private:
  TruckDashboard &dash_;
  String api_;

  HTTPClient http_{};
  WiFiClient client_{};
  TruckState state_{};
};
