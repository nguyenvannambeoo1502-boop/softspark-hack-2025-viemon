#include "light.h"

Light::Light(uint8_t analogPin, float vRef, float rFixed, float alpha)
: _pin(analogPin),
  _vRef(vRef),
  _rFixed(rFixed),
  _alpha(constrain(alpha, 0.0f, 1.0f)),
  _raw(0),
  _smooth(0.0f),
  _analogLow(650),   // default indoor threshold
  _analogHigh(350),
  _hyst(20),
  _state(COMFORT)
{}

void Light::begin() {
  _raw = analogRead(_pin);
  _smooth = (float)_raw;
}

void Light::autoCalibrate(uint32_t ms) {
  int vMin = 1023, vMax = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    int v = analogRead(_pin);
    if (v < vMin) vMin = v;
    if (v > vMax) vMax = v;
    delay(10);
  }

  int span = vMax - vMin;
  if (span < 10) span = 10;

  int lower = vMin + (int)(0.30f * span);
  int upper = vMin + (int)(0.70f * span);

  _analogHigh = lower;   // too bright ≤ this
  _analogLow  = upper;   // too dark ≥ this
  _hyst = max(5, span / 25);

  _raw = analogRead(_pin);
  _smooth = (float)_raw;
  _state = COMFORT;
}

void Light::setThresholds(int analogLow, int analogHigh, int hyst) {
  _analogLow  = constrain(analogLow, 0, 1023);
  _analogHigh = constrain(analogHigh, 0, 1023);
  _hyst       = max(0, hyst);
}

void Light::setAlpha(float a) {
  _alpha = constrain(a, 0.0f, 1.0f);
}

Light::State Light::classify_(int adc, State last) const {
  if (adc <= (_analogHigh - _hyst)) return TOO_BRIGHT;
  if (adc >= (_analogLow  + _hyst)) return TOO_DARK;
  return COMFORT;
}

Light::State Light::update() {
  _raw = analogRead(_pin);
  _smooth = _alpha * (float)_raw + (1.0f - _alpha) * _smooth;
  _state = classify_((int)_smooth, _state);
  return _state;
}
