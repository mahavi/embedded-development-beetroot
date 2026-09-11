#include <Arduino.h>
#include <cstdint>

#include "blinker.h"
#include "configuration.h"

Blinker& blinker() {
  static Blinker instance{Configuration::kLedPin,
                          Configuration::kBlinkIntervalMs};
  return instance;
}

void setup() {
  Serial.begin(Configuration::kBaudRate);
  blinker().init(millis());
}

void loop() {
  static std::uint32_t iterationCount = 0;

  const std::uint32_t startedAtUs = micros();
  blinker().update(millis());
  const std::uint32_t executionTimeUs = micros() - startedAtUs;

  ++iterationCount;
  if (iterationCount < Configuration::kLoopReportInterval) {
    return;
  }

  iterationCount = 0;
  Serial.print("Loop execution time: ");
  Serial.print(executionTimeUs);
  Serial.println(" us");
}
