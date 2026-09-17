#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#define BAUD_RATE 115200
#define RELAY_PIN 6
#define CONTACT_PIN 13

namespace Configuration {
  static constexpr uint8_t kSampleCount = 10;
  static constexpr unsigned long kOpenStableMS = 1000;
  static constexpr unsigned long kDebounceUs = 20000;
}

#endif  // CONFIGURATION_H_
