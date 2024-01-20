// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <time.h>
#include <Arduino_JSON.h>
#include "ets.h"

static double km_conv(double km) {
  return SHOW_MILE ? (km / 1.6) : km;
}

// ISO8601: "0001-01-05T05:11:00Z"
static int to_minutes(const char *date) {
  struct tm tm = {};
  strptime(date, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return (tm.tm_yday * 24 + tm.tm_hour) * 60 + tm.tm_min + (tm.tm_sec < 30 ? 0 : 1);
}

GameState ets_telemetry_parse(String &json, EtsState *state) {
  JSONVar ets = JSON.parse(json);
  if (JSON.typeof(ets) == "undefined") {
    Serial.println("Parsing JSON failed!");
    return GAME_NOT_START;
  }

  auto game = ets["game"];
  if (!JSON.typeof(game).equals("object")) {
    Serial.println("JSON: no \"game\" object.");
    return GAME_NOT_START;
  }
  if (!game["connected"]) {
    Serial.println("Game is not ready.");
    return GAME_NOT_START;
  }
  if (CLOCK_ENABLE && game["paused"]) {
    Serial.println("Game is paused.");
    return GAME_READY;
  }

  auto truck = ets["truck"];
  if (JSON.typeof(truck).equals("object")) {
    state->speed = abs(round(km_conv((double)truck["speed"])));
    if (truck["cruiseControlOn"]) {
      state->cruise = km_conv(truck["cruiseControlSpeed"]);
    } else {
      state->cruise = 0;
    }

    double fuel = truck["fuel"];
    double tank = truck["fuelCapacity"];
    if (tank > DBL_EPSILON) {
      state->fuel = round(fuel * 100 / tank);
    }
    double avg = truck["fuelAverageConsumption"];
    if (avg > DBL_EPSILON) {
      state->fuel_dist = round(km_conv(fuel / avg));
    } else {
      state->fuel_dist = -1;  // keep previous value
    }
  }

  auto nav = ets["navigation"];
  if (JSON.typeof(nav).equals("object")) {
    state->limit = km_conv(nav["speedLimit"]);
    state->eta_dist = round(km_conv((double)nav["estimatedDistance"] / 1000));
    state->eta_time = to_minutes(nav["estimatedTime"]);
  }

  return GAME_DRIVING;
}
