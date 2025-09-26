#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>

class Light {
public:
  enum State : uint8_t { TOO_DARK = 0, COMFORT = 1, TOO_BRIGHT = 2 };

  // Constructor: specify analog pin, ADC reference voltage (5V typical), fixed resistor (ohms), smoothing factor alpha
  Light(uint8_t analogPin, float vRef = 5.0f, float rFixed = 10000.0f, float alpha = 0.20f);

  void begin();

  // Auto-calibration: sweep light during ms milliseconds (dark → bright → normal)
  void autoCalibrate(uint32_t ms = 3000);

  // Manually set thresholds in ADC counts (0..1023)
  void setThresholds(int analogLow, int analogHigh, int hyst);

  // Update reading, return state
  State update();

  // Accessors
  State state()  const { return _state; }
  int   raw()    const { return _raw; }
  float smooth() const { return _smooth; }
  float volts()  const { return (_smooth / 1023.0f) * _vRef; }

  void setAlpha(float a);
  void setRFixed(float r) { _rFixed = r; }
  void setVref(float v)   { _vRef = v; }

  int analogLow()  const { return _analogLow; }
  int analogHigh() const { return _analogHigh; }
  int hysteresis() const { return _hyst; }

private:
  uint8_t _pin;
  float   _vRef;
  float   _rFixed;
  float   _alpha;

  int     _raw;
  float   _smooth;

  int     _analogLow;
  int     _analogHigh;
  int     _hyst;

  State   _state;

  State classify_(int adc, State last) const;
};

#endif // LIGHT_H
