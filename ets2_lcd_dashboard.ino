// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include "board.h"
#include "config.h"
#include "src/clock/ntp_clock.hpp"
#include "src/game/ets2.hpp"
#include "src/game/forza.hpp"
#include "src/game/game.hpp"
#include "src/utils.hpp"

static constexpr int NTP_UPDATE = 60 * 60 * 1000;  // interval to sync clock with NTP

static Display disp(I2C_SDA, I2C_SCL, I2C_FREQ, LCD_LED_PWM, RGB_LED_PIN, LCD_ADDR);

static ClockDashboard clockDash(disp);
static NtpClock ntpClock(clockDash, NTP_SERVER, (TIME_ZONE - DST * 60) * 60, NTP_UPDATE);

static TruckDashboard truckDash(disp);
static Ets2Game ets2(truckDash, ETS_API);

static RacingDashboard racingDash(disp);
static ForzaGame forza(racingDash, FORZA_PORT);

static Game *games[] = { &ets2, &forza };
static Controller controller(ntpClock, games, ARRAY_SIZE(games));

static void serviceStart() {
  ntpClock.start();
  forza.start();
}

static void serviceStop() {
  ntpClock.stop();
  forza.stop();
}

static void wifiConnect(std::function<void()> tick) {
  Serial.printf("Connecting to %s .", SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tick();
  }

  Serial.printf(" Local IP: %s\n", WiFi.localIP().toString().c_str());
  serviceStart();
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  disp.start();
  wifiConnect([] {});

  if (CLOCK_ENABLE) {
    ntpClock.initialSync();
    ntpClock.freshDisplay();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected.");
    serviceStop();
    wifiConnect([] {
      ntpClock.tick();
    });
  }

  controller.tick();
  ntpClock.tick();
}
