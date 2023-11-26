// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include <cfloat>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ets.h"

// Wi-Fi and API server
static const char *SSID = "YOUR WIFI SSID";
static const char *PASSWORD = "YOUR WIFI PASSWORD";
static const char *ETS_API = "http://YOUR_PC_IP:25555/api/ets2/telemetry";

// I2C IO pins
static const int I2C_SDA = 4;
static const int I2C_SCL = 5;

// 2004 LCD PCF8574 I2C address
static const int LCD_ADDR = 0x27;

static const int MAX_FAILURE = 3;
static const int ACTIVE_DELAY = 500;
static const int IDLE_DELAY = 2000;

static HTTPClient http;

static void wifi_connect(void) {
  Serial.printf("Connecting to %s ", SSID);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print(" Local IP: ");
  Serial.println(WiFi.localIP());
}

static bool ets_telemetry_query(EtsState *stat) {
  bool success = false;

  http.begin(ETS_API);
  int http_code = http.GET();
  if (http_code > 0) {
    String json = http.getString();
    if (ets_telemetry_parse(json, stat)) {
      success = true;
    }
  } else {
    Serial.printf("Invalid response: %d!\n", http_code);
  }
  http.end();

  return success;
}

void setup() {
  Serial.begin(921600);
  lcd_init(I2C_SDA, I2C_SCL);
  http.setReuse(true);
  wifi_connect();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected.");
    dashboard_reset();
    wifi_connect();
    return;
  }

  static bool active = true;
  static int failed = 0;

  EtsState stat = {};
  if (ets_telemetry_query(&stat)) {
    dashboard_update(&stat);
    failed = 0;
    active = true;

  } else if (active) {
    if (++failed >= MAX_FAILURE) {
      Serial.println("Server is inactive.");
      dashboard_reset();
      active = false;
    }
  }

  delay(active ? ACTIVE_DELAY : IDLE_DELAY);
}
