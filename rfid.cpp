#include "rfid.h"

const unsigned long REMOVAL_TIMEOUT_MS = 1000; // 1s used only to debounce presence

RfidToggle::RfidToggle(byte ssPin, byte rstPin)
  : rfid(ssPin, rstPin),
    studyMode(false),
    cardPresent(false),
    lastSeenMs(0),
    authUid(nullptr),
    authUidLen(0) {}

void RfidToggle::begin(const byte *authorizedUid, byte uidLength) {
    authUid    = authorizedUid;
    authUidLen = uidLength;
    SPI.begin();
    rfid.PCD_Init();
}

void RfidToggle::update() {
    // Detect new authorized card
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        bool match = (rfid.uid.size == authUidLen);
        for (byte i = 0; i < authUidLen && match; i++) {
            if (rfid.uid.uidByte[i] != authUid[i]) match = false;
        }

        if (match) {
            lastSeenMs = millis();
            // Rising edge: previously not present -> now present  ==> TOGGLE latch
            if (!cardPresent) {
                cardPresent = true;
                studyMode = !studyMode;   // <--- LATCHED TOGGLE on tap
            }
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }

    // Debounce removal: once card is away for a while, allow next tap to toggle again
    if (cardPresent && (millis() - lastSeenMs > REMOVAL_TIMEOUT_MS)) {
        cardPresent = false;   // <--- IMPORTANT: do NOT force studyMode = false
    }
}

bool RfidToggle::isOn() {
    return studyMode;  // latched state
}
