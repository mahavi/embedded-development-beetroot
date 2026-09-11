#ifndef LED_H_
#define LED_H_

#include <cstdint>

enum class LedState : uint8_t {
  Off,
  On,
};

class Led {
 public:
  explicit constexpr Led(std::uint8_t pin) : pin_{pin} {}

  void init() const;
  void set(LedState state) const;

 private:
  const std::uint8_t pin_;
};

#endif  // LED_H_
