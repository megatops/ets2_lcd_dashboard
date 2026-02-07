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

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

#include "telemetry.hpp"

static constexpr int HTTP_CONN_TIMEOUT = 200;  // timeout for connect
static constexpr int HTTP_READ_TIMEOUT = 450;  // timeout for TCP read

static constexpr int JSON_FILTER_SIZE = 512;
static constexpr int JSON_DOC_SIZE = 1024;

// ISO8601: "0001-01-05T05:11:00Z"
static int ToMinutes(const char *date) {
  struct tm tm {};
  strptime(date, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return (tm.tm_yday * 24 + tm.tm_hour) * 60 + tm.tm_min + (tm.tm_sec < 30 ? 0 : 1);
}

static bool IsEV(const char *model) {
  for (auto m = &EV_TRUCKS[0]; *m != nullptr; m++) {
    if (strcmp(model, *m) == 0) {
      return true;
    }
  }
  return false;
}

// only parse the fields we need to save (lots of) memory
static DeserializationOption::Filter &ets2TelemetryFilter(void) {
  static StaticJsonDocument<JSON_FILTER_SIZE> f;
  static auto filter = DeserializationOption::Filter(f);

  if (f.isNull()) {
    Serial.println("Initialize ETS2 JSON filter.");

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

static GameState Ets2TelemetryParse(String &json, EtsState &state) {
  StaticJsonDocument<JSON_DOC_SIZE> ets;
  auto err = deserializeJson(ets, json, ets2TelemetryFilter());
  if (err) {
    Serial.printf("Parsing ETS2 JSON failed: %s\n", err.c_str());
    return GAME_NOT_START;
  }

  JsonObject game = ets["game"];
  if (game.isNull()) {
    Serial.println("ETS2 JSON: no \"game\" object.");
    return GAME_NOT_START;
  }
  if (!game["connected"]) {
    Serial.println("ETS2 is not ready.");
    return GAME_NOT_START;
  }
  if (CLOCK_ENABLE && game["paused"]) {
    Serial.println("ETS2 is paused.");
    return GAME_READY;
  }

  JsonObject truck = ets["truck"];
  if (!truck.isNull()) {
    double fuel = truck["fuel"],
           tank = truck["fuelCapacity"],
           avg = truck["fuelAverageConsumption"];

    // set default fuel capacity on data error
    tank = (tank > DBL_EPSILON) ? tank : DEFAULT_TANK_SIZE;

    state.isEV = IsEV(truck["model"]);
    state.on = truck["electricOn"];
    state.headlight = truck["lightsBeamLowOn"];
    state.speed = abs(round(KmConv((double)truck["speed"])));
    state.cruise = truck["cruiseControlOn"] ? round(KmConv(truck["cruiseControlSpeed"])) : 0;
    state.fuel = round(fuel * 100 / tank);
    state.fuelDist = (avg > DBL_EPSILON) ? round(KmConv(fuel / avg)) : -1;  // -1 to keep previous value
    state.fuelWarn = truck["fuelWarningOn"];
  }

  JsonObject nav = ets["navigation"];
  if (!nav.isNull()) {
    state.limit = round(KmConv(nav["speedLimit"]));
    state.etaDist = round(KmConv((double)nav["estimatedDistance"] / 1000));
    state.etaTime = ToMinutes(nav["estimatedTime"]);
  }

  return GAME_DRIVING;
}

GameState Ets2TelemetryQuery(HTTPClient &http, EtsState &state) {
  GameState game = GAME_SERVER_DOWN;

  static WiFiClient client;
  http.begin(client, ETS_API);
#ifndef ESP8266
  http.setConnectTimeout(HTTP_CONN_TIMEOUT);
#endif
  http.setTimeout(HTTP_READ_TIMEOUT);

  int http_code = http.GET();
  if (http_code == HTTP_CODE_OK) {
    String json = http.getString();
    game = Ets2TelemetryParse(json, state);
  } else {
    Serial.printf("Invalid ETS2 response: %d!\n", http_code);
  }
  http.end();

  return game;
}
