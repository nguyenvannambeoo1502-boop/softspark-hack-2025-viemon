#include "dist.h"

float HCSR04::readCmOnce(uint32_t timeout_us) const {
  // Trigger pulse: LOW 2µs → HIGH 10µs → LOW
  digitalWrite(_trig, LOW);  delayMicroseconds(2);
  digitalWrite(_trig, HIGH); delayMicroseconds(10);
  digitalWrite(_trig, LOW);

  // Echo high time (µs). Returns 0 on timeout.
  unsigned long dur = pulseIn(_echo, HIGH, timeout_us);
  if (dur == 0) return NAN;
  return usToCm(dur);
}

float HCSR04::readCmMedian(uint8_t samples, uint16_t interSampleDelayMs) const {
  if (samples < 1)  samples = 1;
  if (samples > 15) samples = 15;

  float vals[15];
  uint8_t got = 0;

  for (uint8_t i = 0; i < samples; ++i) {
    float d = readCmOnce();
    if (!isnan(d)) vals[got++] = d;
    delay(interSampleDelayMs);
  }
  if (got == 0) return NAN;

  // Insertion sort (ascending)
  for (uint8_t i = 1; i < got; ++i) {
    float key = vals[i];
    int j = i - 1;
    while (j >= 0 && vals[j] > key) { vals[j + 1] = vals[j]; j--; }
    vals[j + 1] = key;
  }

  // Median
  if (got & 1) return vals[got / 2];
  return 0.5f * (vals[got/2 - 1] + vals[got/2]);
}
