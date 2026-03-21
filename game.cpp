// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#include "game.hpp"
#include "utils.hpp"

static constexpr int IDLE_DELAY = 5000;  // API query interval when idle

Controller *Controller::instance_ = nullptr;

Controller::Controller(Display &disp, NtpClock &clock, Game **games, size_t count)
  : disp_(disp), clock_(clock), timer_(SoftwareTimer(IDLE_DELAY, timerCb)) {
  games_ = games;
  gameCount_ = count;
  instance_ = this;
}

void Controller::updateState() {
  if (active_ != nullptr) {
    switch (state_) {
      case GameState::DRIVING:
        driving_ = true;
        break;
      case GameState::READY:  // game paused, quit driving mode
        driving_ = false;
        break;
      default:
        // for temporary failure, preserve the last mode
        // until it recovered, or completely inactive.
        break;
    }
  } else {
    driving_ = false;  // inactive
  }
}

bool Controller::pollGame(Game &game) {
  state_ = game.getTelemetry();
  if (state_ >= GameState::READY) {
    if (active_ != &game) {
      // the game become active, speed up polling for faster responses
      Serial.printf("%s become active.\n", game.name());
      active_ = &game;
      timer_.setInterval(game.ACTIVE_DELAY);
    }
    failed_ = 0;
    return true;  // stop polling other game
  }

  // in probe, just try next game
  if (active_ != &game) {
    return false;
  }

  // failed in game, check the failure count
  if (++failed_ < game.MAX_FAILURE) {
    return true;  // temporal failure, still in this game
  }
  Serial.printf("%s is inactive.\n", game.name());
  active_ = nullptr;
  timer_.setInterval(IDLE_DELAY);
  return false;
}

void Controller::pollGames() {
  // poll current game
  if ((active_ != nullptr) && pollGame(*active_)) {
    return;
  }

  // probe games
  for (size_t i = 0; i < gameCount_; i++) {
    if (pollGame(*games_[i])) {
      return;
    }
  }
}

void Controller::realTimerCb() {
  pollGames();
  updateState();

  if (driving_) {
    active_->freshDashboard(clock_.time());
  } else if (!disp_.isOwnedBy(&clock_)) {
    clock_.freshDashboard();  // need to switch to clock mode (not driving, or inactive)
  }
  // otherwise let clock_tick() to update the clock disp
}
