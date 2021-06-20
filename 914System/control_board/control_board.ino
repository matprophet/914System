#include <Arduino.h>
#include <Adafruit_TLC5947.h>
#include <Mux.h>
#include <mcp_can.h>
#include <arduino-timer.h>

#define DEBUG 1
#include <914CAN.h>

using namespace admux;

// How many boards do you have chained?
#define NUM_TLC5974 1
#define dataPin    7 // pin
#define clockPin   8 // pin
#define latchPin   9 // pin

Adafruit_TLC5947 gLEDDriver = Adafruit_TLC5947(NUM_TLC5974, clockPin, dataPin, latchPin);
Mux switchesMux(Pin(6, INPUT_PULLUP, PinType::Digital), Pinset(2, 3, 4, 5));
Timer<1, millis> timer; // create a timer with 1 task and millisecond resolution
MCP_CAN gCanbus(CAN_CS_PIN);

#define FLASH_DELAY 100

typedef enum : byte {
  kButtonActionToggle,
  kButtonActionMomentary,
} ButtonAction;

typedef struct {
  CANFunction function;
  ButtonAction action;
  byte switchChannel; // on the mux
  byte ledBaseChannel;
  byte ledSubChannel;
  bool ledState;
  byte isInTransition;
  bool state;
} Element;

#define NUM_ELEMENTS 8
#define LED_OFF      0
#define LED_ON       4096

Element gElements[] = {
  { kCANFunctionStarter,         kButtonActionMomentary, 0, 0, 0, false, false, false, },
  { kCANFunctionFuelPumps,       kButtonActionToggle,    1, 0, 1, false, false, false, },
  { kCANFunctionHazards,         kButtonActionToggle,    2, 0, 2, false, false, false, },
  { kCANFunctionFans,            kButtonActionToggle,    3, 1, 0, false, false, false, },
  { kCANFunctionTurnSignalLeft,  kButtonActionToggle,    4, 1, 1, false, false, false, },
  { kCANFunctionHeadlightsLow,   kButtonActionToggle,    5, 1, 2, false, false, false, },
  { kCANFunctionTurnSignalRight, kButtonActionToggle,    6, 2, 0, false, false, false, },
  { kCANFunctionHeadlightsHigh,  kButtonActionToggle,    7, 2, 1, false, false, false, },
};

// ==============================================================
//  Timer
// ==============================================================
bool handleTimer(void *) {
  bool doLEDUpdate = false;

  if (getStateForFunction(kCANFunctionHazards)) {
    byte ledState = getLEDStateForFunction(kCANFunctionHazards);
    setLEDStateForFunction(kCANFunctionHazards, ledState == false ? true : false);
    doLEDUpdate = true;
  }
  else if (getStateForFunction(kCANFunctionTurnSignalLeft)) {
    byte ledState = getLEDStateForFunction(kCANFunctionTurnSignalLeft);
    setLEDStateForFunction(kCANFunctionTurnSignalLeft, ledState == false ? true : false);
    doLEDUpdate = true;
  }
  else if (getStateForFunction(kCANFunctionTurnSignalRight)) {
    byte ledState = getLEDStateForFunction(kCANFunctionTurnSignalRight);
    setLEDStateForFunction(kCANFunctionTurnSignalRight, ledState == false ? true : false);
    doLEDUpdate = true;    
  }

  if (doLEDUpdate) {
    updateLEDs();
  }
  
  return true; // repeat? true
}

// ---------------------------------------------------------------
void setupTimer() {
  // call the handleTimer function every 500 millis (1/2 second)
  timer.every(500, handleTimer);
}

// ==============================================================
//  LEDs & Lights
// ==============================================================
void setAllLightsOn(bool onvalue) { 
  uint16_t v = onvalue ? LED_ON : LED_OFF;
  gLEDDriver.setLED(0, v, v, v);
  gLEDDriver.setLED(1, v, v, v);
  gLEDDriver.setLED(2, v, v, 0);
  gLEDDriver.write();
}

// ---------------------------------------------------------------
void updateLEDs() {
  uint16_t leds[NUM_ELEMENTS];
  for (byte i=0; i < NUM_ELEMENTS; i++) {
    Element *e = &(gElements[i]);
    leds[(e->ledBaseChannel * 3) + e->ledSubChannel] = (e->ledState ? LED_ON : LED_OFF);

    if (0) {
      DPRINT("[updateLEDs] e: "); DPRINT(((e->ledBaseChannel * 3) + e->ledSubChannel)); 
      DPRINT(" function: "); DPRINT(e->function); 
      DPRINT(" state: "); DPRINT(e->state); 
      DPRINT(" ledState: "); DPRINTLN(e->ledState);       
    }
  }  

  gLEDDriver.setLED(0, leds[0], leds[1], leds[2]);
  gLEDDriver.setLED(1, leds[3], leds[4], leds[5]);
  gLEDDriver.setLED(2, leds[6], leds[7], 0);
  gLEDDriver.write();
}

