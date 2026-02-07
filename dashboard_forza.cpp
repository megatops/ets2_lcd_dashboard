// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// Normal:
// +----+----+----+----
// ##########        ..
// [00:00.00] 00:00.000
// BigBigBig POS:00 Big
// NumNumNum LAP:00 Num
// +----+----+----+----
//
// Performance
// ###              ###
// C00:00.00 888 POS:00
// L00:00.00 big LAP:00
// B00:00.00 num E####F

#include "dashboard.hpp"
#include "board.h"

#define FMT_STOPWATCH(time) "%02d:%02d.%02d", (time) / 100 / 60, (time) / 100 % 60, (time) % 100
#define FMT_TIMESTAMP(time) "%02d:%02d.%03d", (time) / 1000 / 60, (time) / 1000 % 60, (time) % 1000

static bool blinkShow;  // synchronize the blinks
static bool isPro;      // performance dashboard

static void UpdateSpeed(bool force, int speed) {
  LIMIT(speed, 999);
  LAZY_UPDATE(force, speed, {
    if (isPro) {
      lcd.setCursor(10, 3);
      lcd.printf("%03d", speed);
    } else {
      digit.print(0, 2, speed, 3, false);
    }
    DEBUG("Update speed: %d\n", speed);
  });
}

static void PrintN(int x, int y) {
  // use the strokes defined in LargeDigit
  lcd.setCursor(x, y);
  lcd.write(1);
  lcd.write(7);
  lcd.write(0);
  lcd.setCursor(x, y + 1);
  lcd.write(1);
  lcd.write(' ');
  lcd.write(0);
}

static void UpdateGear(bool force, int gear) {
  LIMIT(gear, 9);

  int x = isPro ? 10 : 17;
  int y = isPro ? 1 : 2;
  LAZY_UPDATE(force, gear, {
    if (gear > 0) {
      digit.print(x, y, gear, 1, false);
    } else {
      PrintN(x, y);
    }
    DEBUG("Update gear: %d\n", gear);
  });
}

static void UpdateBestTime(bool force, int bestTime) {
  bestTime /= 10;  // use the stop watch format
  LIMIT(bestTime, 599999);

  int y = isPro ? 3 : 1;
  LAZY_UPDATE(force, bestTime, {
    lcd.setCursor(1, y);
    if (bestTime > 0) {
      lcd.printf(FMT_STOPWATCH(bestTime));
    } else {
      lcd.print(isPro ? "est: N/A" : "Best:N/A");
    }
    DEBUG("Update best lap time: %d\n", bestTime);
  });
}

static void UpdateLastTime(bool force, int lastTime) {
  lastTime /= 10;  // use the stop watch format
  LIMIT(lastTime, 599999);

  LAZY_UPDATE(force, lastTime, {
    lcd.setCursor(1, 2);
    if (lastTime > 0) {
      lcd.printf(FMT_STOPWATCH(lastTime));
    } else {
      lcd.print("ast: N/A");
    }
    DEBUG("Update last lap time: %d\n", lastTime);
  });
}

static void UpdateCurrTimePro(bool force, int currTime) {
  currTime /= 10;  // use the stop watch format
  LIMIT(currTime, 599999);

  LAZY_UPDATE(force, currTime, {
    lcd.setCursor(1, 1);
    if (currTime > 0) {
      lcd.printf(FMT_STOPWATCH(currTime));
    } else {
      lcd.print("urr: N/A");
    }
    DEBUG("Update current lap time: %d\n", currTime);
  });
}

static void UpdateCurrTime(bool force, int currTime) {
  LIMIT(currTime, 5999999);
  LAZY_UPDATE(force, currTime, {
    lcd.setCursor(11, 1);
    lcd.printf(FMT_TIMESTAMP(currTime));
    DEBUG("Update current lap time: %d\n", currTime);
  });
}

static void UpdateLap(bool force, int lap) {
  LIMIT(lap, 99);

  int x = isPro ? 18 : 14;
  int y = isPro ? 2 : 3;
  LAZY_UPDATE(force, lap, {
    lcd.setCursor(x, y);
    lcd.printf("%2d", lap);
    DEBUG("Update lap: %d\n", lap);
  });
}

static void UpdatePos(bool force, int pos) {
  LIMIT(pos, 99);

  int x = isPro ? 18 : 14;
  int y = isPro ? 1 : 2;
  LAZY_UPDATE(force, pos, {
    lcd.setCursor(x, y);
    if (pos > 0) {
      lcd.printf("%2d", pos);
    } else {
      lcd.print(" -");
    }
    DEBUG("Update pos: %d\n", pos);
  });
}

