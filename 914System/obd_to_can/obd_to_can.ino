#include <Arduino.h>
#include <914CAN.h>
#include <SoftwareSerial.h>
#include <arduino-timer.h>
#include "OBD9141.h"

#define KLINE_RX_PIN 7
#define KLINE_TX_PIN 6

// ==============================================================
//  Globals
// ==============================================================
MCP_CAN gCanbus(CAN_CS_PIN);
SoftwareSerial gKLine = SoftwareSerial(KLINE_RX_PIN, KLINE_TX_PIN); // Rx, Tx
OBD9141 gOBD;
Timer<1, millis> timerHighPriorityOBD; // create a timer with 1 task and millisecond resolution
Timer<1, millis> timerLowPriorityOBD; // create a timer with 1 task and millisecond resolution
Timer<1, millis> timerCANReporting; // create a timer with 1 task and millisecond resolution

// ---------------------------------------------------------------
// typedefs for the OBD entries.
typedef enum : byte {
  kOBDPriorityHigh = 1,
  kOBDPriorityLow
} OBDPriority;

// ---------------------------------------------------------------
typedef struct{
  uint8_t pid;
  uint8_t mode; // OBD Service
  OBDPriority priority; // 1 is highest
  uint8_t responseLength; // 1 or 2
  uint16_t lastResponse;
} OBDUnit;

#define NUM_OBD_PIDS 6
OBDUnit gOBDUnits[NUM_OBD_PIDS] = {
  {PID_RPM,          PID_MODE_LIVE_DATA, kOBDPriorityHigh, 2, 0},
  {PID_THROTTLE,     PID_MODE_LIVE_DATA, kOBDPriorityHigh, 1, 0},
  {PID_COOLANT_TEMP, PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},
  {PID_ENGINE_LOAD,  PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},
  {PID_INTAKE_TEMP,  PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},
  {PID_INTAKE_MAP,   PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},    
};

// ==============================================================
//  Timers
// ==============================================================
bool handleHighPriorityOBDTimer(void *) {
  updateOBDForPriority(kOBDPriorityHigh);
  return true;
}

// ---------------------------------------------------------------
bool handleLowPriorityOBDTimer(void *) {
  updateOBDForPriority(kOBDPriorityLow);
  return true;
}

// ---------------------------------------------------------------
bool handleCANReportingTimer(void *) {
  byte *canData = NULL;
  byte numBytes = canDataForOBD(&canData);
  byte sndStat = gCanbus.sendMsgBuf(k914ECUCanID, numBytes, canData);
  DPRINTLN(sndStat);
  if (canData) {
    free(canData);
  }

  return true;
}

// ---------------------------------------------------------------
void setUpTimers() {
  timerHighPriorityOBD.every(200, handleHighPriorityOBDTimer);
  timerLowPriorityOBD.every(2000, handleLowPriorityOBDTimer); // 1000 millis (1 second)
  timerCANReporting.every(200, handleCANReportingTimer);
}

// ---------------------------------------------------------------
void tickTimers() {
  timerHighPriorityOBD.tick();
  timerLowPriorityOBD.tick();
  timerCANReporting.tick();
}

// ==============================================================
//  OBD2
// ==============================================================
void setUpOBDUnit(int index, byte pid, byte mode, byte responseLength, OBDPriority priority) {
  gOBDUnits[index].pid = pid;
  gOBDUnits[index].mode = mode;
  gOBDUnits[index].responseLength = responseLength;
  gOBDUnits[index].priority = priority;
}

// ---------------------------------------------------------------
void getVIN() {
  DPRINTLN("[setUpOBD] Getting VIN...");
  if (gOBD.getPID(PID_VIN, 9, 17)) {
    DPRINT("VIN:");
    for (int i=0 ; i<17 ; i++){  
      DPRINT(gOBD.readUint8(i));
    }
    DPRINTLN("");
  }
}

// ---------------------------------------------------------------
void getSupportedPIDs() {
  uint8_t cmds[7] = {
    PID_SUPPORTED_PIDS_1_20,
    PID_SUPPORTED_PIDS_21_40,
    PID_SUPPORTED_PIDS_41_60,
    PID_SUPPORTED_PIDS_61_80,
    PID_SUPPORTED_PIDS_81_A0,
    PID_SUPPORTED_PIDS_A1_C0,
    PID_SUPPORTED_PIDS_C1_E0
  };

  byte mode = PID_MODE_LIVE_DATA;
 
  for (int cmd=0; cmd<7; cmd++) {      
    delay(30); // offical timeout
    if (gOBD.getPID(cmds[cmd], mode, 4)) {
      DPRINT("Mode "); DPRINT(mode); DPRINT(" CMD:0x"); DPRINT(cmds[cmd], HEX); DPRINT(": Supported PIDs:");  
      uint32_t pid = 1+(cmd * 20);
      for( byte b=0; b<4; b++){
        uint8_t val = gOBD.readUint8(b);
        //DPRINT(b); DPRINT(" - 0x"); DPRINT(val, HEX); DPRINT(" ");
        for (int i=7 ; i>=0 ; i--){
          uint8_t mask = (1 << i);                  
          //DPRINTLN(" "); DPRINT("mask: "); DPRINT(mask, HEX); DPRINT(" val: "); DPRINT(val, HEX); DPRINT(" &: "); DPRINT(val & mask);
          if ( (val & mask) != 0 ){
           DPRINT(" 0x"); DPRINT( pid, HEX ); 
          }
          pid++;
        }
      } 
      DPRINTLN(" ");    
    } else {
      DPRINT("Mode "); DPRINT(mode); DPRINT(": No supported PIDs for CMD:0x"); DPRINTLN(cmds[cmd], HEX); 
    }
  }
}

