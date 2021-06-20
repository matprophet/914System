#ifndef _914CAN_H
#define _914CAN_H
#include <Arduino.h>
#include <mcp_can.h>

//#define DEBUGLOGGING
#ifdef DEBUGLOGGING
#include <SoftwareSerial.h>
#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINTLN(...)
#endif

#define ON  LOW
#define OFF HIGH
#define CAN_INT_PIN A0
#define CAN_CS_PIN  10

extern const uint32_t k914SerialSpeed;
extern const uint32_t k914CanBitrate;
extern const unsigned long k914BroadcastCanID;
extern const unsigned long k914FrunkCanID;
extern const unsigned long k914TrunkCanID;
extern const unsigned long k914MainCanID;
extern const unsigned long k914ControlBoxCanID;
extern const unsigned long k914ECUCanID;

#pragma mark - Enums
/*
 CAN[0] Command
 CAN[1] Field
 CAN[2] Value
 */
typedef enum : byte {
    kCANFieldCommand = 0x1,
    kCANFieldFunction,
    kCANFieldValue
} CANField;

typedef enum : byte {
    kCANCommandSet = 0x1,
    kCANCommandGet,
    kCANResponse
} CANCommand;

typedef enum : byte {
    kCANFunctionStarter = 0x1, // 1
    kCANFunctionFans, // 2
    kCANFunctionFuelPumps, // 3
    kCANFunctionHeadlightsLow, // 4
    kCANFunctionHeadlightsHigh, // 5
    kCANFunctionHazards, // 6
    kCANFunctionRunningLights,  // 7
    kCANFunctionReverseLights, // 8
    kCANFunctionTurnSignalLeft, // 9
    kCANFunctionTurnSignalRight, // 10
    kCANFunctionUnused,
} CANFunction;

typedef byte RelayPin;
extern const RelayPin kRelayPinNone;

typedef enum : RelayPin {
    kFrunkRelayPinMarkerLights = 2,
    kFrunkRelayPinRightTurnSignal = 3,
    kFrunkRelayPinFans = 4,
    kFrunkRelayPinUnusedX = 5,
    kFrunkRelayPinLowBeams = 6,
    kFrunkRelayPinHighBeams = 7,
    kFrunkRelayPinFuelPumps = 8,
    kFrunkRelayPinLeftTurnSignal = 9,
    kFrunkRelayPinUnused = 10
} FrunkRelayPin;

typedef enum : RelayPin {
    kTrunkRelayPinLeftTurnSignal = 2,
    kTrunkRelayPinReverseLights,
    kTrunkRelayPinMarkerLights,
    kTrunkRelayPinRightTurnSignal,
    kTrunkRelayPinUnused
} TrunkRelayPin;

typedef enum : RelayPin {
    kMainRelayPinStarter = 5,
    kMainRelayPinUnused
} MainRelayPin;

#pragma mark - Public functions

void setUpFrunkBox(MCP_CAN *canbus);
void setUpTrunkBox(MCP_CAN *canbus);
void setUpMainBox(MCP_CAN *canbus);
void setUpControlsBox(MCP_CAN *canbus);
void setUpOBD2CANBox(MCP_CAN *canbus);

bool hasPendingCANPacket();
uint8_t sendCANCommand(MCP_CAN *canbus, CANCommand command, CANFunction function, byte value);

RelayPin frunkRelayPinFromFunction(CANFunction function);
RelayPin trunkRelayPinFromFunction(CANFunction function);
RelayPin mainRelayPinFromFunction(CANFunction function);

#endif
