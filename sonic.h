#ifndef HCSR04_H
#define HCSR04_H

#include <Arduino.h>

class HCSR04 {
  private:
    int trigPin;
    int echoPin;
  public:
    HCSR04(int trig, int echo);   // Constructor
    long getDistance();           // Returns distance in cm
};

#endif
