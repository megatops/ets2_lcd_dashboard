// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#include <Arduino.h>
#include <WiFiUdp.h>

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

#include "forza_udp.hpp"
#include "config.h"

struct EtsState {
  // electric truck
  bool isEV;

  // for brightness control
  bool on;
  bool headlight;  // low beam

  // for LED indicators
  bool highBeam;
  bool leftBlinker;
  bool rightBlinker;
  bool parkingLight;
  bool parkBrake;

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
  float bestLap;  // msec
  float lastLap;  // msec
  float currLap;  // msec
};

enum GameState {
  GAME_SERVER_DOWN,
  GAME_NOT_START,

  GAME_READY,
  GAME_DRIVING,  // driving the truck
};

constexpr bool DEBUG_ENABLE = false;  // verbose serial debug info

#define DEBUG(...) \
  do { \
    if (DEBUG_ENABLE) { \
      Serial.printf(__VA_ARGS__); \
    } \
  } while (0)

static inline double KmConv(double km) {
  return SHOW_MILE ? (km / 1.61) : km;
}

GameState Ets2TelemetryQuery(HTTPClient &http, EtsState &state);
GameState ForzaTelemetryQuery(WiFiUDP &udp, ForzaState &state);

#endif
