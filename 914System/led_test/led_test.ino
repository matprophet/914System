#include <Arduino.h>
#include <Adafruit_TLC5947.h>
#include <Mux.h>

using namespace admux;

// How many boards do you have chained?
#define NUM_TLC5974 1
#define dataPin    7 // pin
#define clockPin   8 // pin
#define latchPin   9 // pin
Adafruit_TLC5947 gLEDDriver = Adafruit_TLC5947(NUM_TLC5974, clockPin, dataPin, latchPin);

#define FLASH_DELAY 200
#define NUM_ELEMENTS 8
#define NUM_BUTTONS 8
#define LED_ON 4096
#define LED_OFF 0

void setLight(int l) {
  uint16_t leds[NUM_BUTTONS];
  for (byte i=0; i < NUM_ELEMENTS; i++) {
    leds[i] = (i == l) ? LED_ON : LED_OFF;
  }  

  gLEDDriver.setLED(0, leds[0], leds[1], leds[2]);
  gLEDDriver.setLED(1, leds[3], leds[4], leds[5]);
  gLEDDriver.setLED(2, leds[6], leds[7], 0);
  gLEDDriver.write();
}

void setup() {
  gLEDDriver.begin();
}

void loop() {
  for (byte i=0; i < NUM_ELEMENTS; i++) {
    setLight(i);
    delay(500);
  }
}
