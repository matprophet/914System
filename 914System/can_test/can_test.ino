#include <Arduino.h>
#include <914CAN.h>
#include <SoftwareSerial.h>
#include <arduino-timer.h>

// ==============================================================
//  Globals
// ==============================================================
MCP_CAN gCanbus(10);
Timer<1, millis> timerCANReporting; // create a timer with 1 task and millisecond resolution

bool handleCANReportingTimer(void *) {
  byte *canData = NULL;
  byte numBytes = canDataForOBD(&canData);
  byte sndStat = gCanbus.sendMsgBuf(k914ECUCanID, numBytes, canData);

  DPRINT("[handleCANReportingTimer] #bytes to send: ");
  DPRINT(numBytes);
  DPRINT(" - error: ");
  DPRINTLN(sndStat);
  if (canData) {
    free(canData);
  }
  
  return true;
}

// ---------------------------------------------------------------
void setUpTimers() {
  timerCANReporting.every(1000, handleCANReportingTimer);
}

// ---------------------------------------------------------------
void tickTimers() {
  timerCANReporting.tick();
}


// ---------------------------------------------------------------
uint8_t canDataForOBD(uint8_t **outData) {
  uint8_t numBytes = 7;
  uint8_t *canData = (uint8_t *)malloc(numBytes);
  int rpm = random(100, 3000);
  canData[0] = (rpm & 0xFF);  // rpm low
  canData[1] = ((rpm >> 8 ) & 0xFF);  // rpm high
  canData[2] = (random(0, 100) & 0xFF); // TPS
  canData[3] = (random(10, 50) & 0xFF); // Coolant
  canData[4] = (0 & 0xFF); // Engine Load
  canData[5] = (random(10, 50) & 0xFF); // Intake Temp
  canData[6] = (0 & 0xFF); // Intake MAP

  *outData = canData;

  return numBytes;
}

// ==============================================================
//  Setup & Loop
// ==============================================================
void setup() {
  setUpOBD2CANBox(&gCanbus);
  setUpTimers();
}

// ---------------------------------------------------------------
void loop() {
  tickTimers();
}
