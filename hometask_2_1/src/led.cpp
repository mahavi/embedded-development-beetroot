#include <Arduino.h>
#include "led.h"

void Led::init() const {
  pinMode(pin_, OUTPUT);
  set(LedState::Off);
}

void Led::set(LedState state) const {
  digitalWrite(pin_, state == LedState::On ? HIGH : LOW);
}
