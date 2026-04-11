// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include "forza_udp.hpp"
#include "game.hpp"
#include "../dashboard/racing.hpp"

class ForzaGame : public Game {
public:
  ForzaGame(RacingDashboard &dash, uint16_t port);
  GameState getTelemetry() override;
  void freshDisplay(time_t time) override;

  inline const char *name() const override {
    return "Forza";
  }

  inline void start() {
    Serial.printf("Listening Forza telemetry on: %u\n", port_);
    udp_.begin(port_);
  }

  inline void stop() {
    udp_.stop();
  }

private:
  GameState forzaTelemetryParse(size_t len);

private:
  RacingDashboard &dash_;
  uint16_t port_{};

  RacingState state_{};
  WiFiUDP udp_{};
  ForzaPkt pkt_{};  // packet buffer
};
