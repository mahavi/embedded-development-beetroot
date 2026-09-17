#include <Arduino.h>
#include "configuration.h"

enum class State {
  WAIT_FOR_OPEN_CONTACT,
  WAIT_FOR_CLOSED_CONTACT,
  WAIT_FOR_OPENING_CONTACT,
  FINISHED
};

State state = State::WAIT_FOR_OPEN_CONTACT;

unsigned long openSinceMs = 0;
unsigned long commandTimeUs = 0;
unsigned long closeTimeUs = 0;
unsigned long totalCloseTimeUs = 0;
unsigned long totalOpenTimeUs = 0;
uint8_t completedSamples = 0;

volatile bool contactEdgeDetected = false;
volatile unsigned long contactEdgeTimeUs = 0;
volatile unsigned long lastAcceptedEdgeUs = 0;

void IRAM_ATTR onContactChanged() {
  const unsigned long edgeTimeUs = micros();
  if (!contactEdgeDetected &&
      edgeTimeUs - lastAcceptedEdgeUs >= Configuration::kDebounceUs) {
    contactEdgeTimeUs = edgeTimeUs;
    lastAcceptedEdgeUs = edgeTimeUs;
    contactEdgeDetected = true;
  }
}

bool getStableEdge(uint8_t contactLevel, unsigned long &edgeTimeUs) {
  noInterrupts();
  const bool detected = contactEdgeDetected;
  const unsigned long firstEdgeUs = contactEdgeTimeUs;
  if (!detected || micros() - firstEdgeUs < Configuration::kDebounceUs) {
    interrupts();
    return false;
  }

  if (digitalRead(CONTACT_PIN) != contactLevel) {
    contactEdgeDetected = false;
    interrupts();
    return false;
  }

  edgeTimeUs = firstEdgeUs;
  interrupts();
  return true;
}

void beginMeasurement() {
  noInterrupts();
  contactEdgeDetected = false;
  state = State::WAIT_FOR_CLOSED_CONTACT;
  commandTimeUs = micros();
  digitalWrite(RELAY_PIN, HIGH);
  interrupts();
}

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(CONTACT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), onContactChanged, CHANGE);
}

void loop() {
  if (state == State::FINISHED) {
    delay(10);
    return;
  }

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
    unsigned long edgeTimeUs;
    if (getStableEdge(LOW, edgeTimeUs)) {
      closeTimeUs = edgeTimeUs - commandTimeUs;
      noInterrupts();
      contactEdgeDetected = false;
      state = State::WAIT_FOR_OPENING_CONTACT;
      commandTimeUs = micros();
      digitalWrite(RELAY_PIN, LOW);
      interrupts();
    }
    return;
  }

  if (state == State::WAIT_FOR_OPENING_CONTACT) {
    unsigned long edgeTimeUs;
    if (getStableEdge(HIGH, edgeTimeUs)) {
      const unsigned long openTimeUs = edgeTimeUs - commandTimeUs;
      totalCloseTimeUs += closeTimeUs;
      totalOpenTimeUs += openTimeUs;
      completedSamples++;
      openSinceMs = 0;

      Serial.printf("Measurement %u: ON %lu us, OFF %lu us\n",
                    completedSamples, closeTimeUs, openTimeUs);

      if (completedSamples == Configuration::kSampleCount) {
        digitalWrite(RELAY_PIN, LOW);
        state = State::FINISHED;
        Serial.printf("Average after %u measurements: ON %.3f us, OFF %.3f us\n",
                      completedSamples,
                      static_cast<float>(totalCloseTimeUs) / completedSamples,
                      static_cast<float>(totalOpenTimeUs) / completedSamples);
      } else {
        state = State::WAIT_FOR_OPEN_CONTACT;
      }
    }
    return;
  }
}
