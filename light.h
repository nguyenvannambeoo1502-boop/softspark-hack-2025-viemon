#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>

class Light {
public:
  static constexpr int kAdcMin = 0;
  static constexpr int kAdcMax = 1023;

  enum State : uint8_t { TOO_DARK = 0, COMFORT = 1, TOO_BRIGHT = 2 };

  Light(uint8_t analogPin, float vRef = 5.0f, float rFixed = 10000.0f, float alpha = 0.20f);

  void begin();
  void autoCalibrate(uint32_t ms = 3000);

  // Thresholds in ADC counts (0..1023). They are LOGICAL:
  //  - analogLow  = trip toward TOO_DARK
  //  - analogHigh = trip toward TOO_BRIGHT
  // Their numeric ordering depends on wiring; we do NOT force low>high or low<high.
  void setThresholds(int analogLow, int analogHigh, int hyst);

  State update();

  // Accessors
  State state()  const { return _state; }
  int   raw()    const { return _raw; }
  float smooth() const { return _smooth; }
  float volts()  const { return (_smooth / float(kAdcMax)) * _vRef; }

  void setAlpha(float a);
  void setRFixed(float r) { _rFixed = r; }
  void setVref(float v)   { _vRef = v; }

  int  analogLow()  const { return _analogLow; }
  int  analogHigh() const { return _analogHigh; }
  int  hysteresis() const { return _hyst; }

  // Wiring: true  -> higher ADC = darker  (LDR at bottom of divider)
  //         false -> higher ADC = brighter (LDR at top of divider)
  void setInverted(bool inv) { _inverted = inv; }
  bool inverted() const { return _inverted; }

private:
  uint8_t _pin;
  float   _vRef;
  float   _rFixed;
  float   _alpha;

  int     _raw;
  float   _smooth;

  int     _analogLow;    // logical "too dark" trip
  int     _analogHigh;   // logical "too bright" trip
  int     _hyst;

  bool    _inverted;     // see above
  State   _state;

  State classify_(int adc, State last) const;

  static int clampAdc_(int x) {
    if (x < kAdcMin) return kAdcMin;
    if (x > kAdcMax) return kAdcMax;
    return x;
  }
};

#endif // LIGHT_H
