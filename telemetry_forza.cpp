// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include "telemetry.hpp"

static GameState ForzaTelemetryParse(const ForzaPkt &pkt, size_t len, ForzaState &state) {
  const ForzaSledData *sled = nullptr;
  const ForzaDashData *dash = nullptr;

  switch (len) {
    case sizeof(ForzaMotorsportPktV1):
      sled = &pkt.motosportV1.sled;
      dash = &pkt.motosportV1.dash;
      break;

    case sizeof(ForzaMotorsportPktV2):
      sled = &pkt.motosportV2.sled;
      dash = &pkt.motosportV2.dash;
      break;

    case sizeof(ForzaHorizonPkt):
      sled = &pkt.horizon.sled;
      dash = &pkt.horizon.dash;
      break;

    default:
      return GAME_SERVER_DOWN;
  }

  if (CLOCK_ENABLE && (sled->IsRaceOn == 0)) {
    return GAME_READY;
  }

  state.speed = abs(round(KmConv((double)dash->Speed * 60 * 60 / 1000)));
  state.gear = dash->Gear;
  state.rpmIdle = round(sled->EngineIdleRpm);
  state.rpmMax = round(sled->EngineMaxRpm);
  state.rpm = round(sled->CurrentEngineRpm);
  state.fuel = round(dash->Fuel * 100.0);
  state.lap = dash->LapNumber + 1;
  state.pos = dash->RacePosition;
  state.bestLap = round(dash->BestLap * 1000);
  state.lastLap = round(dash->LastLap * 1000);
  state.currLap = round(dash->CurrentLap * 1000);
  state.isPro = sled->CarClass >= FORZA_PRO_CLASS;
  return GAME_DRIVING;
}

GameState ForzaTelemetryQuery(WiFiUDP &udp, ForzaState &state) {
  int len = udp.parsePacket();
  if (len <= 0) {
    return GAME_SERVER_DOWN;  // no packet
  }

  if (!IsForzaPacket(len)) {
    Serial.printf("Invalid Forza packet of size %d from %s\n", len, udp.remoteIP().toString().c_str());
    return GAME_SERVER_DOWN;
  }

  static ForzaPkt pkt;
  int n = udp.read(pkt.bytes, sizeof(pkt));
  if (n != len) {
    Serial.printf("Failed to read Forza packet: %d of %d\n", n, len);
    return GAME_SERVER_DOWN;
  }

  return ForzaTelemetryParse(pkt, n, state);
}
