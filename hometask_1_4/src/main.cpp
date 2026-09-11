#include <Arduino.h>

#define LED_PIN_1 16
#define LED_PIN_2 42
#define BUTTON_PIN_1 38
#define BUTTON_PIN_2 0

constexpr unsigned long FAST_DELAY_MS = 250;
constexpr unsigned long NORMAL_DELAY_MS = 500;
constexpr unsigned long SLOW_DELAY_MS = 1000;

unsigned long currentDelay = NORMAL_DELAY_MS;

void blink();
void updateBlinkSpeedIfNeeded();

void setup() {
    Serial.begin(115200);
    delay(1000);

    digitalWrite(LED_BUILTIN, LOW);

    pinMode(LED_PIN_1, OUTPUT);
    pinMode(LED_PIN_2, OUTPUT);

    pinMode(BUTTON_PIN_1, INPUT_PULLUP);
    pinMode(BUTTON_PIN_2, INPUT_PULLUP);
}

void loop() {
    updateBlinkSpeedIfNeeded();
    blink();
}

void updateBlinkSpeedIfNeeded() {
    bool isButton1Pressed = digitalRead(BUTTON_PIN_1) == LOW;
    bool isButton2Pressed = digitalRead(BUTTON_PIN_2) == LOW;
    if (isButton1Pressed && isButton2Pressed) {
        Serial.println("Reset Blink Speed");
        currentDelay = NORMAL_DELAY_MS;
    } else if (isButton1Pressed) {
        Serial.println("Increase Blink Speed");
        currentDelay = FAST_DELAY_MS;
    } else if (isButton2Pressed) {
        Serial.println("Decrease Blink Speed");
        currentDelay = SLOW_DELAY_MS;
    }
    delay(100);
}

void blink() {
    digitalWrite(LED_PIN_1, HIGH);
    digitalWrite(LED_PIN_2, LOW);
    delay(currentDelay);

    digitalWrite(LED_PIN_1, LOW);
    digitalWrite(LED_PIN_2, HIGH);
    delay(currentDelay);
}