static void UpdateFuel(bool force, int fuel) {
  LIMIT(fuel, 100);

  int seg = round(fuel / (100.0 / 4));
  LAZY_UPDATE(force, seg, {
    char bar[] = "\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    lcd.setCursor(15, 3);
    lcd.print(bar);
    DEBUG("Update fuel: %d%%\n", fuel);
  });
}

static void UpdateLED(bool force, float load) {
  int leds = 0;  // numbers to light up
  for (size_t i = 0; i < RGB_LED_NUM; i++) {
    if (load < RGB_LOAD_MAP[i]) {
      break;
    }
    leds = i + 1;
  }

  LAZY_UPDATE(force, leds, {
    rgbBar.clear();
    for (int i = 0; i < leds; i++) {
      RgbSet(i, RGB_COLOR_MAP[i]);
    }
    rgbBar.show();
    DEBUG("Update LED: %d\n", leds);
  });
}

static void RgbRedZone(void) {
  if (blinkShow) {
    auto &color = RGB_COLOR_MAP[RGB_LED_NUM - 1];  // same with the last LED
    for (int i = 0; i < RGB_LED_NUM; i++) {
      RgbSet(i, color);
    }
    rgbBar.show();
  } else {
    RgbOFF();
  }
}

static void UpdateRpm(bool force, int rpm, int rpmIdle, int rpmMax) {
  // calculate engine load
  rpm = (rpm < rpmIdle) ? rpmIdle : rpm;
  float load = (rpm - rpmIdle) * 100.0 / (rpmMax - rpmIdle);

  static bool inRed;
  if ((load >= FORZA_RED_ZONE) || (inRed && (load >= FORZA_SHIFT_ZONE))) {
    // in red zone, just blink the bar
    force |= !inRed;
    inRed = true;
    auto bar = BLINK("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",
                     "                    ");
    LAZY_UPDATE(force, bar, {
      lcd.setCursor(0, 0);
      lcd.print(bar);
      RgbRedZone();
    });
    return;
  }

  force |= inRed;
  inRed = false;
  UpdateLED(force, load);

  int pct = round(load * 100.0 / FORZA_SHIFT_ZONE);
  LIMIT(pct, 100);

  if (isPro) {
    // Converging rpm bar [## -> .. <- ##]
    int seg = round(pct / 10.0);
    LAZY_UPDATE(force, seg, {
      char bar[] = "                    ";
      for (int i = 0; i < seg; i++) {
        bar[i] = 0xFF;
        bar[20 - 1 - i] = 0xFF;
      }
      lcd.setCursor(0, 0);
      lcd.print(bar);
      DEBUG("Update rpm: %.2f%%\n", load);
    });

  } else {
    // Linear rpm bar: [###### ->   ..]
    int seg = round(pct / (100.0 / 20));
    LAZY_UPDATE(force, seg, {
      char bar[] = "                    ";
      for (int i = 0; i < seg; i++) {
        bar[i] = 0xFF;
      }
      lcd.setCursor(0, 0);
      lcd.print(bar);
      DEBUG("Update rpm: %.2f%%\n", load);
    });
  }
}

static void DashboardInit(void) {
  RgbOFF();

  lcd.setCursor(0, 0);
  lcd.print("                    ");

  if (isPro) {
    lcd.setCursor(0, 1);
    lcd.print("C             POS:  ");
    lcd.setCursor(0, 2);
    lcd.print("L             LAP:  ");
    lcd.setCursor(0, 3);
    lcd.print("B             E    F");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("[        ]          ");
    lcd.setCursor(0, 2);
    lcd.print("          POS:      ");
    lcd.setCursor(0, 3);
    lcd.print("          LAP:      ");
  }
}

void ForzaDashboardUpdate(ForzaState &state) {
  bool force = (dashMode != DASH_FORZA) || (isPro != state.isPro);  // mode change needs a full update
  dashMode = DASH_FORZA;
  isPro = state.isPro;
  static int blinkCounter;

  BacklightUpdate(force, BACKLIGHT_DAY);

  if (force) {
    DashboardInit();
    blinkCounter = 0;
  }

  constexpr int PERIOD = 3;
  blinkShow = (blinkCounter++ < PERIOD);
  blinkCounter %= (PERIOD * 2);

  UpdateRpm(force, state.rpm, state.rpmIdle, state.rpmMax);
  UpdateSpeed(force, state.speed);
  UpdateGear(force, state.gear);
  UpdateLap(force, state.lap);
  UpdatePos(force, state.pos);
  UpdateBestTime(force, state.bestLap);
  if (isPro) {
    UpdateCurrTimePro(force, state.currLap);
    UpdateLastTime(force, state.lastLap);
    UpdateFuel(force, state.fuel);
  } else {
    UpdateCurrTime(force, state.currLap);
  }
}
