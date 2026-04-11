// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// +----+----+----+----
// 00:00 >8888 km 00:00
// CruisBigBigBig Limit
// [888]NumNumNum [888]
// Fuel ####...... 8888
// +----+----+----+----

#include "truck.hpp"
#include <TimeLib.h>
#include "../utils.hpp"

void TruckDashboard::updateEta(const TruckState *state) {
  auto etaDist = min(state->etaDist, 9999);
  LAZY_UPDATE(etaDist, {
    disp_.setCursor(8, 0);
    if (etaDist >= 1000) {
      disp_.printf("%4d", etaDist);  // 8888km
    } else {
      disp_.printf("%3d ", etaDist);  // 888 km
    }
    DEBUG("Update ETA distance: %d\n", etaDist);
  });

  auto etaTime = min(state->etaTime, 99 * 60 + 59);
  LAZY_UPDATE(etaTime, {
    dispPrintf(15, 0, "%02d:%02d", etaTime / 60, etaTime % 60);
    DEBUG("Update ETA time: %d\n", etaTime);
  });
}

void TruckDashboard::updateSpeed(const TruckState *state) {
  auto speed = min(state->speed, 199);
  LAZY_UPDATE(speed, {
    disp_.printLarge(5, 1, speed, 3, false);
    DEBUG("Update speed: %d\n", speed);
  });

  auto cruise = min(state->cruise, 999);
  LAZY_UPDATE(cruise, {
    disp_.setCursor(1, 2);
    if (cruise > 0) {
      disp_.printf("%3d", cruise);
    } else {
      disp_.print("---");
    }
    DEBUG("Update cruise: %d\n", cruise);
  });

  auto limit = min(state->limit, 999);
  LAZY_UPDATE(limit, {
    disp_.setCursor(16, 2);
    if (limit > 0) {
      disp_.printf("%3d", limit);
    } else {
      disp_.print("---");
    }
    DEBUG("Update speed limit: %d\n", limit);
  });

  // blink the label as speeding warning
  bool speeding = (state->limit > 0) && (state->speed > state->limit);
  auto label = BLINK_IF(speeding, "Limit", "     ");
  LAZY_UPDATE(label, dispPrint(15, 1, label));
}

void TruckDashboard::updateFuel(const TruckState *state) {
  auto fuelDist = min(state->fuelDist, 9999);
  LAZY_UPDATE(fuelDist, {
    dispPrintf(16, 3, "%4d", fuelDist);
    DEBUG("Update fuel distance: %d\n", fuelDist);
  });

  auto fuel = min(state->fuel, 100);
  int seg = round(fuel / (100.0 / 10));
  LAZY_UPDATE(seg, {
    char bar[] = "\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    dispPrint(5, 3, bar);
    DEBUG("Update fuel: %d%%\n", fuel);
  });

  // blink the label as fuel warning
  auto label = BLINK_IF(state->fuelWarn, state->isEV ? "Batt" : "Fuel", "    ");
  LAZY_UPDATE(label, dispPrint(0, 3, label));
}

void TruckDashboard::updateClock(time_t time) {
  int h = CLOCK_12H ? hourFormat12(time) : hour(time),
      m = minute(time);
  LAZY_UPDATE(h, dispPrintf(0, 0, "%02d", h));
  LAZY_UPDATE(m, dispPrintf(3, 0, "%02d", m));

  auto label = BLINK_IF(CLOCK_BLINK, ":", " ");
  LAZY_UPDATE(label, dispPrint(2, 0, label));
}

void TruckDashboard::updateLEDs(const TruckState *state) {
  bool changed = false;  // need to fresh LED bar

  // special: blinkers
  if (state->leftBlinker) {
    disp_.ledSet(LedSlot::LBLINKER, blinkShow_ ? LED_INFO : LED_OFF);
    changed = true;
  }
  LAZY_UPDATE(state->leftBlinker, {
    if (!state->leftBlinker) {
      disp_.ledSet(LedSlot::LBLINKER, LED_OFF);
      changed = true;
    }
    DEBUG("Update leftBlinker: %d\n", state->leftBlinker);
  });

  if (state->rightBlinker) {
    disp_.ledSet(LedSlot::RBLINKER, blinkShow_ ? LED_INFO : LED_OFF);
    changed = true;
  }
  LAZY_UPDATE(state->rightBlinker, {
    if (!state->rightBlinker) {
      disp_.ledSet(LedSlot::RBLINKER, LED_OFF);
      changed = true;
    }
    DEBUG("Update rightBlinker: %d\n", state->rightBlinker);
  });

  // special: multi-state
  int airWarnLevel = state->airEmerg ? 2 : (state->airWarn ? 1 : 0);
  LAZY_UPDATE(airWarnLevel, {
    disp_.ledSet(LedSlot::AIR_WARN, (airWarnLevel == 2) ? LED_ALERT : ((airWarnLevel == 1) ? LED_WARN : LED_OFF));
    changed = true;
    DEBUG("Update airWarnLevel: %d\n", airWarnLevel);
  });

#define UPDATE_INDICATOR(flag, slot, color) \
  do { \
    LAZY_UPDATE((flag), { \
      disp_.ledSet((slot), (flag) ? (color) : LED_OFF); \
      changed = true; \
      DEBUG("Update " #flag ": %d\n", (flag)); \
    }); \
  } while (0)

  // normal on/off indicators
  bool lowBeam = state->parkingLight && state->headlight,
       highBeam = state->parkingLight && state->highBeam;
  UPDATE_INDICATOR(lowBeam, LedSlot::LOW_BEAM, LED_INFO);
  UPDATE_INDICATOR(highBeam, LedSlot::HIGH_BEAM, LED_NOTICE);
  UPDATE_INDICATOR(state->beacon, LedSlot::BEACON, LED_WARN);
  UPDATE_INDICATOR(state->brake, LedSlot::BRAKE, LED_INFO);
  UPDATE_INDICATOR(state->parkBrake, LedSlot::PARK_BRAKE, LED_ALERT);

#undef UPDATE_INDICATOR

  if (changed) {
    disp_.ledShow();
  }
}

void TruckDashboard::dashboardInit() {
  if (!force_) {
    return;
  }
  disp_.ledOFF();
  dispPrintf(0, 0, "%s \x7e     %s   :  ", CLOCK_ENABLE ? "     " : "Navi:", SHOW_MILE ? "mi" : "km");
  dispPrint(0, 1, "Cruis               ");
  dispPrint(0, 2, "[   ]          [   ]");
  dispPrint(0, 3, "                    ");
}

void TruckDashboard::fresh(void *owner, time_t time, const TruckState *state) {
  // owner change needs a full update
  force_ = disp_.setOwner(owner);
  blinkShow_ = !blinkShow_;  // 1Hz @ 2FPS

  // no backlight when engine off, dim when headlight on
  int backLight = state->headlight ? BACKLIGHT_NIGHT : BACKLIGHT_DAY;
  disp_.backlightUpdate(force_, state->on ? backLight : BACKLIGHT_OFF);
  int ledLight = state->headlight ? RGB_LEVEL_NIGHT : RGB_LEVEL_DAY;
  bool freshLed = disp_.ledBrightnesslUpdate(force_, state->on ? ledLight : RGB_LEVEL_OFF);

  dashboardInit();
  if (CLOCK_ENABLE) {
    updateClock(time);
  }
  updateSpeed(state);
  updateEta(state);
  updateFuel(state);

  // dynamic RGB brightness change needs a full update (workaround for NeoPixel limitation)
  force_ |= freshLed;
  updateLEDs(state);
}
