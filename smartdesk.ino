/* Combined Light + Sound Test (with proper Sound logic)
 * Serial: 115200
 * Commands: 'c' = recalibrate light, 's' = recalibrate sound, 'h' = help
 */

#include <Arduino.h>
#include "light.h"
#include "sound.h"

// ---------------- Pins ----------------
static const uint8_t PIN_LIGHT = A0;
static const uint8_t PIN_MIC   = A1;

// ---------------- Light ----------------
Light gLight(PIN_LIGHT, /*vRef=*/5.0f, /*rFixed=*/10000.0f, /*alpha=*/0.20f);

// ---------------- Sound with logic ----------------
class SoundLogic {
public:
  enum State : uint8_t { QUIET = 0, NOISY = 1 };

  // bands are offsets (ADC counts) around baseline
  // loBand: below baseline+loBand forces QUIET (when coming down)
  // hiBand: above baseline+hiBand forces NOISY (when going up)
  SoundLogic(int loBand = 30, int hiBand = 60, float drift = 0.002f)
  : _baseline(512), _avg(512), _loBand(loBand), _hiBand(hiBand),
    _beta(constrain(drift, 0.0f, 0.1f)), _primed(false), _state(QUIET) {}

  void begin(uint8_t pin) {
    sound_begin(pin);
    _t0 = millis();
    _primed = false;
    _acc = 0; _n = 0;
  }

  void autoCalibrate(uint32_t ms = 2000) {
    // collect a quiet-room baseline for 'ms'
    _acc = 0; _n = 0;
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
      int v = sound_read_avg();
      _acc += v; _n++;
      delay(2);
    }
    if (_n > 0) _baseline = (int)(_acc / _n);
    _primed  = true;
  }

  void setBands(int loBand, int hiBand) {
    _loBand = max(0, loBand);
    _hiBand = max(_loBand + 1, hiBand); // ensure hi > lo
  }

  void setDrift(float beta) { _beta = constrain(beta, 0.0f, 0.1f); }

  State update() {
    _avg = sound_read_avg();

    // after calibration, follow slow drift (e.g., temperature, gain)
    if (_primed) {
      _baseline = (int)(_baseline + _beta * (float)(_avg - _baseline));
    } else {
      // if user skipped autoCalibrate(), do a short rolling mean (1s)
      if (millis() - _t0 < 1000) {
        _acc += _avg; _n++;
        _baseline = (int)(_acc / max(1, _n));
      } else {
        _primed = true;
      }
    }

    const int hi = _baseline + _hiBand;
    const int lo = _baseline + _loBand;

    // Hysteresis: go noisy only above hi; return quiet only below lo
    if (_state == QUIET) {
      if (_avg > hi) _state = NOISY;
    } else { // NOISY
      if (_avg < lo) _state = QUIET;
    }
    return _state;
  }

  // Accessors
  State state()   const { return _state; }
  int   avg()     const { return _avg; }
  int   baseline()const { return _baseline; }
  int   loBand()  const { return _loBand; }
  int   hiBand()  const { return _hiBand; }
  bool  primed()  const { return _primed; }

private:
  int      _baseline, _avg;
  int      _loBand, _hiBand;
  float    _beta;
  bool     _primed;

  // scratch for initial averaging
  long     _acc = 0;
  int      _n   = 0;
  uint32_t _t0  = 0;

  State    _state;
};

SoundLogic gSound(/*loBand=*/35, /*hiBand=*/70, /*drift=*/0.002f);

// ---------------- Env logic ----------------
enum EnvState : uint8_t { ENV_OK=0, ENV_TOO_DARK=1, ENV_TOO_BRIGHT=2, ENV_TOO_NOISY=3 };

EnvState classifyEnv(const Light& L, const SoundLogic& S) {
  if (S.state() == SoundLogic::NOISY) return ENV_TOO_NOISY;
  switch (L.state()) {
    case Light::TOO_DARK:   return ENV_TOO_DARK;
    case Light::TOO_BRIGHT: return ENV_TOO_BRIGHT;
    case Light::COMFORT:    return ENV_OK;
  }
  return ENV_OK;
}

static const uint8_t LED = LED_BUILTIN;

const __FlashStringHelper* envName(EnvState s) {
  switch (s) {
    case ENV_OK:         return F("OK");
    case ENV_TOO_DARK:   return F("TOO_DARK");
    case ENV_TOO_BRIGHT: return F("TOO_BRIGHT");
    case ENV_TOO_NOISY:  return F("TOO_NOISY");
  }
  return F("OK");
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(F("\n[EnvTest] Booting..."));
  // --- Light ---
  gLight.begin();
  Serial.println(F("Auto-calibrating light (3s): sweep dark→bright→normal..."));
  gLight.autoCalibrate(3000);

  // --- Sound ---
  gSound.begin(PIN_MIC);
  Serial.println(F("Calibrating sound baseline (2s): keep room at typical noise..."));
  gSound.autoCalibrate(2000);

  Serial.print(F("Light thr L/H/Hyst = "));
  Serial.print(gLight.analogLow()); Serial.print(F("/"));
  Serial.print(gLight.analogHigh()); Serial.print(F("/"));
  Serial.println(gLight.hysteresis());

  Serial.print(F("Sound baseline=")); Serial.print(gSound.baseline());
  Serial.print(F(", bands lo/hi="));  Serial.print(gSound.loBand());
  Serial.print(F("/"));               Serial.println(gSound.hiBand());
}

void loop() {
  // --- Commands ---
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c == 'c' || c == 'C') {
      Serial.println(F("[Cmd] Re-calibrating light (3s)..."));
      gLight.autoCalibrate(3000);
    } else if (c == 's' || c == 'S') {
      Serial.println(F("[Cmd] Re-calibrating sound (2s)..."));
      gSound.autoCalibrate(2000);
    } else if (c == 'h' || c == 'H') {
      Serial.println(F("Commands: c=calibrate light, s=calibrate sound, h=help"));
    }
  }

  // --- Update sensors ---
  gLight.update();
  gSound.update();

  // --- Combined logic & LED ---
  EnvState env = classifyEnv(gLight, gSound);
  static uint32_t lastBlink = 0;
  static bool ledOn = false;
  if (env == ENV_OK) {
    digitalWrite(LED, HIGH);
  } else {
    uint32_t now = millis();
    if (now - lastBlink >= 250) {
      ledOn = !ledOn;
      digitalWrite(LED, ledOn ? HIGH : LOW);
      lastBlink = now;
    }
  }

  // --- Telemetry (for Serial Plotter) ---
  static uint32_t lastPrint = 0;
  uint32_t now = millis();
  if (now - lastPrint >= 200) {
    lastPrint = now;
    Serial.print(F("t="));       Serial.print(now);
    Serial.print(F(",Lraw="));   Serial.print(gLight.raw());
    Serial.print(F(",Lsmooth="));Serial.print((int)gLight.smooth());
    Serial.print(F(",Lstate=")); Serial.print((int)gLight.state());
    Serial.print(F(",Savg="));   Serial.print(gSound.avg());
    Serial.print(F(",Sbase="));  Serial.print(gSound.baseline());
    Serial.print(F(",Sstate=")); Serial.print((int)gSound.state()); // 0=QUIET,1=NOISY
    Serial.print(F(",ENV="));    Serial.println(envName(env));
  }

  delay(1);
}
