// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <time.h>
#include <Arduino_JSON.h>
#include "ets.h"

// ISO8601: "0001-01-05T05:11:00Z"
static int to_minutes(const char *date) {
  struct tm tm = {};
  strptime(date, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return (tm.tm_yday * 24 + tm.tm_hour) * 60 + tm.tm_min + (tm.tm_sec < 30 ? 0 : 1);
}

bool ets_telemetry_parse(String &json, EtsState *stat) {
  JSONVar ets = JSON.parse(json);
  if (JSON.typeof(ets) == "undefined") {
    Serial.println("Parsing JSON failed!");
    return false;
  }

  auto game = ets["game"];
  if (!JSON.typeof(game).equals("object")) {
    Serial.println("JSON: no \"game\" object.");
    return false;
  }
  if (!game["connected"]) {
    Serial.println("Game is not ready.");
    return false;
  }

  auto truck = ets["truck"];
  if (JSON.typeof(truck).equals("object")) {
    stat->speed = abs(round((double)truck["speed"]));
    if (truck["cruiseControlOn"]) {
      stat->cruise = truck["cruiseControlSpeed"];
    } else {
      stat->cruise = 0;
    }

    double fuel = truck["fuel"];
    double tank = truck["fuelCapacity"];
    if (tank > DBL_EPSILON) {
      stat->fuel = round(fuel * 100 / tank);
    }
    double avg = truck["fuelAverageConsumption"];
    if (avg > DBL_EPSILON) {
      stat->fuel_dist = round(fuel / avg);
    } else {
      stat->fuel_dist = -1;  // keep previous value
    }
  }

  auto nav = ets["navigation"];
  if (JSON.typeof(nav).equals("object")) {
    stat->limit = nav["speedLimit"];
    stat->eta_dist = round((double)nav["estimatedDistance"] / 1000);
    stat->eta_time = to_minutes(nav["estimatedTime"]);
  }

  return true;
}
