#include "light.h"

namespace {
  constexpr int ADC_MIN = Light::kAdcMin;
  constexpr int ADC_MAX = Light::kAdcMax;

  inline int clampAdc(int x) {
    if (x < ADC_MIN) return ADC_MIN;
    if (x > ADC_MAX) return ADC_MAX;
    return x;
  }
}

Light::Light(uint8_t analogPin, float vRef, float rFixed, float alpha)
: _pin(analogPin),
  _vRef(vRef),
  _rFixed(rFixed),
  _alpha(constrain(alpha, 0.0f, 1.0f)),
  _raw(0),
  _smooth(0.0f),
  // Defaults (won't matter once you call setThresholds)
  _analogLow(650),      // logical "too dark" trip
  _analogHigh(350),     // logical "too bright" trip
  _hyst(20),
  _inverted(true),      // kept for compatibility; not used by classify_ below
  _state(COMFORT)
{}

void Light::begin() {
  pinMode(_pin, INPUT);
  delay(5);
  _raw = analogRead(_pin);
  _smooth = float(_raw);
}

void Light::autoCalibrate(uint32_t ms) {
  // Kept for optional use; NOT needed if you set fixed thresholds in setup().
  int vMin = ADC_MAX, vMax = ADC_MIN;
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    int v = analogRead(_pin);
    if (v < vMin) vMin = v;
    if (v > vMax) vMax = v;
    delay(10);
  }

  int span = vMax - vMin;
  if (span < 10) span = 10;

  const int edge30 = vMin + int(0.30f * span);
  const int edge70 = vMin + int(0.70f * span);

  // Default mapping (legacy); ignored if you later call setThresholds().
  _analogHigh = clampAdc(edge30); // bright side
  _analogLow  = clampAdc(edge70); // dark side

  int hyst = max(5, span / 25);
  _hyst = hyst;

  _raw = analogRead(_pin);
  _smooth = float(_raw);
  _state = COMFORT;
}

void Light::setThresholds(int analogLow, int analogHigh, int hyst) {
  // Fixed thresholds you want: e.g., 700 (dark) and 900 (bright)
  _analogLow  = clampAdc_(analogLow);
  _analogHigh = clampAdc_(analogHigh);
  _hyst       = max(0, hyst);
}

void Light::setAlpha(float a) {
  _alpha = constrain(a, 0.0f, 1.0f);
}

Light::State Light::classify_(int adc, State /*last*/) const {
  // Fixed-meaning classification:
  //   TOO_DARK    if adc <= (low - H)
  //   TOO_BRIGHT  if adc >= (high + H)
  //   COMFORT     otherwise
  const int H = _hyst;

  if (adc <= (_analogLow - H))  return TOO_DARK;     // e.g., < 700
  if (adc >= (_analogHigh + H)) return TOO_BRIGHT;   // e.g., > 900
  return COMFORT;                                     // 750..900 with H=25
}

Light::State Light::update() {
  _raw = analogRead(_pin);
  _smooth = _alpha * float(_raw) + (1.0f - _alpha) * _smooth;
  const int adc = int(_smooth + 0.5f);
  _state = classify_(adc, _state);
  return _state;
}
