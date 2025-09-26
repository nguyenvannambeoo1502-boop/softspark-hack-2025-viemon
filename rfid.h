#ifndef RFID_H
#define RFID_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

class RfidToggle {
public:
    // Constructor: pass SS and RST pin numbers
    RfidToggle(byte ssPin, byte rstPin);

    // Setup function (call in setup())
    void begin(const byte *authorizedUid, byte uidLength);

    // Call in loop() to update state
    void update();

    // Get current state (true = ON, false = OFF)
    bool isOn();

private:
    MFRC522 rfid;              // RC522 object
    bool studyMode;             // current ON/OFF state
    bool cardPresent;           // is card currently in field
    unsigned long lastSeenMs;   // last time card seen
    const byte* authUid;        // authorized UID
    byte authUidLen;            // length of UID
};

#endif
