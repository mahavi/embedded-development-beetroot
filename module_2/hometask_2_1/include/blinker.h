#ifndef BLINKER_H_
#define BLINKER_H_

#include <cstdint>

#include "led.h"

class Blinker {
 public:
  constexpr Blinker(std::uint8_t ledPin, std::uint32_t intervalMs)
      : led_{ledPin}, intervalMs_{intervalMs} {}

  void init(std::uint32_t nowMs);
  void update(std::uint32_t nowMs);

 private:
  static constexpr LedState toggle(LedState state) {
    return state == LedState::Off ? LedState::On : LedState::Off;
  }

  Led led_;
  const std::uint32_t intervalMs_;
  LedState state_{LedState::Off};
  std::uint32_t lastToggleMs_{0};
};

#endif  // BLINKER_H_
