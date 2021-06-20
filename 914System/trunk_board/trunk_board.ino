#include <Arduino.h>
#include <914CAN.h>
#include <arduino-timer.h>

MCP_CAN gCanbus(CAN_CS_PIN);
Timer<1, millis> timer; // create a timer with 1 task and microsecond resolution

typedef struct {
  CANFunction function;
  byte state;
} Element;

Element gElements[] = {
  { kCANFunctionTurnSignalLeft,  0, },
  { kCANFunctionTurnSignalRight, 0, },
  { kCANFunctionReverseLights,   0, },
  { kCANFunctionHazards,         0, },
  { kCANFunctionRunningLights,   0, },
};
#define NUM_ELEMENTS 5

// ==============================================================
//  Timer
// ==============================================================
bool handleTimer(void *) {
  if (getStateForFunction(kCANFunctionHazards)) {
    digitalWrite(kTrunkRelayPinLeftTurnSignal, !digitalRead(kTrunkRelayPinLeftTurnSignal));    
    digitalWrite(kTrunkRelayPinRightTurnSignal, !digitalRead(kTrunkRelayPinRightTurnSignal));
  }
  else if (getStateForFunction(kCANFunctionTurnSignalLeft)) {
    digitalWrite(kTrunkRelayPinLeftTurnSignal, !digitalRead(kTrunkRelayPinLeftTurnSignal));    
  }
  else if (getStateForFunction(kCANFunctionTurnSignalRight)) {
    digitalWrite(kTrunkRelayPinRightTurnSignal, !digitalRead(kTrunkRelayPinRightTurnSignal));
  }
  
  return true; // repeat? true
}

// ---------------------------------------------------------------
void setupTimer() {
  // call the handleTimer function every 500 millis (1/2 second)
  timer.every(500, handleTimer);
}

// ==============================================================
//  States & Functions
// ==============================================================
void setStateForFunction(CANFunction function, byte value) {
  if (function == kCANFunctionHazards) {
    if (value == 1) {
      setStateForFunction(kCANFunctionTurnSignalLeft, 0);
      setStateForFunction(kCANFunctionTurnSignalRight, 0);
    }
    digitalWrite(kTrunkRelayPinRightTurnSignal, OFF);
    digitalWrite(kTrunkRelayPinLeftTurnSignal, OFF);
  }
  else if (function == kCANFunctionTurnSignalLeft) {
    if (value == 1){
      setStateForFunction(kCANFunctionHazards, 0);
      setStateForFunction(kCANFunctionTurnSignalRight, 0);
    }
    digitalWrite(kTrunkRelayPinRightTurnSignal, OFF);
    digitalWrite(kTrunkRelayPinLeftTurnSignal, OFF);
  }
  else if (function == kCANFunctionTurnSignalRight) {
    if (value == 1){
      setStateForFunction(kCANFunctionHazards, 0);
      setStateForFunction(kCANFunctionTurnSignalLeft, 0);
    }
    digitalWrite(kTrunkRelayPinRightTurnSignal, OFF);
    digitalWrite(kTrunkRelayPinLeftTurnSignal, OFF);
  }
  else if (function == kCANFunctionHeadlightsLow) {
    if (value) {
      setStateForFunction(kCANFunctionHeadlightsHigh, 0); 
      setStateForFunction(kCANFunctionRunningLights, 1);
    } else {
      setStateForFunction(kCANFunctionRunningLights, 0);
    }
  }
  else if (function == kCANFunctionHeadlightsHigh) {
    if (value) {
      setStateForFunction(kCANFunctionHeadlightsLow, 0); 
      setStateForFunction(kCANFunctionRunningLights, 1);
    } else {
      setStateForFunction(kCANFunctionRunningLights, 0);
    }
  }

  // Set the function value.
  for (int i=0 ; i<NUM_ELEMENTS ; i++) {
    Element *e = &(gElements[i]);
    if (e->function == function) {
      e->state = value;
      break;
    }
  }
}

// ---------------------------------------------------------------
byte getStateForFunction(CANFunction function) {
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
    byte pin = trunkRelayPinFromFunction(e->function);
    if (pin != kRelayPinNone){
      digitalWrite(pin, e->state ? ON : OFF);
    }    
  }
}

// ==============================================================
//  Setup & Loop
// ==============================================================
void setup() {
  setUpTrunkBox(&gCanbus);
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

  //delay(100);
  
  timer.tick(); // tick the timer
}
