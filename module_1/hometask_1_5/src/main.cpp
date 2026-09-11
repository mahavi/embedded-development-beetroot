#include <Arduino.h>

#define BUTTON_IN 16

volatile uint32_t button_counter = 0;

uint32_t last_count = 0;

void IRAM_ATTR button_isr() {
    button_counter++;
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_IN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BUTTON_IN), button_isr, FALLING);
}

void loop() {
  if (button_counter != last_count) {
    last_count = button_counter;
    Serial.printf("Button Pressed! Count: %d\n", last_count);
  }

  delay(10);
}