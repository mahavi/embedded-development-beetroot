#include <Arduino.h>

constexpr uint8_t kLightSensorPin = 6;

constexpr uint16_t kAdcMaxValue = 4095;
constexpr uint32_t kReferenceVoltageMv = 3100;

uint32_t calculateMilliVolts(uint16_t rawValue);
float calculatePercentageDiff(uint32_t calculatedValue, uint32_t referenceValue);

void setup() {
    Serial.begin(115200);

    analogReadResolution(12);
}

void loop() {
    uint16_t rawValue = analogRead(kLightSensorPin);
    uint32_t calculatedMilliVolts = calculateMilliVolts(rawValue);
    uint32_t measuredMilliVolts = analogReadMilliVolts(kLightSensorPin);

    char diffText[16] = "N/A";
    if (measuredMilliVolts != 0) {
        float diff = calculatePercentageDiff(calculatedMilliVolts, measuredMilliVolts);
        snprintf(diffText, sizeof(diffText), "%.2f%%", diff);
    }

    Serial.printf(
        "raw=%u\tcalculated=%lu mV\tmeasured=%lu mV\tdiff=%s\n",
        static_cast<unsigned int>(rawValue),
        static_cast<unsigned long>(calculatedMilliVolts),
        static_cast<unsigned long>(measuredMilliVolts),
        diffText);

    delay(100);
}

uint32_t calculateMilliVolts(uint16_t rawValue) {
    return static_cast<uint32_t>(rawValue) * kReferenceVoltageMv / kAdcMaxValue;
}

float calculatePercentageDiff(uint32_t calculatedValue, uint32_t referenceValue) {
    uint32_t absoluteDifference = calculatedValue > referenceValue
        ? calculatedValue - referenceValue
        : referenceValue - calculatedValue;

    return static_cast<float>(absoluteDifference) * 100.0F / referenceValue;
}
