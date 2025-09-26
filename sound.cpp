#include "sound.h"

static uint8_t PIN_MIC;
void sound_begin(uint8_t pin) { PIN_MIC = pin; pinMode(PIN_MIC, INPUT); }

int sound_read_raw() { return analogRead(PIN_MIC); }

int sound_read_avg() {
  long sum=0;
  for(int i=0;i<10;i++) sum += analogRead(PIN_MIC);
  return sum/10;
}
