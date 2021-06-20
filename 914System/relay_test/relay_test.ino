/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

#include <Arduino.h>

#define DELAY 1000

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  for (int i=2 ; i<=9 ; i++) {
    pinMode(i, OUTPUT);
   digitalWrite(i, HIGH);    
  }
}

// the loop function runs over and over again forever
void loop() {
  #if 1

  digitalWrite(8, LOW);    
  delay(DELAY);
  digitalWrite(8, HIGH);    
  delay(DELAY);

  
  #else
  
  for (int i=2 ; i<=9 ; i++) {
    digitalWrite(i, LOW);    
  }  
  delay(DELAY);

  for (int i=2 ; i<=9 ; i++) {
    digitalWrite(i, HIGH);    
  }
  delay(DELAY);
  #endif
}
