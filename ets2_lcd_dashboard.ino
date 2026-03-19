// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <SoftwareTimer.h>
#include <TimeLib.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include "config.h"
#include "board.h"
#include "display.hpp"
#include "ntp_clock.hpp"
#include "dashboard_ets2.hpp"
#include "dashboard_forza.hpp"

// Server timeouts
static constexpr int ETS2_MAX_FAILURE = 10;    // max retries before idle
static constexpr int ETS2_ACTIVE_DELAY = 500;  // ETS2 API query interval in game
static constexpr int FORZA_MAX_FAILURE = 5 * FORZA_PPS;
static constexpr int FORZA_ACTIVE_DELAY = 1000 / FORZA_PPS;
static constexpr int IDLE_DELAY = 5000;            // API query interval when idle
static constexpr int INIT_DELAY = 1000;            // API query interval when just startup (quicker to back to game)
static constexpr int NTP_UPDATE = 60 * 60 * 1000;  // interval to sync clock with NTP

static Display disp(I2C_SDA, I2C_SCL, I2C_FREQ, LCD_LED_PWM, RGB_LED_PIN, LCD_ADDR);
static NtpClock ntpClock(disp, NTP_SERVER, (TIME_ZONE - DST * 60) * 60, NTP_UPDATE);
static Ets2Dashboard ets2Dash(disp, ETS_API);
static ForzaDashboard forzaDash(disp, FORZA_PORT);

static void WifiConnect(std::function<void()> tick) {
  Serial.printf("Connecting to %s .", SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tick();
  }

  Serial.printf(" Local IP: %s\n", WiFi.localIP().toString().c_str());
  ntpClock.connect();
  forzaDash.listen();
}

static void ClockInitialSync() {
  ntpClock.initialSync();
  ntpClock.freshDisplay();
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

  // ETS2 query
  if (timerState <= TimerState::IDLE || timerState == TimerState::ETS2_ACIVE) {
    game =  ets2Dash.getGameData();
    DashboardTimerStateTrans("ETS2", game, ETS2_MAX_FAILURE, TimerState::ETS2_ACIVE);
  }

  // Forza query
  if (timerState <= TimerState::IDLE || timerState == TimerState::FORZA_ACTIVE) {
    game = forzaDash.getGameData();
    DashboardTimerStateTrans("Forza", game, FORZA_MAX_FAILURE, TimerState::FORZA_ACTIVE);
  }

  if (IsDriving(game)) {
    switch (timerState) {
      case TimerState::ETS2_ACIVE:
        ets2Dash.freshDisplay(ntpClock.getEpochTime());
        break;
      case TimerState::FORZA_ACTIVE:
        forzaDash.freshDisplay(ntpClock.getEpochTime());
        break;
      default:
        Serial.println("Unexpected dashboard state!");
        break;
    }

  } else if (disp.mode != DisplayMode::CLOCK) {
    // need to switch to clock mode (not driving, or inactive)
    ntpClock.freshDisplay();
  }
  // otherwise let clock_tick() to update the clock disp
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  disp.start();
  WifiConnect([]{});

  if (CLOCK_ENABLE) {
    ClockInitialSync();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected.");
    ntpClock.disconnect();
    forzaDash.stop();
    WifiConnect([]{ntpClock.tick();});
  }

  dashboardTimer.tick();
  ntpClock.tick();
}
