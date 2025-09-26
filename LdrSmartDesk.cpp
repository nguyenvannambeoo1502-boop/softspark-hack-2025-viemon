#include "LdrSmartDesk.h"

LdrSmartDesk::LdrSmartDesk(uint8_t analogPin, float vRef, float rFixed, float alpha)
: _pin(analogPin),
  _vRef(vRef),
  _rFixed(rFixed),
  _alpha(constrain(alpha, 0.0f, 1.0f)),
  _raw(0),
  _smooth(0.0f),
  _analogLow(650),      // good starting point for indoor desks with 10k divider
  _analogHigh(350),
  _hyst(20),
  _state(COMFORT)
{}

void LdrSmartDesk::begin() {
  _raw = analogRead(_pin);
  _smooth = (float)_raw;
}

void LdrSmartDesk::autoCalibrate(uint32_t ms) {
  int vMin = 1023, vMax = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    int v = analogRead(_pin);
    if (v < vMin) vMin = v;
    if (v > vMax) vMax = v;
    delay(10);
  }
  // Build a comfort band in the middle ~40% of the observed range.
  int span = vMax - vMin;
  if (span < 10) span = 10; // guard
  int lower = vMin + (int)(0.30f * span);  // ~30%
  int upper = vMin + (int)(0.70f * span);  // ~70%

  // With wiring 5V—LDR—A0—Rfixed—GND : lower = brighter side, upper = darker side
  _analogHigh = lower;   // ≤ this ⇒ TOO_BRIGHT
  _analogLow  = upper;   // ≥ this ⇒ TOO_DARK
  _hyst = max(5, span / 25); // ~4% band as hysteresis

  // Reseed smoother
  _raw = analogRead(_pin);
  _smooth = (float)_raw;
  _state = COMFORT;
}

void LdrSmartDesk::setThresholds(int analogLow, int analogHigh, int hyst) {
  _analogLow  = constrain(analogLow, 0, 1023);
  _analogHigh = constrain(analogHigh, 0, 1023);
  _hyst       = max(0, hyst);
}

void LdrSmartDesk::setAlpha(float a) {
  _alpha = constrain(a, 0.0f, 1.0f);
}

LdrSmartDesk::State LdrSmartDesk::classify_(int adc, State last) const {
  // Apply symmetric hysteresis around edges
  if (adc <= (_analogHigh - _hyst)) return TOO_BRIGHT;
  if (adc >= (_analogLow  + _hyst)) return TOO_DARK;

  // Inside the band: hold comfort to avoid chatter
  return COMFORT;
}

LdrSmartDesk::State LdrSmartDesk::update() {
  _raw = analogRead(_pin);
  _smooth = _alpha * (float)_raw + (1.0f - _alpha) * _smooth;
  _state = classify_((int)_smooth, _state);
  return _state;
}
