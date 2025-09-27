#include <Arduino.h>
#include "light.h"
#include "dist.h"

// ---------------- Pins ----------------
static const uint8_t PIN_LIGHT = A0;   // Light sensor analog (LDR divider)
static const uint8_t PIN_THERM = A1;   // Thermistor analog
static const uint8_t PIN_TRIG  = 9;    // HC-SR04
static const uint8_t PIN_ECHO  = 8;
static const uint8_t BTN_PIN   = 7;

// LEDs (warnings)
static const uint8_t LED_TEMP     = 13; // LED1: temp out of comfort (too cold/hot)
static const uint8_t LED_DISTANCE = 12; // LED2: too close
static const uint8_t LED_LIGHT    = 11; // LED3: too dark

// ---------------- Instances ----------------
Light  gLight(PIN_LIGHT, 5.0f, 10000.0f, 0.20f);
HCSR04 gRange(PIN_TRIG, PIN_ECHO);

// ---------------- Thermistor params ----------------
// Wiring assumed: 5V — Thermistor — A1 — 10k (THERM_R_SER) — GND
// (So higher temp -> lower R_therm -> higher ADC)
static const float THERM_R0     = 10000.0f;   // 10k at 25 °C
static const float THERM_T0_C   = 25.0f;
static const float THERM_BETA   = 3950.0f;
static const float THERM_R_SER  = 10000.0f;   // series resistor to GND
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

// ---- Thermistor conversion (Steinhart–Hart β model) ----
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

  // Sensors
  gLight.begin();
  // NOTE (wiring 5V—LDR—A0—10k—GND): brighter -> LOWER ADC, darker -> HIGHER ADC.
  // So dark bound > bright bound:
  gLight.setThresholds(/*analogLow(dark)*/700, /*analogHigh(bright)*/350, /*hyst*/25);

  pinMode(PIN_THERM, INPUT);
  gRange.begin();

  // LEDs
  pinMode(LED_TEMP,     OUTPUT);
  pinMode(LED_DISTANCE, OUTPUT);
  pinMode(LED_LIGHT,    OUTPUT);
  digitalWrite(LED_TEMP,     LOW);
  digitalWrite(LED_DISTANCE, LOW);
  digitalWrite(LED_LIGHT,    LOW);

  Serial.println(F("=== SMART DESK MONITOR (Light + Thermistor + Distance) ==="));
}

void loop() {
  checkButton();
  if (!sensorsEnabled) {
    digitalWrite(LED_TEMP,     LOW);
    digitalWrite(LED_DISTANCE, LOW);
    digitalWrite(LED_LIGHT,    LOW);
    delay(10);
    return;
  }

  // ---- LIGHT ----
  gLight.update();
  // LED3 ON when lack of light
  digitalWrite(LED_LIGHT, (gLight.state() == Light::TOO_DARK) ? HIGH : LOW);

  // ---- THERMISTOR (Temperature) ----
  int   adc    = analogRead(PIN_THERM);
  float rTherm = thermistorResistance(adc);
  float tempC  = thermistorTempC(rTherm);
  bool  tempBad = false;
  const __FlashStringHelper* tempState;
  if (isnan(tempC)) {
    tempState = F("N/A");
    tempBad   = false; // keep LED off if invalid reading
  } else if (tempC < 20.0f) {
    tempState = F("TOO_COLD");
    tempBad   = true;
  } else if (tempC <= 30.0f) {
    tempState = F("OK");
    tempBad   = false;
  } else {
    tempState = F("TOO_HOT");
    tempBad   = true;
  }
  // LED1 ON when temp out of comfort
  digitalWrite(LED_TEMP, tempBad ? HIGH : LOW);

  // ---- DISTANCE ----
  static uint32_t tRangePrev = 0;
  static uint8_t  distS      = DST_OK;
  static float    lastDcm    = NAN;

  if (millis() - tRangePrev >= 330) {
    tRangePrev = millis();
    float d_cm = gRange.readCmMedian(3, 40);
    if (!isnan(d_cm) && (d_cm < 2.0f || d_cm > 400.0f)) d_cm = NAN;
    distS = evalDistState(distS, d_cm);
    lastDcm = d_cm;
  }
  // LED2 ON when too close
  digitalWrite(LED_DISTANCE, (distS == DST_TOO_CLOSE) ? HIGH : LOW);

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
