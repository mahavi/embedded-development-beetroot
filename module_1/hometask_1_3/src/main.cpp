#include <Arduino.h>

#define DELAY_MS 300
#define LED_PIN_1 3
#define LED_PIN_2 10

void blink();

void setup() {
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
}

void loop() {
  blink();
}

void blink() {
  digitalWrite(LED_PIN_1, HIGH);
  digitalWrite(LED_PIN_2, LOW);
  delay(DELAY_MS);

  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, HIGH);
  delay(DELAY_MS);
}