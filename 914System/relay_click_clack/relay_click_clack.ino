#include <Arduino.h>

byte num_pins = 8;
byte pins[8] = {2, 3, 4, 5, 6, 7, 8, 9};

void setup() {

  Serial.begin(9600); while (!Serial) /* Waits for serial port to connect (needed for Leonardo only) */;

  for (byte i = 0; i < num_pins; i++) {
    byte pin = pins[i];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
}

void loop() {
  byte data;

  Serial.println("turning channels on...");
  for (byte i = 0; i < num_pins; i++) {
    digitalWrite(pins[i], LOW);
    delay(500);
  }
  Serial.println("done");

  delay(1000);

  Serial.println("turning channels off...");

  for (byte i = 0; i < num_pins; i++) {
    digitalWrite(pins[i], HIGH);
    delay(500);
  }
  Serial.println("done");

  delay(1000);
}
