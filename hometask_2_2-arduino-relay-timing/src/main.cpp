#include <Arduino.h>
#include "configuration.h"

enum class State {
  WAIT_FOR_OPEN_CONTACT,
  WAIT_FOR_CLOSED_CONTACT,
  FINISHED
};

State state = State::WAIT_FOR_OPEN_CONTACT;

unsigned long openSinceMs = 0;
unsigned long commandTimeMs = 0;
unsigned long totalTimeMs = 0;
uint8_t completedSamples = 0;

volatile bool contactEdgeDetected = false;

void IRAM_ATTR onContactClosed() {
    contactEdgeDetected = true;
}

void beginMeasurement() {
  noInterrupts();
  contactEdgeDetected = false;
  commandTimeMs = millis();
  digitalWrite(RELAY_PIN, HIGH);
  interrupts();

  Serial.printf("Measurement %u: relay is ON\n", completedSamples + 1);
  state = State::WAIT_FOR_CLOSED_CONTACT;
}

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(CONTACT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), onContactClosed, FALLING);
}

void loop() {
  const unsigned long now = millis();

  if (state == State::WAIT_FOR_OPEN_CONTACT) {
    if (digitalRead(CONTACT_PIN) == HIGH) {
      if (openSinceMs == 0) {
        openSinceMs = now;
      }
      if (now - openSinceMs >= Configuration::kOpenStableMS) {
        beginMeasurement();
      }
    } else {
      openSinceMs = 0;
    }
    return;
  }

  if (state == State::WAIT_FOR_CLOSED_CONTACT) {
    noInterrupts();
    const bool edgeWasDetected = contactEdgeDetected;
    if (edgeWasDetected) {
      contactEdgeDetected = false;
    }
    interrupts();

    if (edgeWasDetected) {
      const unsigned long closeTimeMs = millis() - commandTimeMs;
      totalTimeMs += closeTimeMs;
      completedSamples++;

      Serial.printf("Close time: %lu ms\n", closeTimeMs);

      digitalWrite(RELAY_PIN, LOW);
      openSinceMs = 0;

      const float averageMs = static_cast<float>(totalTimeMs) / completedSamples;
      Serial.printf("Average time: %.2f ms\n", averageMs);
      Serial.println("--------------------------------");
      state = State::WAIT_FOR_OPEN_CONTACT;

      return;
    }

    return;
  }

  digitalWrite(RELAY_PIN, LOW);
}