// ---------------------------------------------------------------
void setUpButtons() {
  gLEDDriver.begin();
  setAllLightsOn(true);  delay(FLASH_DELAY);
  setAllLightsOn(false); delay(FLASH_DELAY);
  setAllLightsOn(true);  delay(FLASH_DELAY);
  setAllLightsOn(false); delay(FLASH_DELAY);
}


// ==============================================================
//  Functional States
// ==============================================================
void setStateForFunction(CANFunction function, bool state) {
  for (byte i=0; i < NUM_ELEMENTS; i++) {
    Element *e = &(gElements[i]);
    if (e->function == function) {
      if (1) {
        DPRINT("[setStateForFunction] e:"); DPRINT(i); 
        DPRINT(" function: "); DPRINT(e->function); 
        DPRINT(" state: "); DPRINTLN(state); 
      }
            
      e->state = state;
      e->ledState = e->state ? true : false;
      break;
    }
  }
}

// ---------------------------------------------------------------
bool getStateForFunction(CANFunction function) {
  for (int i=0 ; i<NUM_ELEMENTS ; i++) {
    Element *e = &(gElements[i]);
    if (e->function == function) {
      return e->state;
    }
  }
}

// ---------------------------------------------------------------
void setLEDStateForFunction(CANFunction function, bool state) {
  for (byte i=0; i < NUM_ELEMENTS; i++) {
    Element *e = &(gElements[i]);
    if (e->function == function) {
      e->ledState = state;
      break;
    }
  }
}

// ---------------------------------------------------------------
bool getLEDStateForFunction(CANFunction function) {
  for (int i=0 ; i<NUM_ELEMENTS ; i++) {
    Element *e = &(gElements[i]);
    if (e->function == function) {
      return e->ledState;
    }
  }
}

// ==============================================================
//  Setup & Loop
// ==============================================================
void setup() {
  setUpControlsBox(&gCanbus);
  setupTimer();
  setUpButtons();
}

// ---------------------------------------------------------------
void loop() {
  bool anyStateDidChange = false;
  
  for (byte i=0; i < NUM_ELEMENTS; i++) {
    Element *e = &(gElements[i]);
    bool isPressed = (switchesMux.read(e->switchChannel) == LOW);

    if (isPressed && 0) {
      DPRINT("element: "); DPRINT(i); 
      DPRINT(" function: "); DPRINT(e->function); 
      DPRINT(" switchChannel: "); DPRINT(e->switchChannel); 
      DPRINT(" is: "); DPRINTLN(isPressed ? "pressed" : "not pressed");
    }

    switch (e->action) {
      case kButtonActionMomentary: {
        if (e->state != isPressed) {
          setStateForFunction( e->function, isPressed);
          sendCANCommand(&gCanbus, kCANCommandSet, e->function, e->state);
          anyStateDidChange = true;
        }
        break;
      }

      case kButtonActionToggle: {
        if (isPressed) {
          if (e->isInTransition == false) {
            e->isInTransition = true;
            setStateForFunction(e->function, !e->state);
            sendCANCommand(&gCanbus, kCANCommandSet, e->function, e->state);
            anyStateDidChange = true;
          }
        }
        else {
            e->isInTransition = false;
        }
        break;
      }
    }

    // Turn off any related functions to the current state if it has changed
    if (anyStateDidChange && e->state) {
      //DPRINT("[loop] Resetting related for func "); DPRINTLN(e->function); DPRINTLN("");
      if (e->function == kCANFunctionHeadlightsHigh) {
        setStateForFunction( kCANFunctionHeadlightsLow, false);
      }
      else if (e->function == kCANFunctionHeadlightsLow) {
        setStateForFunction( kCANFunctionHeadlightsHigh, false);
      }
      else if (e->function == kCANFunctionHazards) {
        setStateForFunction( kCANFunctionTurnSignalLeft, false);
        setStateForFunction( kCANFunctionTurnSignalRight, false);
      }
      else if (e->function == kCANFunctionTurnSignalLeft) {
        setStateForFunction( kCANFunctionHazards, false);
        setStateForFunction( kCANFunctionTurnSignalRight, false);
      }
      else if (e->function == kCANFunctionTurnSignalRight) {
        setStateForFunction( kCANFunctionHazards, false);
        setStateForFunction( kCANFunctionTurnSignalLeft, false);
      }
    }
  }

  // Force an update to the lights
  if (anyStateDidChange) {
    updateLEDs();
  }

  timer.tick(); // tick the timer

  delay(100); // small delay to help with bounced buttons
}
