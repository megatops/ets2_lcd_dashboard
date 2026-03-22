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

#include "game_forza.hpp"
#include "../utils.hpp"

static constexpr int FORZA_MAX_FAILURE = 5 * FORZA_PPS;
static constexpr int FORZA_ACTIVE_DELAY = 1000 / FORZA_PPS;

#define FMT_STOPWATCH(time) "%02d:%02d.%02d", (time) / 100 / 60, (time) / 100 % 60, (time) % 100
#define FMT_TIMESTAMP(time) "%02d:%02d.%03d", (time) / 1000 / 60, (time) / 1000 % 60, (time) % 1000

ForzaGame::ForzaGame(Display &display, uint16_t port)
  : Game(display, FORZA_MAX_FAILURE, FORZA_ACTIVE_DELAY) {
  port_ = port;
}

GameState ForzaGame::forzaTelemetryParse(size_t len) {
  const ForzaSledData *sled{};
  const ForzaDashData *dash{};

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

GameState ForzaGame::getTelemetry() {
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

void ForzaGame::printN(int x, int y) {
  // use the strokes defined in LargeDigit
  disp_.setCursor(x, y);
  disp_.write(1);
  disp_.write(7);
  disp_.write(0);
  disp_.setCursor(x, y + 1);
  disp_.write(1);
  disp_.write(' ');
  disp_.write(0);
}

void ForzaGame::updateSpeedGear() {
  auto speed = min(state_.speed, 999);
  LAZY_UPDATE(speed, {
    if (isPro_) {
      dispPrintf(10, 3, "%03d", speed);
    } else {
      disp_.printLarge(0, 2, speed, 3, false);
    }
    DEBUG("Update speed: %d\n", speed);
  });

  auto gear = min(state_.gear, 9);
  LAZY_UPDATE(gear, {
    int x = isPro_ ? 10 : 17;
    int y = isPro_ ? 1 : 2;
    if (gear > 0) {
      disp_.printLarge(x, y, gear, 1, false);
    } else {
      printN(x, y);
    }
    DEBUG("Update gear: %d\n", gear);
  });
}

void ForzaGame::updateLapTime() {
  LAZY_UPDATE(state_.bestLap, {
    auto bestTime = min(static_cast<int>(round(state_.bestLap / 10.0)), 599999);  // use the stop watch format
    int y = isPro_ ? 3 : 1;
    disp_.setCursor(1, y);
    if (bestTime > 0) {
      disp_.printf(FMT_STOPWATCH(bestTime));
    } else {
      disp_.print(isPro_ ? "est: N/A" : "Best:N/A");
    }
    DEBUG("Update best lap time: %d\n", bestTime);
  });

  if (!isPro_) {
    return;
  }

  LAZY_UPDATE(state_.lastLap, {
    auto lastTime = min(static_cast<int>(round(state_.lastLap / 10.0)), 599999);  // use the stop watch format
    disp_.setCursor(1, 2);
    if (lastTime > 0) {
      disp_.printf(FMT_STOPWATCH(lastTime));
    } else {
      disp_.print("ast: N/A");
    }
    DEBUG("Update last lap time: %d\n", lastTime);
  });
}

void ForzaGame::updateCurrTime() {
  if (isPro_) {
    auto currTime = min(static_cast<int>(round(state_.currLap / 10.0)), 599999);  // use the stop watch format
    LAZY_UPDATE(currTime, {
      disp_.setCursor(1, 1);
      if (currTime > 0) {
        disp_.printf(FMT_STOPWATCH(currTime));
      } else {
        disp_.print("urr: N/A");
      }
      DEBUG("Update current lap time: %d\n", currTime);
    });

  } else {
    auto currTime = min(state_.currLap, 5999999);
    LAZY_UPDATE(currTime, {
      dispPrintf(11, 1, FMT_TIMESTAMP(currTime));
      DEBUG("Update current lap time: %d\n", currTime);
    });
  }
}

void ForzaGame::updateLapPos() {
  auto lap = min(state_.lap, 99);
  LAZY_UPDATE(lap, {
    int x = isPro_ ? 18 : 14;
    int y = isPro_ ? 2 : 3;
    dispPrintf(x, y, "%2d", lap);
    DEBUG("Update lap: %d\n", lap);
  });

  auto pos = min(state_.pos, 99);
  LAZY_UPDATE(pos, {
    int x = isPro_ ? 18 : 14;
    int y = isPro_ ? 1 : 2;
    disp_.setCursor(x, y);
    if (pos > 0) {
      disp_.printf("%2d", pos);
    } else {
      disp_.print(" -");
    }
    DEBUG("Update pos: %d\n", pos);
  });
}

void ForzaGame::updateFuel() {
  if (!isPro_) {
    return;
  }

  auto fuel = min(state_.fuel, 100);
  int seg = round(fuel / (100.0 / 4));
  LAZY_UPDATE(seg, {
    char bar[] = "\xA5\xA5\xA5\xA5";
    for (int i = 0; i < seg; i++) {
      bar[i] = 0xFF;
    }
    dispPrint(15, 3, bar);
    DEBUG("Update fuel: %d%%\n", fuel);
  });
}

void ForzaGame::ledProgress(float load) {
  int leds = 0;  // numbers to light up
  for (size_t i = 0; i < ARRAY_SIZE(RGB_LOAD_MAP); i++) {
    if (load < RGB_LOAD_MAP[i]) {
      break;
    }
    leds = i + 1;
  }

  LAZY_UPDATE(leds, {
    disp_.ledClear();
    for (int i = 0; i < leds; i++) {
      disp_.ledSet(i, RGB_COLOR_MAP[i]);
    }
    disp_.ledShow();
    DEBUG("Update LED: %d\n", leds);
  });
}

void ForzaGame::ledRedZone() {
  if (blinkShow_) {
    disp_.ledFill(RGB_COLOR_MAP[ARRAY_SIZE(RGB_COLOR_MAP) - 1]);  // same with the last LED
    disp_.ledShow();
  } else {
    disp_.ledOFF();
  }
}

void ForzaGame::updateRpm() {
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
      dispPrint(0, 0, bar);
      ledRedZone();
    });
    return;
  }

  force_ |= inRed_;
  inRed_ = false;
  ledProgress(load);

  int pct = min(static_cast<int>(round(load * 100.0 / FORZA_SHIFT_ZONE)), 100);
  if (isPro_) {
    // Converging rpm bar [## -> .. <- ##]
    int seg = round(pct / 10.0);
    LAZY_UPDATE(seg, {
      char bar[] = "                    ";
      for (int i = 0; i < seg; i++) {
        bar[i] = 0xFF;
        bar[20 - 1 - i] = 0xFF;
      }
      dispPrint(0, 0, bar);
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
      dispPrint(0, 0, bar);
      DEBUG("Update rpm: %.2f%%\n", load);
    });
  }
}

void ForzaGame::dashboardInit() {
  if (!force_) {
    return;
  }
  disp_.ledOFF();
  dispPrint(0, 0, "                    ");
  dispPrint(0, 1, isPro_ ? "C             POS:  " : "[        ]          ");
  dispPrint(0, 2, isPro_ ? "L             LAP:  " : "          POS:      ");
  dispPrint(0, 3, isPro_ ? "B             E    F" : "          LAP:      ");
}

void ForzaGame::freshDisplay([[maybe_unused]] time_t time) {
  // owner/style change needs a full update
  force_ = disp_.setOwner(this) || (isPro_ != state_.isPro);
  isPro_ = state_.isPro;

  constexpr int PERIOD = 3;
  blinkShow_ = (frameCnt_++ < PERIOD);
  frameCnt_ %= (PERIOD * 2);

  disp_.backlightUpdate(force_, BACKLIGHT_DAY);
  disp_.ledBrightnesslUpdate(force_, RGB_LEVEL_DAY);

  dashboardInit();
  updateRpm();
  updateSpeedGear();
  updateCurrTime();
  updateLapTime();
  updateLapPos();
  updateFuel();
}
