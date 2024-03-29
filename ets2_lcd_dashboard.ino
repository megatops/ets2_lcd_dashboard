// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023-2024 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <SoftwareTimer.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "ets.h"

// Wi-Fi and API server
static const char *SSID = "YOUR WIFI SSID";
static const char *PASSWORD = "YOUR WIFI PASSWORD";
static const char *ETS_API = "http://YOUR_PC_IP:25555/api/ets2/telemetry";

// Clock settings
static const bool CLOCK_ENABLE = true;           // false to disable the clock feature
static const char *NTP_SERVER = "pool.ntp.org";  // "ntp.ntsc.ac.cn" for mainland China
static const int TIME_ZONE = 8 * 60;             // local time zone in minutes
static const bool DST = false;                   // daylight saving time

// Dashboard configurations
const bool CLOCK_BLINK = false;   // blink the ":" mark in dashboard clock
const bool WARN_SPEEDING = true;  // blink the speed limit when speeding
const bool SHOW_MILE = false;     // display with mile instead of km

// Backlight levels
static const int BACKLIGHT_MAX = 255;
static const int BACKLIGHT_OFF = 0;

// Backlight levels for dashboard
static const int BACKLIGHT_DAY = 200;    // for daytime, brighter
static const int BACKLIGHT_NIGHT = 128;  // when headlight on, dimmer

// Backlight levels for clock
static const int BACKLIGHT_CLOCK = 128;     // normal time
static const int BACKLIGHT_CLOCK_DIM = 24;  // night light

// Hours to dim the clock (1:00am ~ 5:59am by default)
static const bool CLOCK_DIM_HOURS[24] = {
  [0] = false,
  [1] = true,
  [2] = true,
  [3] = true,
  [4] = true,
  [5] = true,
  [6] = false,
  [7] = false,
  [8] = false,
  [9] = false,
  [10] = false,
  [11] = false,
  [12] = false,
  [13] = false,
  [14] = false,
  [15] = false,
  [16] = false,
  [17] = false,
  [18] = false,
  [19] = false,
  [20] = false,
  [21] = false,
  [22] = false,
  [23] = false,
};

// 2004 LCD PCF8574 I2C address
static const int LCD_ADDR = 0x27;

// I2C IO pins
static const int I2C_SDA = 4;
static const int I2C_SCL = 5;

// LCD backlight control pin (PWM)
static const int LCD_LED = 6;

// Bus frequencies
static const int SERIAL_BAUDRATE = 921600;
static const uint32_t I2C_FREQ = 400000;  // 400kHz fast mode

// Server timeouts
static const int MAX_FAILURE = 10;             // max retries before idle
static const int ACTIVE_DELAY = 500;           // API query interval in game
static const int IDLE_DELAY = 5000;            // API query interval when idle
static const int NTP_UPDATE = 60 * 60 * 1000;  // interval to sync clock with NTP
static const int HTTP_CONN_TIMEOUT = 200;      // timeout for connect
static const int HTTP_READ_TIMEOUT = 500;      // timeout for TCP read

static HTTPClient http;
static WiFiUDP ntpUDP;
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
    Serial.printf("NTP sync success: %s.\n", ntp.getFormattedTime());
  }

  // the time may have been updated by NTP, fetch it again
  last_update = ntp.getEpochTime();
  clock_update(last_update);
}

static GameState ets_telemetry_query(EtsState *state) {
  GameState game = GAME_SERVER_DOWN;

  http.begin(ETS_API);
  http.setConnectTimeout(HTTP_CONN_TIMEOUT);
  http.setTimeout(HTTP_READ_TIMEOUT);
  int http_code = http.GET();
  if (http_code > 0) {
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
  EtsState state = {};
  GameState game = ets_telemetry_query(&state);

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
    dashboard_update(&state, ntp.getEpochTime());
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
    EtsState s = {};
    dashboard_update(&s, 0);
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
