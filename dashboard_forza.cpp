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

#include "dashboard_forza.hpp"

#define FMT_STOPWATCH(time) "%02d:%02d.%02d", (time) / 100 / 60, (time) / 100 % 60, (time) % 100
#define FMT_TIMESTAMP(time) "%02d:%02d.%03d", (time) / 1000 / 60, (time) / 1000 % 60, (time) % 1000

ForzaDashboard::ForzaDashboard(Display &display, WiFiUDP &udp)
  : Dashboard(display), udp_(udp) {}

GameState ForzaDashboard::forzaTelemetryParse(size_t len) {
  const ForzaSledData *sled = nullptr;
  const ForzaDashData *dash = nullptr;

  switch (len) {
    case sizeof(ForzaMotorsportPktV1):
      sled = &pkt_.motosportV1.sled;
      dash = &pkt_.motosportV1.dash;
      break;

    case sizeof(ForzaMotorsportPktV2):
      sled = &pkt_.motosportV2.sled;
      dash = &pkt_.motosportV2.dash;
      break;

    case sizeof(ForzaHorizonPkt):
      sled = &pkt_.horizon.sled;
      dash = &pkt_.horizon.dash;
      break;

    default:
      return GameState::SERVER_DOWN;
  }

  if (CLOCK_ENABLE && (sled->IsRaceOn == 0)) {
    return GameState::READY;
  }

  // never touch state_ until we can confirm we will success, so we can display
  // previous state on temporary failure.
  state_ = {
    .speed = static_cast<int>(abs(round(KmConv((double)dash->Speed * 3600 / 1000)))),
    .gear = dash->Gear,
    .rpmIdle = static_cast<int>(round(sled->EngineIdleRpm)),
    .rpm = static_cast<int>(round(sled->CurrentEngineRpm)),
    .rpmMax = static_cast<int>(round(sled->EngineMaxRpm)),
    .fuel = static_cast<int>(dash->Fuel * 100.0),
    .isPro = sled->CarClass >= FORZA_PRO_CLASS,

    .lap = dash->LapNumber + 1,
    .pos = dash->RacePosition,
    .bestLap = static_cast<int>(dash->BestLap * 1000),
    .lastLap = static_cast<int>(dash->LastLap * 1000),
    .currLap = static_cast<int>(dash->CurrentLap * 1000),
  };
  return GameState::DRIVING;
}

GameState ForzaDashboard::getGameData() {
  int len = udp_.parsePacket();
  if (len <= 0) {
    return GameState::SERVER_DOWN;  // no packet
  }

  if (!IsForzaPacket(len)) {
    Serial.printf("Invalid Forza packet of size %d from %s\n", len, udp_.remoteIP().toString().c_str());
    return GameState::SERVER_DOWN;
  }

  int n = udp_.read(pkt_.bytes, sizeof(pkt_));
  if (n != len) {
    Serial.printf("Failed to read Forza packet: %d of %d\n", n, len);
    return GameState::SERVER_DOWN;
  }

  return forzaTelemetryParse(n);
}

void ForzaDashboard::updateSpeed() {
  auto speed = state_.speed;
  LIMIT(speed, 999);

  LAZY_UPDATE(speed, {
    if (isPro_) {
      display_.setCursor(10, 3);
      display_.printf("%03d", speed);
    } else {
      display_.printLarge(0, 2, speed, 3, false);
    }
    DEBUG("Update speed: %d\n", speed);
  });
}

void ForzaDashboard::printN(int x, int y) {
  // use the strokes defined in LargeDigit
  display_.setCursor(x, y);
  display_.write(1);
  display_.write(7);
  display_.write(0);
  display_.setCursor(x, y + 1);
  display_.write(1);
  display_.write(' ');
  display_.write(0);
}

void ForzaDashboard::updateGear() {
  auto gear = state_.gear;
  LIMIT(gear, 9);

  int x = isPro_ ? 10 : 17;
  int y = isPro_ ? 1 : 2;
  LAZY_UPDATE(gear, {
    if (gear > 0) {
      display_.printLarge(x, y, gear, 1, false);
    } else {
      printN(x, y);
    }
    DEBUG("Update gear: %d\n", gear);
  });
}

void ForzaDashboard::updateBestTime() {
  auto bestTime = state_.bestLap;
  bestTime /= 10;  // use the stop watch format
  LIMIT(bestTime, 599999);

  int y = isPro_ ? 3 : 1;
  LAZY_UPDATE(bestTime, {
    display_.setCursor(1, y);
    if (bestTime > 0) {
      display_.printf(FMT_STOPWATCH(bestTime));
    } else {
      display_.print(isPro_ ? "est: N/A" : "Best:N/A");
    }
    DEBUG("Update best lap time: %d\n", bestTime);
  });
}

void ForzaDashboard::updateLastTime() {
  auto lastTime = state_.lastLap;
  lastTime /= 10;  // use the stop watch format
  LIMIT(lastTime, 599999);

  LAZY_UPDATE(lastTime, {
    display_.setCursor(1, 2);
    if (lastTime > 0) {
      display_.printf(FMT_STOPWATCH(lastTime));
    } else {
      display_.print("ast: N/A");
    }
    DEBUG("Update last lap time: %d\n", lastTime);
  });
}

void ForzaDashboard::updateCurrTime() {
  auto currTime = state_.currLap;

  if (isPro_) {
    currTime = round(currTime / 10.0);  // use the stop watch format
    LIMIT(currTime, 599999);

    LAZY_UPDATE(currTime, {
      display_.setCursor(1, 1);
      if (currTime > 0) {
        display_.printf(FMT_STOPWATCH(currTime));
      } else {
        display_.print("urr: N/A");
      }
      DEBUG("Update current lap time: %d\n", currTime);
    });

  } else {
    LIMIT(currTime, 5999999);
    LAZY_UPDATE(currTime, {
      display_.setCursor(11, 1);
      display_.printf(FMT_TIMESTAMP(currTime));
      DEBUG("Update current lap time: %d\n", currTime);
    });
  }
}

