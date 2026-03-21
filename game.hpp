// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2023-2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#pragma once

#include <SoftwareTimer.h>
#include "config.h"
#include "display.hpp"
#include "ntp_clock.hpp"
#include "utils.hpp"

enum GameState {
  SERVER_DOWN,
  NOT_START,

  READY,
  DRIVING,  // driving the truck
};

#define BLINK_IF(cond, msg1, msg2) ((!(cond) || blinkShow_) ? (msg1) : (msg2))
#define BLINK(msg1, msg2) (blinkShow_ ? (msg1) : (msg2))

class Game {
public:
  virtual ~Game() {}
  virtual const char *name();
  virtual GameState getTelemetry();
  virtual void freshDashboard(time_t time);

public:
  const int MAX_FAILURE;   // max retries before idle
  const int ACTIVE_DELAY;  // data query interval in game (ms)

protected:
  explicit Game(Display &display, int maxFailure, int activeDelay)
    : MAX_FAILURE(maxFailure), ACTIVE_DELAY(activeDelay), disp_(display) {}

protected:
  Display &disp_;
  bool blinkShow_{};    // synchronize the blinks
  int blinkCounter_{};  // count blink duration
  bool force_{};        // force update (bypass cache)
};

class Controller {
public:
  Controller(Display &disp, NtpClock &clock, Game **games, size_t count);

  inline void tick() {
    timer_.tick();
  }

private:
  bool pollGame(Game &game);
  void pollGames();
  void realTimerCb();  // the actual timer cb
  void updateState();

private:
  Display &disp_;
  NtpClock &clock_;
  SoftwareTimer timer_;

  Game **games_{};
  size_t gameCount_{};

  Game *active_{};  // current active game
  bool driving_{};
  GameState state_{ GameState::SERVER_DOWN };
  int failed_{};

private:
  static Controller *instance_;
  static void timerCb() {
    if (instance_ != nullptr) {
      instance_->realTimerCb();
    }
  }
};
