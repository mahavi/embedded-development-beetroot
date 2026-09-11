#include "blinker.h"

void Blinker::init(std::uint32_t nowMs) {
  state_ = LedState::Off;
  lastToggleMs_ = nowMs;
  led_.init();
}

void Blinker::update(std::uint32_t nowMs) {
  if (nowMs - lastToggleMs_ < intervalMs_) {
    return;
  }

  lastToggleMs_ = nowMs;
  state_ = toggle(state_);
  led_.set(state_);
}
