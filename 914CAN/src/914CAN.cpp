#include <914CAN.h>
#include <SoftwareSerial.h>
#include <mcp_can.h>

// ==============================================================
// Constants
// ==============================================================
const uint32_t k914SerialSpeed = 115200;
const unsigned long k914BroadcastCanID = 0x7;
const unsigned long k914FrunkCanID = 0x8;
const unsigned long k914TrunkCanID = 0x9;
const unsigned long k914MainCanID = 0xA;
const unsigned long k914ControlBoxCanID = 0xB;
const unsigned long k914ECUCanID = 601;
const RelayPin kRelayPinNone = 99;

// ==============================================================
// Forward Declarations
// ==============================================================
void setUpCanBus(MCP_CAN *mpccan, INT8U moduleClockSpeed);

#pragma mark - Relay Setup
// ==============================================================
//  Relay Setup
// ==============================================================
void setUpFrunkRelays() {
    DPRINTLN("[setUpRelays] Setting up...");
    for (byte i = kFrunkRelayPinMarkerLights; i <= kFrunkRelayPinUnused; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, HIGH);
    }
    DPRINTLN("");
}

// ---------------------------------------------------------------
void setUpTrunkRelays() {
    DPRINTLN("[setUpRelays] Setting up...");
    for (byte i = kTrunkRelayPinLeftTurnSignal; i <= kTrunkRelayPinUnused; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, HIGH);
    }
    DPRINTLN("");
}

// ---------------------------------------------------------------
void setUpMainRelays() {
    DPRINTLN("[setUpRelays] Setting up...");
    for (byte i = kMainRelayPinStarter; i <= kMainRelayPinUnused; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
    }
    DPRINTLN("");
}

#pragma mark - Relay Pin Access
// ==============================================================
//  Relay Pin Access
// ==============================================================
RelayPin frunkRelayPinFromFunction(CANFunction function) {
    switch(function) {
        case kCANFunctionFans:            return kFrunkRelayPinFans;
        case kCANFunctionFuelPumps:       return kFrunkRelayPinFuelPumps;
        case kCANFunctionTurnSignalLeft:  return kFrunkRelayPinLeftTurnSignal;
        case kCANFunctionTurnSignalRight: return kFrunkRelayPinRightTurnSignal;
        case kCANFunctionRunningLights:   return kFrunkRelayPinMarkerLights;
        case kCANFunctionHeadlightsLow:   return kFrunkRelayPinLowBeams;
        case kCANFunctionHeadlightsHigh:  return kFrunkRelayPinHighBeams;
    }
    return kRelayPinNone;
}

// ---------------------------------------------------------------
RelayPin trunkRelayPinFromFunction(CANFunction function) {
    switch(function) {
        case kCANFunctionTurnSignalLeft:  return kTrunkRelayPinLeftTurnSignal;
        case kCANFunctionTurnSignalRight: return kTrunkRelayPinRightTurnSignal;
        case kCANFunctionRunningLights:   return kTrunkRelayPinMarkerLights;
        case kCANFunctionReverseLights:   return kTrunkRelayPinReverseLights;
    }
    return kRelayPinNone;
}

// ---------------------------------------------------------------
RelayPin mainRelayPinFromFunction(CANFunction function) {
    switch(function) {
        case kCANFunctionStarter:  return kMainRelayPinStarter;
    }
    return kRelayPinNone;
}

#pragma mark - Setup Functions
// ==============================================================
//  Setup Functions
// ==============================================================
void setUpFrunkBox(MCP_CAN *canbus) {
    Serial.begin(k914SerialSpeed);
    setUpFrunkRelays();
    setUpCanBus(canbus, MCP_8MHZ);
    DPRINTLN("[setUpFrunkBox] finished."); DPRINTLN("");
}

// ---------------------------------------------------------------
void setUpTrunkBox(MCP_CAN *canbus) {
    Serial.begin(k914SerialSpeed);
    setUpTrunkRelays();
    setUpCanBus(canbus, MCP_8MHZ);
    DPRINTLN("[setUpTrunkBox] finished."); DPRINTLN("");
}

// ---------------------------------------------------------------
void setUpMainBox(MCP_CAN *canbus) {
    Serial.begin(k914SerialSpeed);
    setUpMainRelays();
    setUpCanBus(canbus, MCP_8MHZ);
    DPRINTLN("[setUpMainBox] finished."); DPRINTLN("");
}

// ---------------------------------------------------------------
void setUpControlsBox(MCP_CAN *canbus) {
    Serial.begin(k914SerialSpeed);
    setUpCanBus(canbus, MCP_8MHZ);
    DPRINTLN("[setUpControlsBox] finished."); DPRINTLN("");
}

// ---------------------------------------------------------------
void setUpOBD2CANBox(MCP_CAN *canbus) {
    Serial.begin(k914SerialSpeed);
    setUpCanBus(canbus, MCP_16MHZ);
    DPRINTLN("[setUpOBD2CANBox] finished."); DPRINTLN("");
}


#pragma mark - CanBus Functions
// ==============================================================
//  CAN Bus Functions
// ==============================================================
void setUpCanBus(MCP_CAN *mpccan, INT8U moduleClockSpeed) {
    DPRINTLN("[setUpCanBus] Setting up...");
    byte stat;
    pinMode(CAN_INT_PIN, INPUT);
    
    while(true) {
        delay(200);
        stat = mpccan->begin(MCP_ANY, CAN_500KBPS, moduleClockSpeed);
        DPRINT("  begin() status: "); DPRINTLN(stat);
        if(stat == CAN_OK) {
            break;
        }
        else {
            delay(1000);
        }
    }
    mpccan->setMode(MCP_NORMAL);

    //stat = mpccan->init_Filt(0, k914BroadcastCanID);
    //DPRINT(" filter 0 status: "); DPRINTLN(stat);

    DPRINTLN("");
}

// ---------------------------------------------------------------
bool hasPendingCANPacket() {
    return !digitalRead(CAN_INT_PIN);
}

// ---------------------------------------------------------------
uint8_t sendCANCommand(MCP_CAN *canbus, CANCommand command, CANFunction function, byte value) {
    byte can_data[8] = {0}; //assign an array for data
    can_data[kCANFieldCommand]  = command;
    can_data[kCANFieldFunction] = function;
    can_data[kCANFieldValue]    = value;
    
    byte sndStat = canbus->sendMsgBuf(k914BroadcastCanID, 4, can_data);
    DPRINT("[sendCANCommand] ");
    DPRINT(" command: "); DPRINT(command);
    DPRINT(" - function: "); DPRINT(function);
    DPRINT(" - value: "); DPRINT(value);
    DPRINT(" - status: "); DPRINTLN(sndStat);
    
    byte err = canbus->checkError();
    DPRINT("[sendCANCommand] checkError: "); DPRINTLN(err);
    if(err == CAN_CTRLERROR){
        DPRINT("  Error register value: ");
        byte tempErr = canbus->getError() & MCP_EFLG_ERRORMASK; // We are only interested in errors, not warnings.
        DPRINTLN(tempErr, BIN);
        
        DPRINT("  Transmit error counter register value: ");
        tempErr = canbus->errorCountTX();
        DPRINTLN(tempErr, DEC);
        
        DPRINT("  Receive error counter register value: ");
        tempErr = canbus->errorCountRX();
        DPRINTLN(tempErr, DEC);
    }
    DPRINTLN("");
}


