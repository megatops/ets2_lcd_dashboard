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

#include "ets.h"
#include "config.h"
#include "board.h"

// Server timeouts
static constexpr int MAX_FAILURE = 10;             // max retries before idle
static constexpr int ACTIVE_DELAY = 500;           // API query interval in game
static constexpr int IDLE_DELAY = 5000;            // API query interval when idle
static constexpr int NTP_UPDATE = 60 * 60 * 1000;  // interval to sync clock with NTP
static constexpr int HTTP_CONN_TIMEOUT = 200;      // timeout for connect
static constexpr int HTTP_READ_TIMEOUT = 450;      // timeout for TCP read

static WiFiClient client;
static WiFiUDP ntpUDP;
static HTTPClient http;
static NTPClient ntp(ntpUDP, NTP_SERVER, (TIME_ZONE - DST * 60) * 60, NTP_UPDATE);

static void wifi_connect(void (*tick)() = nullptr) {
  Serial.printf("Connecting to %s .", SSID);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (tick) {
      (*tick)();
    }
  }

  Serial.print(" Local IP: ");
  Serial.println(WiFi.localIP());
}

static void clock_initial_sync(void) {
  Serial.printf("NTP syncing with %s .", NTP_SERVER);

  ntp.begin();
  while (!ntp.forceUpdate()) {
    delay(500);
    Serial.print(".");
  }

  Serial.print(" Local Time: ");
  Serial.println(ntp.getFormattedTime());
  clock_update(ntp.getEpochTime());
}

static void clock_tick(void) {
  if (!CLOCK_ENABLE || lcd_get_mode() != LCD_CLOCK) {
    return;
  }

  static time_t last_update = -1;
  if (ntp.getEpochTime() == last_update) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED && ntp.update()) {
    Serial.printf("NTP sync success: %s.\n", ntp.getFormattedTime().c_str());
  }

  // the time may have been updated by NTP, fetch it again
  last_update = ntp.getEpochTime();
  clock_update(last_update);
}

static GameState ets_telemetry_query(EtsState &state) {
  GameState game = GAME_SERVER_DOWN;

  http.begin(client, ETS_API);
#ifndef ESP8266
  http.setConnectTimeout(HTTP_CONN_TIMEOUT);
#endif
  http.setTimeout(HTTP_READ_TIMEOUT);

  int http_code = http.GET();
  if (http_code == HTTP_CODE_OK) {
    String json = http.getString();
    game = ets_telemetry_parse(json, state);
  } else {
    Serial.printf("Invalid response: %d!\n", http_code);
  }
  http.end();

  return game;
}

static SoftwareTimer dashboard_timer(ACTIVE_DELAY, &dashboard_timer_func);

static void dashboard_timer_func(void) {
  static int failed;
  bool active = (dashboard_timer.getInterval() == ACTIVE_DELAY);

  // try to query the telemetry API server
  EtsState state{};
  GameState game = ets_telemetry_query(state);

  if (game >= GAME_READY) {
    // the game is running, speed up polling for faster responses
    if (!active) {
      dashboard_timer.setInterval(ACTIVE_DELAY);
    }
    failed = 0;

  } else if (active && ++failed >= MAX_FAILURE) {
    // slow down the frequency of queries to reduce the API
    // server (running on your gaming PC) resource usage
    Serial.println("Server is inactive.");
    dashboard_timer.setInterval(IDLE_DELAY);
    active = false;
  }

  if (active && game < GAME_READY) {
    // sometimes the API server may return failure, don't
    // update the dashboard in this case, just wait for the
    // temporary failure recovered, or completely inactive.
  } else if (!CLOCK_ENABLE || game == GAME_DRIVING) {
    // only update dashboard when driving
    dashboard_update(state, ntp.getEpochTime());
  } else if (lcd_get_mode() != LCD_CLOCK) {
    clock_update(ntp.getEpochTime());
  }  // otherwise let clock_tick() to update
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  pinMode(LCD_LED, OUTPUT);
  lcd_init(I2C_SDA, I2C_SCL, I2C_FREQ);
  http.setReuse(true);
  wifi_connect();

  if (CLOCK_ENABLE) {
    clock_initial_sync();
  } else {
    EtsState s{};
    dashboard_update(s, 0);
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected.");
    ntp.end();
    wifi_connect(&clock_tick);
    ntp.begin();
  }

  dashboard_timer.tick();
  clock_tick();
}