void ForzaDashboard::updateLap() {
  auto lap = state_.lap;
  LIMIT(lap, 99);

  int x = isPro_ ? 18 : 14;
  int y = isPro_ ? 2 : 3;
  LAZY_UPDATE(lap, {
    display_.setCursor(x, y);
    display_.printf("%2d", lap);
    DEBUG("Update lap: %d\n", lap);
  });
}

void ForzaDashboard::updatePos() {
  auto pos = state_.pos;
  LIMIT(pos, 99);

  int x = isPro_ ? 18 : 14;
  int y = isPro_ ? 1 : 2;
  LAZY_UPDATE(pos, {
    display_.setCursor(x, y);
    if (pos > 0) {
      display_.printf("%2d", pos);
    } else {
      display_.print(" -");
    }
    DEBUG("Update pos: %d\n", pos);
  });
}

void ForzaDashboard::updateFuel() {
  auto fuel = state_.fuel;
  LIMIT(fuel, 100);

  int seg = round(fuel / (100.0 / 4));
  LAZY_UPDATE(seg, {
    char bar[] = "\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    display_.setCursor(15, 3);
    display_.print(bar);
    DEBUG("Update fuel: %d%%\n", fuel);
  });
}

void ForzaDashboard::updateLED(float load) {
  int leds = 0;  // numbers to light up
  for (size_t i = 0; i < ARRAY_SIZE(RGB_LOAD_MAP); i++) {
    if (load < RGB_LOAD_MAP[i]) {
      break;
    }
    leds = i + 1;
  }

  LAZY_UPDATE(leds, {
    display_.ledClear();
    for (int i = 0; i < leds; i++) {
      display_.ledSet(i, RGB_COLOR_MAP[i]);
    }
    display_.ledShow();
    DEBUG("Update LED: %d\n", leds);
  });
}

void ForzaDashboard::ledRedZone() {
  if (blinkShow_) {
    display_.ledFill(RGB_COLOR_MAP[ARRAY_SIZE(RGB_COLOR_MAP) - 1]);  // same with the last LED
    display_.ledShow();
  } else {
    display_.ledOFF();
  }
}

void ForzaDashboard::updateRpm() {
  auto rpm = state_.rpm, rpmIdle = state_.rpmIdle, rpmMax = state_.rpmMax;

  // calculate engine load
  rpm = (rpm < rpmIdle) ? rpmIdle : rpm;
  float load = (rpm - rpmIdle) * 100.0 / (rpmMax - rpmIdle);

  if ((load >= FORZA_RED_ZONE) || (inRed_ && (load >= FORZA_SHIFT_ZONE))) {
    // in red zone, just blink the bar
    force_ |= !inRed_;
    inRed_ = true;
    auto bar = BLINK("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",
                     "                    ");
    LAZY_UPDATE(bar, {
      display_.setCursor(0, 0);
      display_.print(bar);
      ledRedZone();
    });
    return;
  }

  force_ |= inRed_;
  inRed_ = false;
  updateLED(load);

  int pct = round(load * 100.0 / FORZA_SHIFT_ZONE);
  LIMIT(pct, 100);

  if (isPro_) {
    // Converging rpm bar [## -> .. <- ##]
    int seg = round(pct / 10.0);
    LAZY_UPDATE(seg, {
      char bar[] = "                    ";
      for (int i = 0; i < seg; i++) {
        bar[i] = 0xFF;
        bar[20 - 1 - i] = 0xFF;
      }
      display_.setCursor(0, 0);
      display_.print(bar);
      DEBUG("Update rpm: %.2f%%\n", load);
    });

  } else {
    // Linear rpm bar: [###### ->   ..]
    int seg = round(pct / (100.0 / 20));
    LAZY_UPDATE(seg, {
      char bar[] = "                    ";
      for (int i = 0; i < seg; i++) {
        bar[i] = 0xFF;
      }
      display_.setCursor(0, 0);
      display_.print(bar);
      DEBUG("Update rpm: %.2f%%\n", load);
    });
  }
}

void ForzaDashboard::dashboardInit() {
  if (!force_) {
    return;
  }
  display_.ledOFF();
  display_.setCursor(0, 0);
  display_.print("                    ");
  display_.setCursor(0, 1);
  display_.print(isPro_ ? "C             POS:  " : "[        ]          ");
  display_.setCursor(0, 2);
  display_.print(isPro_ ? "L             LAP:  " : "          POS:      ");
  display_.setCursor(0, 3);
  display_.print(isPro_ ? "B             E    F" : "          LAP:      ");
}

void ForzaDashboard::freshDisplay([[maybe_unused]] time_t time) {
  // mode/style change needs a full update
  force_ = (display_.mode != DisplayMode::FORZA) || (isPro_ != state_.isPro);
  display_.mode = DisplayMode::FORZA;
  isPro_ = state_.isPro;

  constexpr int PERIOD = 3;
  blinkShow_ = (blinkCounter_++ < PERIOD);
  blinkCounter_ %= (PERIOD * 2);

  display_.backlightUpdate(force_, BACKLIGHT_DAY);
  display_.ledBrightnesslUpdate(force_, RGB_LEVEL_DAY);

  dashboardInit();
  updateRpm();
  updateSpeed();
  updateGear();
  updateLap();
  updatePos();
  updateCurrTime();
  updateBestTime();
  if (isPro_) {
    updateLastTime();
    updateFuel();
  }
}
