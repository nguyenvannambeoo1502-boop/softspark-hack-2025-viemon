#include <Arduino.h>
#include "light.h"
#include "dist.h"

// ---------------- Pins ----------------
static const uint8_t PIN_LIGHT = A0;   // Light sensor analog
static const uint8_t PIN_THERM = A1;   // Thermistor analog
static const uint8_t PIN_TRIG  = 9;
static const uint8_t PIN_ECHO  = 8;
static const uint8_t BTN_PIN   = 7;

// ---------------- Instances ----------------
Light  gLight(PIN_LIGHT, 5.0f, 10000.0f, 0.20f);
HCSR04 gRange(PIN_TRIG, PIN_ECHO);

// ---------------- Thermistor params ----------------
static const float THERM_R0     = 10000.0f;   // 10k at 25 °C
static const float THERM_T0_C   = 25.0f;
static const float THERM_BETA   = 3950.0f;
static const float THERM_R_SER  = 10000.0f;
static const float ADC_MAX      = 1023.0f;

// ---------------- Distance thresholds ----------------
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

// Thermistor conversion
static float thermistorResistance(int adc) {
  if (adc <= 0 || adc >= 1023) return NAN;
  return THERM_R_SER * (ADC_MAX / (float)adc - 1.0f);
}

static float thermistorTempC(float R) {
  if (!(R > 0.0f)) return NAN;
  const float T0_K = THERM_T0_C + 273.15f;
  float invT = (1.0f / T0_K) + (1.0f / THERM_BETA) * log(R / THERM_R0);
  return (1.0f / invT) - 273.15f;
}

// ---------------- Global enable ----------------
static bool sensorsEnabled = true;

static void checkButton() {
  static bool lastState = HIGH;
  bool state = digitalRead(BTN_PIN);

  if (lastState == HIGH && state == LOW) {
    sensorsEnabled = !sensorsEnabled;
    Serial.print(F("[MODE] Sensors "));
    Serial.println(sensorsEnabled ? F("ENABLED") : F("DISABLED"));
  }
  lastState = state;
}

void setup() {
  Serial.begin(115200);
  delay(50);

  pinMode(BTN_PIN, INPUT_PULLUP);
  gLight.begin();
  gLight.setThresholds(700, 900, 25);
  pinMode(PIN_THERM, INPUT);
  gRange.begin();

  Serial.println(F("=== SMART DESK MONITOR (Light + Thermistor + Distance) ==="));
}

void loop() {
  checkButton();
  if (!sensorsEnabled) { delay(10); return; }

  // ---- LIGHT ----
  gLight.update();

  // ---- THERMISTOR ----
  int   adc    = analogRead(PIN_THERM);
  float rTherm = thermistorResistance(adc);
  float tempC  = thermistorTempC(rTherm);

  const __FlashStringHelper* tempState;
  if (isnan(tempC))           tempState = F("N/A");
  else if (tempC < 20.0f)     tempState = F("TOO_COLD");
  else if (tempC <= 30.0f)    tempState = F("OK");
  else                        tempState = F("TOO_HOT");

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

    Serial.print(F(" | TEMP="));
    if (isnan(tempC)) Serial.print(F("--"));
    else Serial.print(tempC, 1);
    Serial.print(F("C/")); 
    Serial.print(tempState);

    Serial.print(F(" | DIST="));
    if (isnan(lastDcm)) Serial.print(F("--"));
    else Serial.print(lastDcm, 1);
    Serial.print(F("cm/")); Serial.println(distStateToText(distS));
  }
}
