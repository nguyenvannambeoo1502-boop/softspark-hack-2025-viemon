#include <Arduino.h>
#include "light.h"
#include "dist.h"

// ---------------- Pins ----------------
static const uint8_t PIN_LIGHT = A0;   // Light sensor analog
static const uint8_t PIN_MIC   = A1;   // Sound sensor analog
static const uint8_t LED_PIN   = 13;   // Built-in LED
// HC-SR04 -> Arduino Uno wiring: VCC->5V, GND->GND, TRIG->D9, ECHO->D8
static const uint8_t PIN_TRIG  = 9;
static const uint8_t PIN_ECHO  = 8;

// ---------------- Instances ----------------
Light  gLight(PIN_LIGHT, 5.0f, 10000.0f, 0.20f);
HCSR04 gRange(PIN_TRIG, PIN_ECHO);

// ---------------- Sound Threshold ----------------
const int SOUND_THRESHOLD = 55;   // adjust experimentally

// ---------------- Study-distance thresholds (OK band) ----------------
// You are OK within [NEAR .. FAR] cm; outside => warn (with hysteresis)
const float STUDY_NEAR_CM = 35.0f;   // too close if < NEAR
const float STUDY_FAR_CM  = 75.0f;   // too far  if > FAR
const float STUDY_HYST_CM = 3.0f;    // ± hysteresis band

// ---------------- Helpers ----------------
static const __FlashStringHelper* lightStateStr(Light::State s) {
  switch (s) {
    case Light::TOO_DARK:   return F("DARK");
    case Light::TOO_BRIGHT: return F("BRIGHT");
    default:                return F("OK");
  }
}

// Distance state as plain bytes (avoid Arduino auto-prototype issues)
enum { DST_TOO_CLOSE = 0, DST_OK = 1, DST_TOO_FAR = 2 };

static uint8_t evalDistState(uint8_t prev, float d_cm) {
  if (isnan(d_cm)) return prev; // hold last state if no echo
  switch (prev) {
    case DST_OK:
      if (d_cm < (STUDY_NEAR_CM - STUDY_HYST_CM)) return DST_TOO_CLOSE;
      if (d_cm > (STUDY_FAR_CM  + STUDY_HYST_CM)) return DST_TOO_FAR;
      return DST_OK;

    case DST_TOO_CLOSE:
      if (d_cm >= (STUDY_NEAR_CM + STUDY_HYST_CM)) {
        if (d_cm > (STUDY_FAR_CM + STUDY_HYST_CM)) return DST_TOO_FAR;
        return DST_OK;
      }
      return DST_TOO_CLOSE;

    case DST_TOO_FAR:
      if (d_cm <= (STUDY_FAR_CM - STUDY_HYST_CM)) {
        if (d_cm < (STUDY_NEAR_CM - STUDY_HYST_CM)) return DST_TOO_CLOSE;
        return DST_OK;
      }
      return DST_TOO_FAR;
  }
  return prev;
}

static const __FlashStringHelper* distStateToText(uint8_t s) {
  switch (s) {
    case DST_TOO_CLOSE: return F("TOO_CLOSE");
    case DST_OK:        return F("OK");
    default:            return F("TOO_FAR");
  }
}

// Edge-triggered notifications (LIGHT/SOUND)
static void notifyIfChanged(Light::State currL, bool noisy) {
  static bool first = true;
  static Light::State prevL;
  static bool prevNoisy;

  if (first || currL != prevL) {
    Serial.print(F("[LIGHT] ")); Serial.println(lightStateStr(currL));
    prevL = currL;
  }
  if (first || noisy != prevNoisy) {
    Serial.print(F("[SOUND] ")); Serial.println(noisy ? F("NOISY") : F("QUIET"));
    prevNoisy = noisy;
  }
  first = false;
}

// Edge-triggered notifications (DISTANCE)
static void notifyIfDistChanged(uint8_t s) {
  static bool first = true;
  static uint8_t prev = DST_OK;
  if (first || s != prev) {
    Serial.print(F("[DIST] ")); Serial.println(distStateToText(s));
    if (s == DST_TOO_CLOSE) {
      Serial.println(F("[WARN] Too close to screen — lean back."));
    } else if (s == DST_TOO_FAR) {
      Serial.println(F("[WARN] Too far — lean closer."));
    }
    prev = s;
    first = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);

  // ---- LIGHT ----
  gLight.begin();
  gLight.setThresholds(700, 900, 25);   // too dark <700, OK 750–900, too bright >900

  // ---- SOUND ----
  pinMode(PIN_MIC, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // ---- DISTANCE (HC-SR04) ----
  gRange.begin();

  // ---- Boot banner ----
  Serial.println(F("=== SMART DESK MONITOR ==="));
  Serial.print(F("Light thr: low="));  Serial.print(gLight.analogLow());
  Serial.print(F(", high="));          Serial.print(gLight.analogHigh());
  Serial.print(F(", hyst="));          Serial.println(gLight.hysteresis());
  Serial.print(F("Sound thr: "));      Serial.println(SOUND_THRESHOLD);
  Serial.print(F("Study dist OK: ["));
  Serial.print(STUDY_NEAR_CM, 0); Serial.print(F(", "));
  Serial.print(STUDY_FAR_CM, 0);  Serial.println(F("] cm"));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  // ---- LIGHT ----
  gLight.update();

  // ---- SOUND ----
  const int  soundRaw = analogRead(PIN_MIC);
  const bool noisy    = (soundRaw > SOUND_THRESHOLD);
  digitalWrite(LED_PIN, noisy ? HIGH : LOW);
  notifyIfChanged(gLight.state(), noisy);

  // ---- DISTANCE (sample ~3×/s) ----
  static uint32_t tRangePrev = 0;
  static uint8_t distS = DST_OK;
  static float lastDcm = NAN;

  if (millis() - tRangePrev >= 330) {
    tRangePrev = millis();

    // quick median for responsiveness
    float d_cm = gRange.readCmMedian(3, 40);  // ~120 ms total
    // Optional sanity clamp to reduce spurious spikes
    if (!isnan(d_cm) && (d_cm < 2.0f || d_cm > 400.0f)) d_cm = NAN;

    distS = evalDistState(distS, d_cm);
    notifyIfDistChanged(distS);
    lastDcm = d_cm; // keep the most recent reading for the 1 Hz line
  }

  // ---- Periodic neat status (1 Hz) ----
  static uint32_t tPrev = 0;
  if (millis() - tPrev > 1000) {
    tPrev = millis();
    // Format: LIGHT=<raw/state> | SOUND=<raw/state> | DIST=<cm/state>
    Serial.print(F("LIGHT=")); Serial.print(gLight.raw());
    Serial.print(F("/"));       Serial.print(lightStateStr(gLight.state()));
    Serial.print(F(" | SOUND=")); Serial.print(soundRaw);
    Serial.print(F("/"));         Serial.print(noisy ? F("NOISY") : F("QUIET"));
    Serial.print(F(" | DIST="));
    if (isnan(lastDcm)) {
      Serial.print(F("--"));
    } else {
      Serial.print(lastDcm, 1);
    }
    Serial.print(F("cm/")); Serial.println(distStateToText(distS));
  }

  // Keep loop responsive
  delay(1);
}
