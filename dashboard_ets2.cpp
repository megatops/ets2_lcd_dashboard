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

#include <TimeLib.h>
#include "dashboard.hpp"

static bool blinkShow;  // synchronize the blinks (1Hz @ 2FPS)

static void UpdateSpeed(bool force, int speed) {
  LIMIT(speed, 199);
  LAZY_UPDATE(force, speed, {
    digit.print(5, 1, speed, 3, false);
    DEBUG("Update speed: %d\n", speed);
  });
}

static void UpdateEtaDist(bool force, int etaDist) {
  LIMIT(etaDist, 9999);
  LAZY_UPDATE(force, etaDist, {
    lcd.setCursor(8, 0);
    if (etaDist >= 1000) {
      lcd.printf("%4d", etaDist);  // 8888km
    } else {
      lcd.printf("%3d ", etaDist);  // 888 km
    }
    DEBUG("Update ETA distance: %d\n", etaDist);
  });
}

static void UpdateEtaTime(bool force, int etaTime) {
  LIMIT(etaTime, 99 * 60 + 59);
  LAZY_UPDATE(force, etaTime, {
    lcd.setCursor(15, 0);
    lcd.printf("%02d:%02d", etaTime / 60, etaTime % 60);
    DEBUG("Update ETA time: %d\n", etaTime);
  });
}

static void UpdateCruise(bool force, int cruise) {
  LIMIT(cruise, 999);
  LAZY_UPDATE(force, cruise, {
    lcd.setCursor(1, 2);
    if (cruise > 0) {
      lcd.printf("%3d", cruise);
    } else {
      lcd.print("---");
    }
    DEBUG("Update cruise: %d\n", cruise);
  });
}

static void UpdateLimit(bool force, int limit, bool speeding) {
  LIMIT(limit, 999);
  LAZY_UPDATE(force, limit, {
    lcd.setCursor(16, 2);
    if (limit > 0) {
      lcd.printf("%3d", limit);
    } else {
      lcd.print("---");
    }
    DEBUG("Update speed limit: %d\n", limit);
  });

  // blink the label as speeding warning
  auto label = BLINK_IF(speeding, "Limit", "     ");
  LAZY_UPDATE(force, label, {
    lcd.setCursor(15, 1);
    lcd.print(label);
  });
}

static void UpdateFuel(bool force, bool is_ev, int fuel, int fuelDist, bool warn) {
  LIMIT(fuel, 100);
  LIMIT(fuelDist, 9999);

  LAZY_UPDATE(force, fuelDist, {
    if (fuelDist < 0) {
      if (!force) {
        break;  // no need to update
      }
      fuelDist = cached_;  // keep the old display
    }
    lcd.setCursor(16, 3);
    lcd.printf("%4d", fuelDist);
    DEBUG("Update fuel distance: %d\n", fuelDist);
  });

  int seg = round(fuel / (100.0 / 10));
  LAZY_UPDATE(force, seg, {
    char bar[] = "\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    lcd.setCursor(5, 3);
    lcd.print(bar);
    DEBUG("Update fuel: %d%%\n", fuel);
  });

  // blink the label as fuel warning
  auto label = BLINK_IF(warn, is_ev ? "Batt" : "Fuel", "    ");
  LAZY_UPDATE(force, label, {
    lcd.setCursor(0, 3);
    lcd.print(label);
  });
}

static void UpdateClock(bool force, int hour, int minute) {
  LAZY_UPDATE(force, hour, {
    lcd.setCursor(0, 0);
    lcd.printf("%02d", hour);
  });

  LAZY_UPDATE(force, minute, {
    lcd.setCursor(3, 0);
    lcd.printf("%02d", minute);
  });

  auto label = BLINK_IF(CLOCK_BLINK, ":", " ");
  LAZY_UPDATE(force, label, {
    lcd.setCursor(2, 0);
    lcd.print(label);
  });
}

