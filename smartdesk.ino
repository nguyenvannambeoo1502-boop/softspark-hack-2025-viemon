#include <Arduino.h>
#include "light.h"
#include "dist.h"

// ---------------- Pins ----------------
static const uint8_t PIN_LIGHT = A0;   // Light sensor analog
static const uint8_t PIN_MIC   = A1;   // Sound sensor analog
static const uint8_t LED_PIN   = 13;   // Built-in LED
static const uint8_t PIN_TRIG  = 9;
static const uint8_t PIN_ECHO  = 8;

// Button
static const uint8_t BTN_PIN = 7;

// ---------------- Instances ----------------
Light  gLight(PIN_LIGHT, 5.0f, 10000.0f, 0.20f);
HCSR04 gRange(PIN_TRIG, PIN_ECHO);

// ---------------- Thresholds ----------------
const int SOUND_THRESHOLD = 55;
const float STUDY_NEAR_CM = 35.0f;
const float STUDY_FAR_CM  = 75.0f;
const float STUDY_HYST_CM = 3.0f;

// ---------------- Helpers ----------------
static const __FlashStringHelper* lightStateStr(Light::State s) {
  switch (s) {
    case Light::TOO_DARK:   return F("DARK");
    case Light::TOO_BRIGHT: return F("BRIGHT");
    default:                return F("OK");
  }
}

enum { DST_TOO_CLOSE = 0, DST_OK = 1, DST_TOO_FAR = 2 };

static uint8_t evalDistState(uint8_t prev, float d_cm) {
  if (isnan(d_cm)) return prev;
  if (d_cm < (STUDY_NEAR_CM - STUDY_HYST_CM)) return DST_TOO_CLOSE;
  if (d_cm > (STUDY_FAR_CM  + STUDY_HYST_CM)) return DST_TOO_FAR;
  return DST_OK;
}

static const __FlashStringHelper* distStateToText(uint8_t s) {
  switch (s) {
    case DST_TOO_CLOSE: return F("TOO_CLOSE");
    case DST_OK:        return F("OK");
    default:            return F("TOO_FAR");
  }
}

// ---------------- Global enable ----------------
static bool sensorsEnabled = true;

// Simple button toggle
static void checkButton() {
  static bool lastState = HIGH;
  bool state = digitalRead(BTN_PIN);

  if (lastState == HIGH && state == LOW) { // falling edge (pressed)
    sensorsEnabled = !sensorsEnabled;
    Serial.print(F("[MODE] Sensors "));
    Serial.println(sensorsEnabled ? F("ENABLED") : F("DISABLED"));
  }
  lastState = state;
}

void setup() {
  Serial.begin(115200);
  delay(50);

  // Button uses INPUT_PULLUP (connect between pin and GND)
  pinMode(BTN_PIN, INPUT_PULLUP);

  gLight.begin();
  gLight.setThresholds(700, 900, 25);
  pinMode(PIN_MIC, INPUT);
  pinMode(LED_PIN, OUTPUT);
  gRange.begin();

  Serial.println(F("=== SMART DESK MONITOR ==="));
}

void loop() {
  checkButton();

  if (!sensorsEnabled) {
    delay(10);
    return;
  }

  // ---- LIGHT ----
  gLight.update();

  // ---- SOUND ----
  int  soundRaw = analogRead(PIN_MIC);
  bool noisy    = (soundRaw > SOUND_THRESHOLD);
  digitalWrite(LED_PIN, noisy ? HIGH : LOW);

  // ---- DISTANCE ----
  static uint32_t tRangePrev = 0;
  static uint8_t distS = DST_OK;
  static float lastDcm = NAN;

  if (millis() - tRangePrev >= 330) {
    tRangePrev = millis();
    float d_cm = gRange.readCmMedian(3, 40);
    if (!isnan(d_cm) && (d_cm < 2.0f || d_cm > 400.0f)) d_cm = NAN;
    distS = evalDistState(distS, d_cm);
    lastDcm = d_cm;
  }

  // ---- Periodic status ----
  static uint32_t tPrev = 0;
  if (millis() - tPrev > 1000) {
    tPrev = millis();
    Serial.print(F("LIGHT=")); Serial.print(gLight.raw());
    Serial.print(F("/")); Serial.print(lightStateStr(gLight.state()));
    Serial.print(F(" | SOUND=")); Serial.print(soundRaw);
    Serial.print(F("/")); Serial.print(noisy ? F("NOISY") : F("QUIET"));
    Serial.print(F(" | DIST="));
    if (isnan(lastDcm)) Serial.print(F("--"));
    else Serial.print(lastDcm, 1);
    Serial.print(F("cm/")); Serial.println(distStateToText(distS));
  }
}
