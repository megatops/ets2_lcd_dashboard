// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// +----+----+----+----
// 00:00 >8888 km 00:00
// CruisBigBigBig Limit
// [888]NumNumNum [888]
// Fuel ####...... 8888
// +----+----+----+----

#include "game_ets2.hpp"
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

Ets2Game::Ets2Game(Display &display, const char *api)
  : Game(display, ETS2_MAX_FAILURE, ETS2_ACTIVE_DELAY), api_(String(api)) {
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

void Ets2Game::updateEta() {
  auto etaDist = min(state_.etaDist, 9999);
  LAZY_UPDATE(etaDist, {
    disp_.setCursor(8, 0);
    if (etaDist >= 1000) {
      disp_.printf("%4d", etaDist);  // 8888km
    } else {
      disp_.printf("%3d ", etaDist);  // 888 km
    }
    DEBUG("Update ETA distance: %d\n", etaDist);
  });

  auto etaTime = min(state_.etaTime, 99 * 60 + 59);
  LAZY_UPDATE(etaTime, {
    dispPrintf(15, 0, "%02d:%02d", etaTime / 60, etaTime % 60);
    DEBUG("Update ETA time: %d\n", etaTime);
  });
}

void Ets2Game::updateSpeed() {
  auto speed = min(state_.speed, 199);
  LAZY_UPDATE(speed, {
    disp_.printLarge(5, 1, speed, 3, false);
    DEBUG("Update speed: %d\n", speed);
  });

  auto cruise = min(state_.cruise, 999);
  LAZY_UPDATE(cruise, {
    disp_.setCursor(1, 2);
    if (cruise > 0) {
      disp_.printf("%3d", cruise);
    } else {
      disp_.print("---");
    }
    DEBUG("Update cruise: %d\n", cruise);
  });

  auto limit = min(state_.limit, 999);
  LAZY_UPDATE(limit, {
    disp_.setCursor(16, 2);
    if (limit > 0) {
      disp_.printf("%3d", limit);
    } else {
      disp_.print("---");
    }
    DEBUG("Update speed limit: %d\n", limit);
  });

  // blink the label as speeding warning
  bool speeding = (state_.limit > 0) && (state_.speed > state_.limit);
  auto label = BLINK_IF(speeding, "Limit", "     ");
  LAZY_UPDATE(label, dispPrint(15, 1, label));
}

void Ets2Game::updateFuel() {
  auto fuelDist = min(state_.fuelDist, 9999);
  LAZY_UPDATE(fuelDist, {
    dispPrintf(16, 3, "%4d", fuelDist);
    DEBUG("Update fuel distance: %d\n", fuelDist);
  });

  auto fuel = min(state_.fuel, 100);
  int seg = round(fuel / (100.0 / 10));
  LAZY_UPDATE(seg, {
    char bar[] = "\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    dispPrint(5, 3, bar);
    DEBUG("Update fuel: %d%%\n", fuel);
  });

  // blink the label as fuel warning
  auto label = BLINK_IF(state_.fuelWarn, state_.isEV ? "Batt" : "Fuel", "    ");
  LAZY_UPDATE(label, dispPrint(0, 3, label));
}

void Ets2Game::updateClock(time_t time) {
  int h = CLOCK_12H ? hourFormat12(time) : hour(time),
      m = minute(time);
  LAZY_UPDATE(h, dispPrintf(0, 0, "%02d", h));
  LAZY_UPDATE(m, dispPrintf(3, 0, "%02d", m));

  auto label = BLINK_IF(CLOCK_BLINK, ":", " ");
  LAZY_UPDATE(label, dispPrint(2, 0, label));
}

void Ets2Game::updateLEDs() {
  bool changed = false;  // need to fresh LED bar

  // special: blinkers
  if (state_.leftBlinker) {
    disp_.ledSet(LedSlot::LBLINKER, blinkShow_ ? LED_INFO : LED_OFF);
    changed = true;
  }
  LAZY_UPDATE(state_.leftBlinker, {
    if (!state_.leftBlinker) {
      disp_.ledSet(LedSlot::LBLINKER, LED_OFF);
      changed = true;
    }
    DEBUG("Update leftBlinker: %d\n", state_.leftBlinker);
  });

  if (state_.rightBlinker) {
    disp_.ledSet(LedSlot::RBLINKER, blinkShow_ ? LED_INFO : LED_OFF);
    changed = true;
  }
  LAZY_UPDATE(state_.rightBlinker, {
    if (!state_.rightBlinker) {
      disp_.ledSet(LedSlot::RBLINKER, LED_OFF);
      changed = true;
    }
    DEBUG("Update rightBlinker: %d\n", state_.rightBlinker);
  });

  // special: multi-state
  int airWarnLevel = state_.airEmerg ? 2 : (state_.airWarn ? 1 : 0);
  LAZY_UPDATE(airWarnLevel, {
    disp_.ledSet(LedSlot::AIR_WARN, (airWarnLevel == 2) ? LED_ALERT : ((airWarnLevel == 1) ? LED_WARN : LED_OFF));
    changed = true;
    DEBUG("Update airWarnLevel: %d\n", airWarnLevel);
  });

#define UPDATE_INDICATOR(flag, slot, color) \
  do { \
    LAZY_UPDATE((flag), { \
      disp_.ledSet((slot), (flag) ? (color) : LED_OFF); \
      changed = true; \
      DEBUG("Update " #flag ": %d\n", (flag)); \
    }); \
  } while (0)

  // normal on/off indicators
  bool lowBeam = state_.parkingLight && state_.headlight,
       highBeam = state_.parkingLight && state_.highBeam;
  UPDATE_INDICATOR(lowBeam, LedSlot::LOW_BEAM, LED_INFO);
  UPDATE_INDICATOR(highBeam, LedSlot::HIGH_BEAM, LED_NOTICE);
  UPDATE_INDICATOR(state_.beacon, LedSlot::BEACON, LED_WARN);
  UPDATE_INDICATOR(state_.brake, LedSlot::BRAKE, LED_INFO);
  UPDATE_INDICATOR(state_.parkBrake, LedSlot::PARK_BRAKE, LED_ALERT);

#undef UPDATE_INDICATOR

  if (changed) {
    disp_.ledShow();
  }
}

void Ets2Game::dashboardInit() {
  if (!force_) {
    return;
  }
  disp_.ledOFF();
  dispPrintf(0, 0, "%s \x7e     %s   :  ", CLOCK_ENABLE ? "     " : "Navi:", SHOW_MILE ? "mi" : "km");
  dispPrint(0, 1, "Cruis               ");
  dispPrint(0, 2, "[   ]          [   ]");
  dispPrint(0, 3, "                    ");
}

void Ets2Game::freshDisplay(time_t time) {
  // owner change needs a full update
  force_ = disp_.setOwner(this);
  blinkShow_ = !blinkShow_;  // 1Hz @ 2FPS

  // no backlight when engine off, dim when headlight on
  int backLight = state_.headlight ? BACKLIGHT_NIGHT : BACKLIGHT_DAY;
  disp_.backlightUpdate(force_, state_.on ? backLight : BACKLIGHT_OFF);
  int ledLight = state_.headlight ? RGB_LEVEL_NIGHT : RGB_LEVEL_DAY;
  bool freshLed = disp_.ledBrightnesslUpdate(force_, state_.on ? ledLight : RGB_LEVEL_OFF);

  dashboardInit();
  if (CLOCK_ENABLE) {
    updateClock(time);
  }
  updateSpeed();
  updateEta();
  updateFuel();

  // dynamic RGB brightness change needs a full update (workaround for NeoPixel limitation)
  force_ |= freshLed;
  updateLEDs();
}
