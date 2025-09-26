#include "HCSR04.h"

HCSR04::HCSR04(int trig, int echo) {
  trigPin = trig;
  echoPin = echo;
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

long HCSR04::getDistance() {
  long duration;
  int distance;

  // Send trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo
  duration = pulseIn(echoPin, HIGH);

  // Convert to cm (speed of sound 343 m/s)
  distance = duration / 58;

  return distance;
}
