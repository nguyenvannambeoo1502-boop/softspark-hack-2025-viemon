#pragma once
#include <Arduino.h>

class HCSR04 {
public:
  HCSR04(uint8_t trigPin, uint8_t echoPin)
  : _trig(trigPin), _echo(echoPin) {}

  void begin() {
    pinMode(_trig, OUTPUT);
    pinMode(_echo, INPUT);
    digitalWrite(_trig, LOW);
    delay(50);
  }

  // One reading; returns NAN if no echo within timeout_us
  float readCmOnce(uint32_t timeout_us = 30000UL) const;

  // Median of N samples (N ≤ 15); returns NAN if all failed
  float readCmMedian(uint8_t samples = 5, uint16_t interSampleDelayMs = 50) const;

private:
  static float usToCm(unsigned long us) {
    return (us * 0.0343f) * 0.5f; // µs → cm
  }

  uint8_t _trig, _echo;
};
