#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <cstdint>

class Configuration final {
 public:
  static constexpr std::uint32_t kBaudRate = 115200;
  static constexpr std::uint8_t kLedPin = 16;
  static constexpr std::uint32_t kBlinkIntervalMs = 1000;
  static constexpr std::uint32_t kLoopReportInterval = 1000;

 private:
  Configuration() = delete;
};

#endif  // CONFIGURATION_H_