static void UpdateLEDs(bool force, bool leftBlinker, bool rightBlinker,
                       bool parkingLight, bool lowBeam, bool highBeam, bool beacon,
                       bool airWarn, bool airEmergency, bool parkBrake) {
  static constexpr RgbColor GREEN{ 0, 255, 0 };
  static constexpr RgbColor BLUE{ 0, 0, 255 };
  static constexpr RgbColor ORANGE{ 255, 128, 0 };
  static constexpr RgbColor RED{ 255, 0, 0 };
  static constexpr RgbColor OFF{ 0, 0, 0 };

  bool changed = false;

  // special: blinkers
  if (leftBlinker) {
    RgbSet(LedSlot::LBLINKER, blinkShow ? GREEN : OFF);
    changed = true;
  }
  LAZY_UPDATE(force, leftBlinker, {
    if (!leftBlinker) {
      RgbSet(LedSlot::LBLINKER, OFF);
      changed = true;
    }
    DEBUG("Update leftBlinker: %d\n", leftBlinker);
  });

  if (rightBlinker) {
    RgbSet(LedSlot::RBLINKER, blinkShow ? GREEN : OFF);
    changed = true;
  }
  LAZY_UPDATE(force, rightBlinker, {
    if (!rightBlinker) {
      RgbSet(LedSlot::RBLINKER, OFF);
      changed = true;
    }
    DEBUG("Update rightBlinker: %d\n", rightBlinker);
  });

  // special: multi-state
  int airWarnLevel = airEmergency ? 2 : (airWarn ? 1 : 0);
  LAZY_UPDATE(force, airWarnLevel, {
    RgbSet(LedSlot::AIR_WARN, (airWarnLevel == 2) ? RED : ((airWarnLevel == 1) ? ORANGE : OFF));
    changed = true;
    DEBUG("Update airWarnLevel: %d\n", airWarnLevel);
  });

#define UPDATE_INDICATOR(flag, slot, color) \
  do { \
    LAZY_UPDATE(force, (flag), { \
      RgbSet((slot), (flag) ? (color) : OFF); \
      changed = true; \
      DEBUG("Update " #flag ": %d\n", (flag)); \
    }); \
  } while (0)

  // normal on/off indicators
  UPDATE_INDICATOR(parkingLight, LedSlot::PARKING_LIGHT, GREEN);
  UPDATE_INDICATOR(lowBeam, LedSlot::LOW_BEAM, GREEN);
  UPDATE_INDICATOR(highBeam, LedSlot::HIGH_BEAM, BLUE);
  UPDATE_INDICATOR(beacon, LedSlot::BEACON, ORANGE);
  UPDATE_INDICATOR(parkBrake, LedSlot::PARK_BRAKE, RED);

#undef UPDATE_INDICATOR

  if (changed) {
    rgbBar.show();
  }
}

static void DashboardInit(void) {
  RgbOFF();
  lcd.setCursor(0, 0);
  lcd.printf("%s \x7e     %s   :  ", CLOCK_ENABLE ? "     " : "Navi:", SHOW_MILE ? "mi" : "km");
  lcd.setCursor(0, 1);
  lcd.print("Cruis               ");
  lcd.setCursor(0, 2);
  lcd.print("[   ]          [   ]");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
}

void Ets2DashboardUpdate(EtsState &state, time_t time) {
  bool force = (dashMode != DASH_ETS2);  // mode change needs a full update
  dashMode = DASH_ETS2;
  blinkShow = !blinkShow;

  // no backlight when engine off, dim when headlight on
  BacklightUpdate(force, state.on ? (state.headlight ? BACKLIGHT_NIGHT : BACKLIGHT_DAY) : BACKLIGHT_OFF);
  bool freshRgb = RgbLevelUpdate(force, state.on ? (state.headlight ? RGB_LEVEL_NIGHT : RGB_LEVEL_DAY) : RGB_LEVEL_OFF);

  if (force) {
    DashboardInit();
  }

  if (CLOCK_ENABLE) {
    UpdateClock(force, CLOCK_12H ? hourFormat12(time) : hour(time), minute(time));
  }
  UpdateLimit(force, state.limit, (state.limit > 0) && (state.speed > state.limit));
  UpdateSpeed(force, state.speed);
  UpdateCruise(force, state.cruise);
  UpdateEtaDist(force, state.etaDist);
  UpdateEtaTime(force, state.etaTime);
  UpdateFuel(force, state.isEV, state.fuel, state.fuelDist, state.fuelWarn);

  // dynamic RGB brightness change needs a full update (workaround for NeoPixel bug)
  UpdateLEDs(force || freshRgb, state.leftBlinker, state.rightBlinker,
             state.parkingLight, state.parkingLight && state.headlight, state.parkingLight && state.highBeam, state.beacon,
             state.airWarn, state.airEmergency, state.parkBrake);
}
