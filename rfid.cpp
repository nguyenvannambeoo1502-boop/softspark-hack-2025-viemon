#include "rfid.h"

const unsigned long REMOVAL_TIMEOUT_MS = 1000; // 1s delay before OFF

RfidToggle::RfidToggle(byte ssPin, byte rstPin)
  : rfid(ssPin, rstPin), studyMode(false), cardPresent(false), lastSeenMs(0),
    authUid(nullptr), authUidLen(0) {}

void RfidToggle::begin(const byte *authorizedUid, byte uidLength) {
    authUid = authorizedUid;
    authUidLen = uidLength;
    SPI.begin();
    rfid.PCD_Init();
}

void RfidToggle::update() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        // check UID
        bool match = (rfid.uid.size == authUidLen);
        for (byte i = 0; i < authUidLen && match; i++) {
            if (rfid.uid.uidByte[i] != authUid[i]) match = false;
        }

        if (match) {
            lastSeenMs = millis();
            if (!cardPresent) {
                cardPresent = true;
                studyMode = true; // switch ON
            }
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }

    // Check removal timeout
    if (cardPresent && (millis() - lastSeenMs > REMOVAL_TIMEOUT_MS)) {
        cardPresent = false;
        studyMode = false; // switch OFF
    }
}

bool RfidToggle::isOn() {
    return studyMode;
}
