// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
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

static void ClockInitialSync(void) {
  Serial.printf("NTP syncing with %s .", NTP_SERVER);

  ntp.begin();
  while (!ntp.forceUpdate()) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf(" Local Time: %s\n", ntp.getFormattedTime().c_str());
  ClockUpdate(ntp.getEpochTime());
}

static void ClockTick(void) {
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

static void DashboardTimerAdjust(const char *name, bool &active, int &failed,
                                 GameState game, int maxFailure, int activeDelay) {
  if (game >= GAME_READY) {
    // the game is running, speed up polling for faster responses
    if (!active) {
      Serial.printf("%s become active.\n", name);
      dashboardTimer.setInterval(activeDelay);
      active = true;
    }
    failed = 0;

  } else if (active && ++failed >= maxFailure) {
    // slow down the frequency of queries to reduce the API
    // server (running on your gaming PC) resource usage
    Serial.printf("%s is inactive.\n", name);
    dashboardTimer.setInterval(IDLE_DELAY);
    active = false;
  }
}

static void DashboardTimerFunc(void) {
  static int failed;
  bool forzaActive = false, ets2Active = false;

  switch (dashboardTimer.getInterval()) {
    case FORZA_ACTIVE_DELAY:
      forzaActive = true;
      break;
    case ETS2_ACTIVE_DELAY:
      ets2Active = true;
      break;
    default:
      break;
  }

  GameState game = GAME_SERVER_DOWN;
  EtsState etsState{};
  ForzaState forzaState{};

  if (!forzaActive) {
    // ETS2 query
    game = Ets2TelemetryQuery(etsHTTP, etsState);
    DashboardTimerAdjust("ETS2 Telemetry Server", ets2Active, failed,
                         game, ETS2_MAX_FAILURE, ETS2_ACTIVE_DELAY);
  }

  if (!ets2Active) {
    // Forza query
    game = ForzaTelemetryQuery(forzaUDP, forzaState);
    DashboardTimerAdjust("Forza", forzaActive, failed,
                         game, FORZA_MAX_FAILURE, FORZA_ACTIVE_DELAY);
  }

  static bool driving;
  if (ets2Active || forzaActive) {
    switch (game) {
      case GAME_DRIVING:
        driving = true;
        break;
      case GAME_READY:  // game paused, quit driving mode
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

  if (driving) {
    // show cached state on temporary failure.
    bool fresh = (game == GAME_DRIVING);
    if (ets2Active) {
      Ets2DashboardUpdate(fresh ? &etsState : nullptr, ntp.getEpochTime());
    } else if (forzaActive) {
      ForzaDashboardUpdate(fresh ? &forzaState : nullptr);
    } else {
      Serial.println("Unexpected dashboard state!");
      driving = false;
    }
  } else if (dashMode != DASH_CLOCK) {
    // need to switch to clock mode (not driving, or inactive)
    ClockUpdate(ntp.getEpochTime());
  }
  // otherwise let clock_tick() to update the clock display
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  pinMode(LCD_LED_PWM, OUTPUT);
  pinMode(RGB_LED_PIN, OUTPUT);
  LcdInit(I2C_SDA, I2C_SCL, I2C_FREQ);
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
