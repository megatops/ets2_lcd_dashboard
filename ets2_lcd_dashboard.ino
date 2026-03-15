// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <NTPClient.h>
#include <SoftwareTimer.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#include "config.h"
#include "board.h"
#include "telemetry.hpp"
#include "dashboard.hpp"
#include "display.hpp"

// Server timeouts
static constexpr int ETS2_MAX_FAILURE = 10;    // max retries before idle
static constexpr int ETS2_ACTIVE_DELAY = 500;  // ETS2 API query interval in game
static constexpr int FORZA_MAX_FAILURE = 5 * FORZA_PPS;
static constexpr int FORZA_ACTIVE_DELAY = 1000 / FORZA_PPS;
static constexpr int IDLE_DELAY = 5000;            // API query interval when idle
static constexpr int INIT_DELAY = 1000;            // API query interval when just startup (quicker to back to game)
static constexpr int NTP_UPDATE = 60 * 60 * 1000;  // interval to sync clock with NTP

static WiFiUDP ntpUDP, forzaUDP;
static HTTPClient etsHTTP;
static NTPClient ntp(ntpUDP, NTP_SERVER, (TIME_ZONE - DST * 60) * 60, NTP_UPDATE);

static void WifiConnect(void (*tick)() = nullptr) {
  Serial.printf("Connecting to %s .", SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (tick) {
      (*tick)();
    }
  }

  Serial.printf(" Local IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Listening Forza telemetry on: %u\n", FORZA_PORT);
  forzaUDP.begin(FORZA_PORT);
}

static void ClockInitialSync() {
  Serial.printf("NTP syncing with %s .", NTP_SERVER);

  ntp.begin();
  while (!ntp.forceUpdate()) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf(" Local Time: %s\n", ntp.getFormattedTime().c_str());
  ClockUpdate(ntp.getEpochTime());
}

static void ClockTick() {
  if (!CLOCK_ENABLE || dashMode != DASH_CLOCK) {
    return;
  }

  static time_t lastUpdate = -1;
  if (ntp.getEpochTime() == lastUpdate) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED && ntp.update()) {
    Serial.printf("NTP sync success: %s.\n", ntp.getFormattedTime().c_str());
  }

  // the time may have been updated by NTP, fetch it again
  lastUpdate = ntp.getEpochTime();
  ClockUpdate(lastUpdate);
}

static SoftwareTimer dashboardTimer(INIT_DELAY, &DashboardTimerFunc);
static TimerState timerState = TimerState::INIT;

static void DashboardTimerSetState(TimerState state) {
  static constexpr unsigned long INTERVAL[TimerState::STATE_MAX]{
    [TimerState::INIT] = INIT_DELAY,
    [TimerState::IDLE] = IDLE_DELAY,
    [TimerState::ETS2_ACIVE] = ETS2_ACTIVE_DELAY,
    [TimerState::FORZA_ACTIVE] = FORZA_ACTIVE_DELAY,
  };

  if (state != timerState) {
    timerState = state;
    dashboardTimer.setInterval(INTERVAL[state]);
  }
}

static void DashboardTimerStateTrans(const char *name, GameState game, int maxFailure, TimerState activeState) {
  static int failed;

  if (game >= GameState::READY) {
    // the game is running, speed up polling for faster responses
    if (timerState != activeState) {
      Serial.printf("%s become active.\n", name);
      DashboardTimerSetState(activeState);
    }
    failed = 0;

  } else if ((timerState == activeState) && (++failed >= maxFailure)) {
    // slow down the frequency of queries to reduce the API
    // server (running on your gaming PC) resource usage
    Serial.printf("%s is inactive.\n", name);
    DashboardTimerSetState(TimerState::IDLE);
  }
}

static bool IsDriving(GameState game) {
  static bool driving;
  if (timerState > TimerState::IDLE) {
    switch (game) {
      case GameState::DRIVING:
        driving = true;
        break;
      case GameState::READY:  // game paused, quit driving mode
        driving = false;
        break;
      default:
        // for temporary failure, preserve the last mode
        // until it recovered, or completely inactive.
        break;
    }
  } else {
    driving = false;  // inactive
  }
  return driving;
}

static void DashboardTimerFunc() {
  GameState game = GameState::SERVER_DOWN;
  EtsState etsState{};
  ForzaState forzaState{};

  // ETS2 query
  if (timerState <= TimerState::IDLE || timerState == TimerState::ETS2_ACIVE) {
    game = Ets2TelemetryQuery(etsHTTP, etsState);
    DashboardTimerStateTrans("ETS2", game, ETS2_MAX_FAILURE, TimerState::ETS2_ACIVE);
  }

  // Forza query
  if (timerState <= TimerState::IDLE || timerState == TimerState::FORZA_ACTIVE) {
    game = ForzaTelemetryQuery(forzaUDP, forzaState);
    DashboardTimerStateTrans("Forza", game, FORZA_MAX_FAILURE, TimerState::FORZA_ACTIVE);
  }

  if (IsDriving(game)) {
    // show cached state on temporary failure.
    bool fresh = (game == GameState::DRIVING);
    switch (timerState) {
      case TimerState::ETS2_ACIVE:
        Ets2DashboardUpdate(fresh ? &etsState : nullptr, ntp.getEpochTime());
        break;
      case TimerState::FORZA_ACTIVE:
        ForzaDashboardUpdate(fresh ? &forzaState : nullptr);
        break;
      default:
        Serial.println("Unexpected dashboard state!");
        break;
    }

  } else if (dashMode != DASH_CLOCK) {
    // need to switch to clock mode (not driving, or inactive)
    ClockUpdate(ntp.getEpochTime());
  }
  // otherwise let clock_tick() to update the clock display
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  display.start();
  etsHTTP.setReuse(true);
  WifiConnect();

  if (CLOCK_ENABLE) {
    ClockInitialSync();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected.");
    ntp.end();
    forzaUDP.stop();
    WifiConnect(&ClockTick);
    ntp.begin();
  }

  dashboardTimer.tick();
  ClockTick();
}