// ---------------------------------------------------------------
void setUpOBD() {
  DPRINTLN("[setUpOBD] Setting up OBD...");
  gOBD.begin(gKLine, KLINE_RX_PIN, KLINE_TX_PIN);

  // Initialize OBD-II adapter
  do {
    DPRINTLN("[setUpOBD] Connecting...");
  } while (!gOBD.init());
  DPRINTLN("[setUpOBD] Connected");

  getSupportedPIDs();
 
  DPRINTLN("[setUpOBD] Done.");
}

// ---------------------------------------------------------------
void updateOBDForPriority(OBDPriority priority){
#if 0
/*
  {PID_RPM,          PID_MODE_LIVE_DATA, kOBDPriorityHigh, 2, 0},
  {PID_THROTTLE,     PID_MODE_LIVE_DATA, kOBDPriorityHigh, 1, 0},
  {PID_COOLANT_TEMP, PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},
  {PID_ENGINE_LOAD,  PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},
  {PID_INTAKE_TEMP,  PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},
  {PID_INTAKE_MAP,   PID_MODE_LIVE_DATA, kOBDPriorityLow,  1, 0},   
*/
/*
  if (priority == kOBDPriorityHigh) {
      uint8_t pids[1] = {PID_THROTTLE};
      if(gOBD.getPIDs(pids, 1, PID_MODE_LIVE_DATA, 1)) {
         gOBDUnits[1].lastResponse = gOBD.readBuffer(5);
         DPRINT("[u] PID: 0x"); DPRINT(PID_THROTTLE, HEX); DPRINT(" : "); DPRINTLN(gOBDUnits[1].lastResponse);
      } else {
        DPRINTLN("[u] PIDs: high-p FAILED");
      }
  }
  */
  
  if (priority == kOBDPriorityHigh) {
      uint8_t pids[2] = {PID_RPM, PID_THROTTLE};
      if(gOBD.getPIDs(pids, 2, PID_MODE_LIVE_DATA, 3)) {
         gOBDUnits[0].lastResponse = gOBD.readBuffer(6)*256 + gOBD.readBuffer(7);
         gOBDUnits[1].lastResponse = gOBD.readBuffer(8);
         DPRINT("[u] PID: 0x"); DPRINT(PID_THROTTLE, HEX); DPRINT(" : "); DPRINTLN(gOBDUnits[1].lastResponse);
      } else {
        DPRINTLN("[u] PIDs: high-p FAILED");
      }
  } 
  
#else   
  for (int i=0; i<NUM_OBD_PIDS; i++){
    OBDUnit *unit = &gOBDUnits[i];
    if (unit->priority == priority) {
      if(gOBD.getPID(unit->pid, unit->mode, unit->responseLength)) {
        if (unit->responseLength == 1){
          unit->lastResponse = gOBD.readUint8();
        } else {
          unit->lastResponse = gOBD.readUint16();
        }

        DPRINT("[u] PID: 0x"); DPRINT(unit->pid, HEX); DPRINT(" : "); DPRINTLN(unit->lastResponse);
      }
      else {
        DPRINT("[u] PID: 0x"); DPRINT(unit->pid, HEX); DPRINTLN(" FAILED");
      }
      delay(60);
    }
  }
#endif
}

// ---------------------------------------------------------------
uint8_t canDataForOBD(uint8_t **outData){
  uint8_t numBytes = 0;
  for (int i=0; i<NUM_OBD_PIDS; i++){
    numBytes += gOBDUnits[i].responseLength;
  }

  uint8_t *canData = (uint8_t *)malloc(numBytes);
  canData[0] = (gOBDUnits[0].lastResponse & 0xFF);  // rpm low
  canData[1] = ((gOBDUnits[0].lastResponse >> 8 ) & 0xFF);  // rpm high
  canData[2] = (gOBDUnits[1].lastResponse & 0xFF); // TPS
  canData[3] = (gOBDUnits[2].lastResponse & 0xFF); // Coolant
  canData[4] = (gOBDUnits[3].lastResponse & 0xFF); // Engine Load
  canData[5] = (gOBDUnits[4].lastResponse & 0xFF); // Intake Temp
  canData[6] = (gOBDUnits[5].lastResponse & 0xFF); // Intake MAP

  *outData = canData;

  return numBytes;
}

// ==============================================================
//  Setup & Loop
// ==============================================================
void setup() {
  setUpOBD2CANBox(&gCanbus);
  setUpTimers();
  setUpOBD();
}

// ---------------------------------------------------------------
void loop() {
  tickTimers();
}
