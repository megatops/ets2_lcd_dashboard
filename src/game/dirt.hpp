// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include "dirt_udp.hpp"
#include "game.hpp"
#include "../dashboard/racing.hpp"

class DirtGame : public Game {
public:
  DirtGame(RacingDashboard &dash, uint16_t port);
  GameState getTelemetry() override;
  void freshDisplay(time_t time) override;

  inline const char *name() const override {
    return "DiRT Rally";
  }

  inline void start() override {
    Serial.printf("Listening %s telemetry on: %u\n", name(), port_);
    udp_.begin(port_);
  }

  inline void stop() override {
    udp_.stop();
  }

private:
  GameState dirtTelemetryParse(size_t len);

private:
  RacingDashboard &dash_;
  uint16_t port_{};

  RacingState state_{};
  WiFiUDP udp_{};
  DirtPkt pkt_{};  // packet buffer
};
