// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include "forza.hpp"
#include "../utils.hpp"

ForzaGame::ForzaGame(RacingDashboard &dash, uint16_t port)
  : Game(5 * RacingDashboard::FPS, 1000 / RacingDashboard::FPS), dash_(dash), port_(port) {}

GameState ForzaGame::forzaTelemetryParse(size_t len) {
  const ForzaSledData *sled{};
  const ForzaDashData *dash{};

  switch (len) {
    case sizeof(ForzaMotorsportPktV1):
      sled = &pkt_.motosportV1.sled;
      dash = &pkt_.motosportV1.dash;
      break;

    case sizeof(ForzaMotorsportPktV2):
      sled = &pkt_.motosportV2.sled;
      dash = &pkt_.motosportV2.dash;
      break;

    case sizeof(ForzaHorizonPkt):
      sled = &pkt_.horizon.sled;
      dash = &pkt_.horizon.dash;
      break;

    default:
      return GameState::SERVER_DOWN;
  }

  if (CLOCK_ENABLE && (sled->IsRaceOn == 0)) {
    return GameState::READY;
  }

  // never touch state_ until we can confirm we will success, so we can display
  // previous state on temporary failure.
  state_ = {
    .speed = static_cast<int>(abs(round(KmConv((double)dash->Speed * 3600 / 1000)))),
    .gear = dash->Gear,
    .rpmIdle = static_cast<int>(round(sled->EngineIdleRpm)),
    .rpm = static_cast<int>(round(sled->CurrentEngineRpm)),
    .rpmMax = static_cast<int>(round(sled->EngineMaxRpm)),
    .fuel = static_cast<int>(dash->Fuel * 100.0),
    .isPro = sled->CarClass >= FORZA_PRO_CLASS,

    .lap = dash->LapNumber + 1,
    .pos = dash->RacePosition,
    .bestLap = static_cast<int>(dash->BestLap * 1000),
    .lastLap = static_cast<int>(dash->LastLap * 1000),
    .currLap = static_cast<int>(dash->CurrentLap * 1000),
  };
  return GameState::DRIVING;
}

GameState ForzaGame::getTelemetry() {
  int len = udp_.parsePacket();
  if (len <= 0) {
    return GameState::SERVER_DOWN;  // no packet
  }

  if (!IsForzaPacket(len)) {
    Serial.printf("Invalid %s packet of size %d from %s\n", name(), len, udp_.remoteIP().toString().c_str());
    return GameState::SERVER_DOWN;
  }

  int n = udp_.read(pkt_.bytes, sizeof(pkt_));
  if (n != len) {
    Serial.printf("Failed to read %s packet: %d of %d\n", name(), n, len);
    return GameState::SERVER_DOWN;
  }

  return forzaTelemetryParse(n);
}

void ForzaGame::freshDisplay([[maybe_unused]] time_t time) {
  dash_.fresh(this, &state_);
}
