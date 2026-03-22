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

struct ForzaState {
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

class ForzaGame : public Game {
public:
  ForzaGame(Display &display, uint16_t port);
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
  void dashboardInit();
  void updateSpeedGear();
  void updateLapTime();
  void updateCurrTime();
  void updateLapPos();
  void updateFuel();
  void updateRpm();

  void printN(int x, int y);
  void ledProgress(float load);
  void ledRedZone();
  GameState forzaTelemetryParse(size_t len);

private:
  uint16_t port_{};
  WiFiUDP udp_{};

  ForzaState state_{};
  bool isPro_{};    // performance dashboard
  bool inRed_{};    // rpm currently in red zone

  int frameCnt_{};  // count blink duration
  ForzaPkt pkt_{};  // packet buffer
};
