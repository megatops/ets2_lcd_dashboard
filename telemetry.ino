// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <cstring>
#include <time.h>
#include <Arduino_JSON.h>
#include "ets.h"
#include "config.h"

static double km_conv(double km) {
  return SHOW_MILE ? (km / 1.6) : km;
}

// ISO8601: "0001-01-05T05:11:00Z"
static int to_minutes(const char *date) {
  struct tm tm = {};
  strptime(date, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return (tm.tm_yday * 24 + tm.tm_hour) * 60 + tm.tm_min + (tm.tm_sec < 30 ? 0 : 1);
}

static bool is_ev(const char *model) {
  for (auto m = &EV_TRUCKS[0]; *m != nullptr; m++) {
    if (strcmp(model, *m) == 0) {
      return true;
    }
  }
  return false;
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
    double fuel = truck["fuel"],
           tank = truck["fuelCapacity"],
           avg = truck["fuelAverageConsumption"];

    state->is_ev = is_ev(truck["model"]);
    state->started = truck["engineOn"];
    state->headlight = truck["lightsBeamLowOn"];
    state->speed = abs(round(km_conv((double)truck["speed"])));
    state->cruise = truck["cruiseControlOn"] ? km_conv(truck["cruiseControlSpeed"]) : 0;
    state->fuel = (tank > DBL_EPSILON) ? round(fuel * 100 / tank) : 0;
    state->fuel_dist = (avg > DBL_EPSILON) ? round(km_conv(fuel / avg)) : -1;  // -1 to keep previous value
  }

  auto nav = ets["navigation"];
  if (JSON.typeof(nav).equals("object")) {
    state->limit = km_conv(nav["speedLimit"]);
    state->eta_dist = round(km_conv((double)nav["estimatedDistance"] / 1000));
    state->eta_time = to_minutes(nav["estimatedTime"]);
  }

  return GAME_DRIVING;
}
