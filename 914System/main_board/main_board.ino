#include <Arduino.h>
#include <914CAN.h>
#include <arduino-timer.h>

#define COOLANT_TEMP_SWITCH_PIN 9
#define VSS_OUTPUT_PIN 16
#define STARTER_STOP_TIMEOUT_MS 500 

// ==============================================================
//  Globals
// ==============================================================
MCP_CAN gCanbus(10); //CAN_CS_PIN
Timer<1, millis> timerCoolantSwitch; // create a timer with 1 task and millisecond resolution
Timer<1, micros> timerVSS; // create a timer with 1 task and millisecond resolution
Timer<1, millis> timerStopStarter;
bool vssPinHigh;

// ---------------------------------------------------------------
typedef struct {
  CANFunction function;
  bool state;
} Element;

#define NUM_ELEMENTS 2
Element gElements[NUM_ELEMENTS] = {
  { kCANFunctionStarter, 0, },
  { kCANFunctionFans,    0, }
};

// ==============================================================
//  Timers
// ==============================================================
bool handleCoolantSwitchTimer(void *) {
  int val = digitalRead(COOLANT_TEMP_SWITCH_PIN);
  if (setStateForFunction(kCANFunctionFans, val == LOW)) {
    DPRINT("coolant switch: "); DPRINTLN(val == LOW);
  }
  return true;
}

// ---------------------------------------------------------------
bool handleVSSTimer(void *) {
  vssPinHigh = !vssPinHigh;
  digitalWrite(VSS_OUTPUT_PIN, vssPinHigh ? HIGH : LOW);
  return true;
}

// ---------------------------------------------------------------
bool handleStopStarterTimer(void *){
  setStateForFunction(kCANFunctionStarter, LOW);
  updateRelaysFromStates();
  return false;
}

// ---------------------------------------------------------------
void setUpTimers() {
  timerCoolantSwitch.every(1000, handleCoolantSwitchTimer); // 1000 msec (1 second)
  timerVSS.every(500, handleVSSTimer); // 500 usec
}

// ---------------------------------------------------------------
void tickTimers() {
  timerCoolantSwitch.tick();
  timerVSS.tick();
  timerStopStarter.tick();
}

// ==============================================================
//  Relay States
// ==============================================================
bool setStateForFunction(CANFunction function, bool state) {
  // Set the function value.
  bool res = false;
  for (int i=0 ; i<NUM_ELEMENTS ; i++) {
    Element *e = &(gElements[i]);
    if (e->function == function) {
      res = (e->state != state);
      e->state = state;
      return res;
    }
  }
  return res;
}

// ---------------------------------------------------------------
void updateRelaysFromStates() {
  for (int i=0 ; i<NUM_ELEMENTS ; i++) {
    Element *e = &(gElements[i]);
    DPRINT("[updateRelaysFromStates] function: "); DPRINT(e->function); DPRINT(" state: "); DPRINTLN(e->state);    
    digitalWrite(mainRelayPinFromFunction(e->function), e->state ? HIGH : LOW);
  }
}

// ---------------------------------------------------------------
void setUpPins() {
  pinMode(COOLANT_TEMP_SWITCH_PIN, INPUT); // coolant temp switch
  pinMode(VSS_OUTPUT_PIN, OUTPUT);
}

// ==============================================================
//  CAN
// ==============================================================
void handlePendingCANPackets() {
  if(hasPendingCANPacket() == false) {
    return;
  }
  
  unsigned long can_ID;
  byte can_length;
  byte can_data[8] = {0};
  byte stat = gCanbus.readMsgBuf(&can_ID, &can_length, can_data);
  DPRINT("[readMsgBuf] can_ID: "); DPRINT(can_ID); DPRINT(" - status: "); DPRINTLN(stat);

  if (can_ID == k914BroadcastCanID) {
    switch(can_data[kCANFieldCommand]) {
      case kCANCommandSet: {
        setStateForFunction(can_data[kCANFieldFunction], can_data[kCANFieldValue]);
        if (can_data[kCANFieldFunction] == kCANFunctionStarter) {
          timerStopStarter.cancel();
          timerStopStarter.in(STARTER_STOP_TIMEOUT_MS, handleStopStarterTimer);
        }
        updateRelaysFromStates();
        break;
      }
      case kCANCommandGet:
      case kCANResponse: {
        break;
      }
    }
  }
}

// ==============================================================
//  Setup & Loop
// ==============================================================
void setup() {
  setUpMainBox(&gCanbus);
  setUpPins();
  setUpTimers();
}

// ---------------------------------------------------------------
void loop() {
  handlePendingCANPackets();
  tickTimers();
}
