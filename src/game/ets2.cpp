// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include "ets2.hpp"
#include <ArduinoJson.h>
#include <cfloat>
#include <cstring>
#include <TimeLib.h>
#include "../utils.hpp"

static constexpr int ETS2_MAX_FAILURE = 10;    // max retries before idle
static constexpr int ETS2_ACTIVE_DELAY = 500;  // 2Hz update
static constexpr int HTTP_CONN_TIMEOUT = 100;  // timeout for connect
static constexpr int HTTP_READ_TIMEOUT = 200;  // timeout for TCP read

// Calculated with: https://arduinojson.org/v6/assistant/
static constexpr int JSON_FILTER_SIZE = 512;
static constexpr int JSON_DOC_SIZE = 1024;

// ISO8601: "0001-01-05T05:11:00Z"
static int toMinutes(const char *date) {
  struct tm tm {};
  strptime(date, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return (tm.tm_yday * 24 + tm.tm_hour) * 60 + tm.tm_min + (tm.tm_sec < 30 ? 0 : 1);
}

static bool isEV(const char *model) {
  for (auto m = &EV_TRUCKS[0]; *m != nullptr; m++) {
    if (strcmp(model, *m) == 0) {
      return true;
    }
  }
  return false;
}

// only parse the fields we need to save (lots of) memory
//
// ArduinoJson v6 filter:
// {
//   "game": {
//     "connected": true,
//     "paused": true
//   },
//   "truck": {
//     "airPressureEmergencyOn": true,
//     "airPressureWarningOn": true,
//     "blinkerLeftActive": true,
//     "blinkerRightActive": true,
//     "cruiseControlOn": true,
//     "cruiseControlSpeed": true,
//     "electricOn": true,
//     "fuel": true,
//     "fuelAverageConsumption": true,
//     "fuelCapacity": true,
//     "fuelWarningOn": true,
//     "lightsBeaconOn": true,
//     "lightsBeamHighOn": true,
//     "lightsBeamLowOn": true,
//     "lightsBrakeOn": true,
//     "lightsParkingOn": true,
//     "model": true,
//     "parkBrakeOn": true,
//     "speed": true
//   },
//   "navigation": {
//     "estimatedTime": true,
//     "estimatedDistance": true,
//     "speedLimit": true
//   }
// }
static DeserializationOption::Filter &ets2TelemetryFilter() {
  static StaticJsonDocument<JSON_FILTER_SIZE> f;
  static auto filter = DeserializationOption::Filter(f);

  if (f.isNull()) {
    Serial.println("Initialize ETS2 JSON filter.");

    auto g = f.createNestedObject("game");
    g["connected"] = true;
    g["paused"] = true;

    auto t = f.createNestedObject("truck");
    t["airPressureEmergencyOn"] = true;
    t["airPressureWarningOn"] = true;
    t["blinkerLeftActive"] = true;
    t["blinkerRightActive"] = true;
    t["cruiseControlOn"] = true;
    t["cruiseControlSpeed"] = true;
    t["electricOn"] = true;
    t["fuel"] = true;
    t["fuelAverageConsumption"] = true;
    t["fuelCapacity"] = true;
    t["fuelWarningOn"] = true;
    t["lightsBeaconOn"] = true;
    t["lightsBeamHighOn"] = true;
    t["lightsBeamLowOn"] = true;
    t["lightsBrakeOn"] = true;
    t["lightsParkingOn"] = true;
    t["model"] = true;
    t["parkBrakeOn"] = true;
    t["speed"] = true;

    auto n = f.createNestedObject("navigation");
    n["estimatedDistance"] = true;
    n["estimatedTime"] = true;
    n["speedLimit"] = true;
  }
  return filter;
}

Ets2Game::Ets2Game(TruckDashboard &dash, const char *api)
  : Game(ETS2_MAX_FAILURE, ETS2_ACTIVE_DELAY), dash_(dash), api_(String(api)) {
  http_.setReuse(true);
}

GameState Ets2Game::ets2TelemetryParse(String &json) {
  StaticJsonDocument<JSON_DOC_SIZE> ets;
  auto err = deserializeJson(ets, json, ets2TelemetryFilter());
  if (err) {
    Serial.printf("Parsing ETS2 JSON failed: %s\n", err.c_str());
    return GameState::NOT_START;
  }

  JsonObject game = ets["game"];
  if (game.isNull()) {
    Serial.println("ETS2 JSON: no \"game\" object.");
    return GameState::NOT_START;
  }
  if (!game["connected"]) {
    Serial.println("ETS2 is not ready.");
    return GameState::NOT_START;
  }
  if (CLOCK_ENABLE && game["paused"]) {
    DEBUG("ETS2 is paused.\n");
    return GameState::READY;
  }

  // never touch state_ until we can confirm we will success, so we can display
  // previous state on temporary failure.
  JsonObject truck = ets["truck"];
  if (!truck.isNull()) {
    state_.isEV = isEV(truck["model"]);
    state_.on = truck["electricOn"];
    state_.speed = abs(round(KmConv((double)truck["speed"])));
    state_.cruise = truck["cruiseControlOn"] ? round(KmConv(truck["cruiseControlSpeed"])) : 0;

    // lights and warnings
    state_.airEmerg = truck["airPressureEmergencyOn"];
    state_.airWarn = truck["airPressureWarningOn"];
    state_.beacon = truck["lightsBeaconOn"];
    state_.brake = truck["lightsBrakeOn"];
    state_.fuelWarn = truck["fuelWarningOn"];
    state_.headlight = truck["lightsBeamLowOn"];
    state_.highBeam = truck["lightsBeamHighOn"];
    state_.leftBlinker = truck["blinkerLeftActive"];
    state_.parkBrake = truck["parkBrakeOn"];
    state_.parkingLight = truck["lightsParkingOn"];
    state_.rightBlinker = truck["blinkerRightActive"];

    // set default fuel capacity on data error
    double tank = truck["fuelCapacity"];
    tank = (tank > DBL_EPSILON) ? tank : DEFAULT_TANK_SIZE;

    double fuel = truck["fuel"];
    state_.fuel = round(fuel * 100 / tank);

    double avg = truck["fuelAverageConsumption"];
    if (avg > DBL_EPSILON) {
      state_.fuelDist = round(KmConv(fuel / avg));
    }  // else keep the previous value
  }

  JsonObject nav = ets["navigation"];
  if (!nav.isNull()) {
    state_.etaDist = round(KmConv((double)nav["estimatedDistance"] / 1000));
    state_.etaTime = toMinutes(nav["estimatedTime"]);
    state_.limit = round(KmConv(nav["speedLimit"]));
  }

  return GameState::DRIVING;
}

GameState Ets2Game::getTelemetry() {
  GameState game = GameState::SERVER_DOWN;

  http_.begin(client_, api_.c_str());
#ifndef ESP8266
  http_.setConnectTimeout(HTTP_CONN_TIMEOUT);
#endif
  http_.setTimeout(HTTP_READ_TIMEOUT);

  int http_code = http_.GET();
  if (http_code == HTTP_CODE_OK) {
    String json = http_.getString();
    game = ets2TelemetryParse(json);
  } else {
    Serial.printf("Invalid ETS2 response: %d!\n", http_code);
  }
  http_.end();

  return game;
}

void Ets2Game::freshDisplay(time_t time) {
  dash_.fresh(this, time, &state_);
}
