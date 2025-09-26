#include <Arduino.h>
#include "dist.h"   // distance sensor functions
#include "ldr.h"    // light sensor functions
#include "rfid.h"   // RFID functions
#include "sound.h"
// print every 500 ms
const uint32_t PRINT_INTERVAL = 500;
uint32_t lastPrint = 0;

void setup() {
  Serial.begin(115200);

  // Initialize each sensor
  dist_begin(7, 6);     // HC-SR04 (TRIG=7, ECHO=6)
  ldr_begin(A0);        // LDR on analog pin A0
  rfid_begin(10, 4);    // RC522 (SS=10, RST=4)

  Serial.println(F("SmartDeskMVP starting..."));
}

void loop() {
  uint32_t now = millis();

  // print data line every 500 ms
  if (now - lastPrint >= PRINT_INTERVAL) {
    lastPrint = now;

    // Distance
    int cm = -1; 
    bool okDist = dist_read_cm(cm);

    // Light
    int lightPct = ldr_read_pct();

    // RFID
    char uid[16] = {0}; 
    bool authorized = false;
    bool seen = rfid_poll(uid, authorized);

    // Output to serial
    Serial.print("t="); Serial.print(now);
    Serial.print(", dist="); Serial.print(okDist ? cm : -1);
    Serial.print(", light="); Serial.print(lightPct);
    Serial.print(", uid="); Serial.print(seen ? uid : "");
    Serial.print(", auth="); Serial.println(seen ? (authorized ? 1 : 0) : 0);
  }
}
