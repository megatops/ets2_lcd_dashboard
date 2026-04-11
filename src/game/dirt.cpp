// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include "dirt.hpp"
#include <cfloat>
#include "../utils.hpp"

DirtGame::DirtGame(RacingDashboard &dash, uint16_t port)
  : Game(5 * RacingDashboard::FPS, 1000 / RacingDashboard::FPS), dash_(dash), port_(port) {}

GameState DirtGame::dirtTelemetryParse(size_t len) {
  if (len != sizeof(CodemastersAPIv3)) {
    return GameState::SERVER_DOWN;
  }

  const auto *pkt = &pkt_.apiV3;

  // never touch state_ until we can confirm we will success, so we can display
  // previous state on temporary failure.
  state_.speed = abs(round(KmConv((double)pkt->speed * 3600 / 1000)));
  state_.gear = pkt->gear;
  state_.rpmIdle = round(pkt->idle_rpm * 10);
  state_.rpm = round(pkt->engine_rate * 10);
  state_.rpmMax = round(pkt->max_rpm * 10);
  state_.fuel = (pkt->fuel_capacity > FLT_EPSILON)
                  ? round(pkt->fuel_in_tank * 100 / pkt->fuel_capacity)  // fuel available
                  : 100;                                                 // not supported, always full

  state_.lap = pkt->lap + 1;
  state_.pos = pkt->race_position;
  state_.lastLap = pkt->last_lap_time * 1000;
  state_.currLap = pkt->lap_time * 1000;

  // Codemasters will not send best lap data, so we have to calculate it by ourselves
  if (state_.lastLap == 0) {
    state_.bestLap = 0;  // new race, clear best lap
  } else if (state_.lastLap < state_.bestLap || state_.bestLap == 0) {
    state_.bestLap = state_.lastLap;
  }

  state_.isPro = true;  // there is no car class information, always use pro

  return GameState::DRIVING;
}

GameState DirtGame::getTelemetry() {
  int len = udp_.parsePacket();
  if (len <= 0) {
    return GameState::SERVER_DOWN;  // no packet
  }

  if (!IsDirtPacket(len)) {
    Serial.printf("Invalid %s packet of size %d from %s\n", name(), len, udp_.remoteIP().toString().c_str());
    return GameState::SERVER_DOWN;
  }

  int n = udp_.read(pkt_.bytes, sizeof(pkt_));
  if (n != len) {
    Serial.printf("Failed to read %s packet: %d of %d\n", name(), n, len);
    return GameState::SERVER_DOWN;
  }

  return dirtTelemetryParse(n);
}

void DirtGame::freshDisplay([[maybe_unused]] time_t time) {
  dash_.fresh(this, &state_);
}
