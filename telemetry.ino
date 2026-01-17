// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <cstring>
#include <time.h>
#include <ArduinoJson.h>
#include "ets.h"
#include "config.h"

static constexpr int JSON_FILTER_SIZE = 512;
static constexpr int JSON_DOC_SIZE = 1024;

static double km_conv(double km) {
  return SHOW_MILE ? (km / 1.61) : km;
}

// ISO8601: "0001-01-05T05:11:00Z"
static int to_minutes(const char *date) {
  struct tm tm {};
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

// only parse the fields we need to save (lots of) memory
static DeserializationOption::Filter &ets_telemetry_filter(void) {
  static StaticJsonDocument<JSON_FILTER_SIZE> f;
  static auto filter = DeserializationOption::Filter(f);

  if (f.isNull()) {
    Serial.println("Initialize JSON filter.");

    auto g = f.createNestedObject("game");
    g["connected"] = true;
    g["paused"] = true;

    auto t = f.createNestedObject("truck");
    // t["blinkerLeftActive"] = true;
    // t["blinkerRightActive"] = true;
    // t["engineOn"] = true;
    // t["lightsBeamHighOn"] = true;
    // t["lightsDashboardOn"] = true;
    // t["make"] = true;
    // t["odometer"] = true;
    t["cruiseControlOn"] = true;
    t["cruiseControlSpeed"] = true;
    t["electricOn"] = true;
    t["fuel"] = true;
    t["fuelAverageConsumption"] = true;
    t["fuelCapacity"] = true;
    t["fuelWarningOn"] = true;
    t["lightsBeamLowOn"] = true;
    t["model"] = true;
    t["speed"] = true;

    auto n = f.createNestedObject("navigation");
    n["estimatedDistance"] = true;
    n["estimatedTime"] = true;
    n["speedLimit"] = true;
  }
  return filter;
}

GameState ets_telemetry_parse(String &json, EtsState &state) {
  StaticJsonDocument<JSON_DOC_SIZE> ets;
  auto err = deserializeJson(ets, json, ets_telemetry_filter());
  if (err) {
    Serial.printf("Parsing JSON failed: %s\n", err.c_str());
    return GAME_NOT_START;
  }

  JsonObject game = ets["game"];
  if (game.isNull()) {
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

  JsonObject truck = ets["truck"];
  if (!truck.isNull()) {
    double fuel = truck["fuel"],
           tank = truck["fuelCapacity"],
           avg = truck["fuelAverageConsumption"];

    // set default fuel capacity on data error
    tank = (tank > DBL_EPSILON) ? tank : DEFAULT_TANK_SIZE;

    state.is_ev = is_ev(truck["model"]);
    state.on = truck["electricOn"];
    state.headlight = truck["lightsBeamLowOn"];
    state.speed = abs(round(km_conv((double)truck["speed"])));
    state.cruise = truck["cruiseControlOn"] ? round(km_conv(truck["cruiseControlSpeed"])) : 0;
    state.fuel = round(fuel * 100 / tank);
    state.fuel_dist = (avg > DBL_EPSILON) ? round(km_conv(fuel / avg)) : -1;  // -1 to keep previous value
    state.fuel_warn = truck["fuelWarningOn"];
  }

  JsonObject nav = ets["navigation"];
  if (!nav.isNull()) {
    state.limit = round(km_conv(nav["speedLimit"]));
    state.eta_dist = round(km_conv((double)nav["estimatedDistance"] / 1000));
    state.eta_time = to_minutes(nav["estimatedTime"]);
  }

  return GAME_DRIVING;
}
