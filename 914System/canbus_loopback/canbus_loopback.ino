/*
 * Loopback example for CAN MCP2515.
 */
#include <SPI.h>
// https://github.com/coryjfowler/MCP_CAN_lib
#include <mcp_can.h>

#define INT A0 // grey
#define CS 10   // yellow

// Definition of SPI:
// SI -> 11  blue
// SO -> 12  green
// SCK -> 13 purple

MCP_CAN Canbus(CS);

void setup() {
  Serial.begin(115200);

  while(true) {
    delay(1000);
    if(Canbus.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
      Serial.println("MCP2515 Initialized Successfully.");
      break;
    }
    else {
      Serial.println("Error Initializing MCP2515...");
      delay(3 * 1000);
    }
  }

  Canbus.setMode(MCP_LOOPBACK);

  pinMode(INT, INPUT);

  Serial.println("setup() done");
}

// https://github.com/coryjfowler/MCP_CAN_lib/blob/master/examples/CAN_loopback/CAN_loopback.ino
unsigned long prevTX = 0;                                        // Variable to store last execution time
const unsigned int invlTX = 1000;                           // 10 second interval constant
byte data[] = {0xAA, 0x55, 0x01, 0x10, 0xFF, 0x12, 0x34, 0x56};  // Generic CAN data to send
long unsigned int rxId;
unsigned char len;
unsigned char rxBuf[8];
char msgString[128];

void loop()
{
  if(!digitalRead(INT)) // If CAN0_INT pin is low, read receive buffer
  {
    Serial.print(" < ");
    Canbus.readMsgBuf(&rxId, &len, rxBuf);            // Read data: len = data length, buf = data byte(s)
    
    if((rxId & 0x80000000) == 0x80000000)             // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
  
    Serial.print(msgString);
  
    if((rxId & 0x40000000) == 0x40000000){            // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      for(byte i = 0; i<len; i++){
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
      }
    }
        
    Serial.println();
  }
  
  if(millis() - prevTX >= invlTX){                    // Send this at a one second interval. 
    Serial.print(" > ");
    prevTX = millis();
    byte sndStat = Canbus.sendMsgBuf(0x100, 8, data);
    Serial.print(" > ");
    
    if(sndStat == CAN_OK) {
      Serial.println("Message Sent Successfully!");
    } else {
      Serial.print("Error ");
      Serial.print(sndStat);
      Serial.println(" Sending Message...");
    }
  }
}
