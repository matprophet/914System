#include <Arduino.h>
#include <914CAN.h>
#include <arduino-timer.h>

MCP_CAN gCanbus(CAN_CS_PIN);
Timer<1, millis> timer; // create a timer with 1 task and microsecond resolution

typedef struct {
  CANFunction function;
  bool state;
} Element;

Element gElements[] = {
  { kCANFunctionFans,            false, },
  { kCANFunctionFuelPumps,       false, },
  { kCANFunctionHeadlightsLow,   false, },
  { kCANFunctionHeadlightsHigh,  false, },
  { kCANFunctionHazards,         false, },
  { kCANFunctionRunningLights,   false, },
  { kCANFunctionTurnSignalLeft,  false, },
  { kCANFunctionTurnSignalRight, false, },
};
#define NUM_ELEMENTS 8

// ==============================================================
//  Timer
// ==============================================================
bool handleTimer(void *) {  
  if (getStateForFunction(kCANFunctionHazards)) {
    digitalWrite(kFrunkRelayPinLeftTurnSignal, !digitalRead(kFrunkRelayPinLeftTurnSignal));    
    digitalWrite(kFrunkRelayPinRightTurnSignal, !digitalRead(kFrunkRelayPinRightTurnSignal));
  }
  else if (getStateForFunction(kCANFunctionTurnSignalLeft)) {
    digitalWrite(kFrunkRelayPinLeftTurnSignal, !digitalRead(kFrunkRelayPinLeftTurnSignal));    
  }
  else if (getStateForFunction(kCANFunctionTurnSignalRight)) {
    digitalWrite(kFrunkRelayPinRightTurnSignal, !digitalRead(kFrunkRelayPinRightTurnSignal));
  }
  
  return true; // repeat? true
}

// ---------------------------------------------------------------
void setupTimer() {
  // call the handleTimer function every 500 millis (1/2 second)
  timer.every(500, handleTimer);
}


// ==============================================================
//  Relay States
// ==============================================================
void setStateForFunction(CANFunction function, bool state) {
  if (function == kCANFunctionHazards) {
    if (state) {
      setStateForFunction(kCANFunctionTurnSignalLeft, false);
      setStateForFunction(kCANFunctionTurnSignalRight, false);
    }
    digitalWrite(kFrunkRelayPinRightTurnSignal, OFF);
    digitalWrite(kFrunkRelayPinLeftTurnSignal, OFF);
  }
  else if (function == kCANFunctionTurnSignalLeft) {
    if (state){
      setStateForFunction(kCANFunctionHazards, false);
      setStateForFunction(kCANFunctionTurnSignalRight, false);
    }
    digitalWrite(kFrunkRelayPinRightTurnSignal, OFF);
    digitalWrite(kFrunkRelayPinLeftTurnSignal, OFF);
  }
  else if (function == kCANFunctionTurnSignalRight) {
    if (state){
      setStateForFunction(kCANFunctionHazards, false);
      setStateForFunction(kCANFunctionTurnSignalLeft, false);
    }
    digitalWrite(kFrunkRelayPinRightTurnSignal, OFF);
    digitalWrite(kFrunkRelayPinLeftTurnSignal, OFF);
  }
  else if (function == kCANFunctionHeadlightsLow) {
    if (state) {
      setStateForFunction(kCANFunctionHeadlightsHigh, false); 
      setStateForFunction(kCANFunctionRunningLights, true);
    } else {
      setStateForFunction(kCANFunctionRunningLights, false);
    }
  }
  else if (function == kCANFunctionHeadlightsHigh) {
    if (state) {
      setStateForFunction(kCANFunctionHeadlightsLow, false); 
      setStateForFunction(kCANFunctionRunningLights, true);
    } else {
      setStateForFunction(kCANFunctionRunningLights, false);
    }
  }

  // Set the function value.
  for (int i=0 ; i<NUM_ELEMENTS ; i++) {
    Element *e = &(gElements[i]);
    if (e->function == function) {
      e->state = state;
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
void updateRelaysFromStates() {
  for (int i=0 ; i<NUM_ELEMENTS ; i++) {
    Element *e = &(gElements[i]);

    byte pin = frunkRelayPinFromFunction(e->function);
    if (pin != kRelayPinNone){
      digitalWrite(pin, e->state ? ON : OFF);
    }

    DPRINT("[updateRelaysFromStates] function: "); DPRINT(e->function); 
    DPRINT(" state: "); DPRINT(e->state); 
    DPRINT(" pin: "); DPRINTLN(pin);
  }
}

// ==============================================================
//  Setup & Loop
// ==============================================================
void setup() {
  setUpFrunkBox(&gCanbus);
  setupTimer();
}

// ---------------------------------------------------------------
void loop() {
  if(hasPendingCANPacket()) {
    unsigned long can_ID;
    byte can_length;
    byte can_data[8] = {0};
    byte stat = gCanbus.readMsgBuf(&can_ID, &can_length, can_data);
    DPRINT("[readMsgBuf] can_ID: "); DPRINT(can_ID); DPRINT(" - status: "); DPRINTLN(stat);

    if (can_ID == k914BroadcastCanID) {
      switch(can_data[kCANFieldCommand]) {
        case kCANCommandSet: {
          setStateForFunction(can_data[kCANFieldFunction], can_data[kCANFieldValue]);
          updateRelaysFromStates();
          break;
        }

        case kCANCommandGet: {
          break;
        }

        case kCANResponse: {
          // Ignore
          break;
        }
      }
    }
  }

  delay(100); // small delay to help wtih intermittent resets

  timer.tick(); // tick the timer
}
