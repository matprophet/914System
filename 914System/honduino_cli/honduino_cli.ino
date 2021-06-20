/*
  Author:
  - Philip Bordado (kerpz@yahoo.com)
  Software:
  - Arduino 1.0.5
  - SoftwareSerialWithHalfDuplex
   https://github.com/nickstedman/SoftwareSerialWithHalfDuplex

  Formula:
  - IMAP = RPM * MAP / IAT / 2
  - MAF = (IMAP/60)*(VE/100)*(Eng Disp)*(MMA)/(R)
   Where: VE = 80% (Volumetric Efficiency), R = 8.314 J/°K/mole, MMA = 28.97 g/mole (Molecular mass of air)
   http://www.lightner.net/obd2guru/IMAP_AFcalc.html
   http://www.installuniversity.com/install_university/installu_articles/volumetric_efficiency/ve_computation_9.012000.htm
*/
#include <HondaCAN.h>
#include "lilParser.h"

SoftwareSerial kline = SoftwareSerial(7, 6); // Rx, Tx

lilParser cmdParser;
enum commands {   noCommand,  // ALWAYS start with noCommand. Or something simlar.
                  reset_ecu,
                  dtc_clear,
                  dtc_cnt,
                  fuel_status,
                  engine_load,
                  engine_temp,
                  ft_short,
                  ft_long,
                  map_sensor,
                  rpm,
                  vss,
                  timing_adv,
                  intake_temp,
                  throttle_position,
                  oxygen_sensor,
                  baro_kpa,
                  ecu_voltage,
                  iacv
              };          // Our list of commands.

// ==============================================================
//  Setup & Loop
// ==============================================================
void setup() {
  DPRINTLN("Connecting to Serial...");
  Serial.begin(115200);
  while (!Serial) {
    delay(50);
  }

  cmdParser.addCmd(reset_ecu, "reset_ecu");
  cmdParser.addCmd(dtc_clear, "dtc_clear");
  cmdParser.addCmd(dtc_cnt, "dtc_cnt");
  cmdParser.addCmd(fuel_status, "fuel_status");
  cmdParser.addCmd(engine_load, "engine_load");
  cmdParser.addCmd(engine_temp, "engine_temp");
  cmdParser.addCmd(ft_short, "ft_short");
  cmdParser.addCmd(ft_long, "ft_long");
  cmdParser.addCmd(map_sensor, "map_sensor");
  cmdParser.addCmd(rpm, "rpm");
  cmdParser.addCmd(ecu_voltage, "ecu_voltage");
/*
  cmdParser.addCmd(vss, "vss");
  cmdParser.addCmd(timing_adv, "timing_adv");
  cmdParser.addCmd(intake_temp, "intake_temp");
  cmdParser.addCmd(throttle_position, "throttle_position");
  cmdParser.addCmd(oxygen_sensor, "oxygen_sensor");
  cmdParser.addCmd(baro_kpa, "baro_kpa");
  cmdParser.addCmd(iacv, "iacv");
*/
  DPRINTLN("Initializing K-Line serial port...");
  
  kline.begin(HD_BUS_SPEED);
  do {
    delay(50);
  } while (!kline);
  
  DPRINTLN("SSM Serial Line Established.");
}

// -----------------------------------------------
HDPacket *sendShit(HDECUFeatureID feature) {
  HDECUFeatureID featureIds[1];
  featureIds[0] = feature;
  
  HDPacket *packet = packetForFeatureReads(featureIds, 1);
  sendRequestWithPacket(packet, kline);
  if (readResponseForPacket(packet, kline)) {
    DPRINTLN("[sendShit] response packet:");
    logPacket(packet);
  } else {
    DPRINTLN("DATA ERROR");
    freePacket(packet);
    packet = NULL;
  }

  return packet;
}

// -----------------------------------------------
//
// -----------------------------------------------
void loop() {
  char  inChar;
  int   command;
  byte  responseData[20];  // dlc data buffer

  if (Serial.available()) {
    inChar = Serial.read();
    command = cmdParser.addChar(inChar);
    switch (command) {
      case reset_ecu: {
        HDInit(kline);
        DPRINTLN("ECU Init: Done");
        break;
      }
      case dtc_clear: {
        if (HDPacket *response = sendShit(kHDECUFeatureDTCClear)) {
          DPRINTLN("DTC Clear Success");
          freePacket(response);
        } 
        break;
      }
      case dtc_cnt: {
        if (HDPacket *response = sendShit(kHDECUFeatureDTCCount)) {
          byte a = ((response->responseData[0] >> 5) & 1) << 7; // get bit 5 on responseData[2], set it to a7
          DPRINT("DTC Count: "); DPRINT(response->responseData[0]); DPRINT(" "); DPRINTLN(a);
          freePacket(response);
        }
        break;
      }
      case fuel_status: {
        if (HDPacket *response = sendShit(kHDECUFeatureFuelStatus)) {
          DPRINT("Fuel status: "); DPRINT(response->responseData[0]); DPRINT(" "); DPRINTLN(response->responseData[1]);
          freePacket(response);
        }
        break;
      }
      
      case engine_load: {
        if (HDPacket *response = sendShit(kHDECUFeatureEngineLoad)) {
          DPRINT("Engine load: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case engine_temp: {
        if (HDPacket *response = sendShit(kHDECUFeatureEngineTemp)) {
          float f = response->responseData[0];
          f = 155.04149 - f * 3.0414878 + pow(f, 2) * 0.03952185 - pow(f, 3) * 0.00029383913 + pow(f, 4) * 0.0000010792568 - pow(f, 5) * 0.0000000015618437;
          //responseData[2] = round(f) + 40; // A-40
          //sprintf_P(btdata2, PSTR("41 05 %02X\r\n>"), responseData[2]);
          DPRINT("Engine temp (f): "); DPRINTLN(f);          
          freePacket(response);
        }      
        break;
      }
      case ft_short: {
        if (HDPacket *response = sendShit(kHDECUFeatureFTShort)) {
          DPRINT("FT Short: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case ft_long: {
        if (HDPacket *response = sendShit(kHDECUFeatureFTLong)) {
          DPRINT("FT Long: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case map_sensor: {
        if (HDPacket *response = sendShit(kHDECUFeatureMapSensor)) {
          DPRINT("MAP Sensor: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case rpm: {
        if (HDPacket *response = sendShit(kHDECUFeatureMapSensor)) {
          DPRINT("RPM: "); DPRINT(response->responseData[0]); DPRINT(" "); DPRINTLN(response->responseData[1]);
          freePacket(response);
        }
        break;
      }
      case vss: {
        if (HDPacket *response = sendShit(kHDECUFeatureVSS)) {
          DPRINT("VSS: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case timing_adv: { // Degrees
        if (HDPacket *response = sendShit(kHDECUFeatureTimingAdv)) {
          DPRINT("Timing Adv Degrees: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case intake_temp: { // C
        if (HDPacket *response = sendShit(kHDECUFeatureIAT)) {
          float f = response->responseData[0];
          f = 155.04149 - f * 3.0414878 + pow(f, 2) * 0.03952185 - pow(f, 3) * 0.00029383913 + pow(f, 4) * 0.0000010792568 - pow(f, 5) * 0.0000000015618437;
          DPRINT("IAT(f): "); DPRINTLN(f);
          freePacket(response);
        }
        break;
      }
      case throttle_position: { // %
        if (HDPacket *response = sendShit(kHDECUFeatureThrottlePos)) {
          DPRINT("Throttle Pos %: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case oxygen_sensor: {  // V
        if (HDPacket *response = sendShit(kHDECUFeatureO2Sensor)) {
          DPRINT("O2 Sensor (v): "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case baro_kpa: { // kPa
        if (HDPacket *response = sendShit(kHDECUFeatureBaroKPa)) {
          DPRINT("Barometer (kPa): "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case ecu_voltage: {
        if (HDPacket *response = sendShit(kHDECUFeatureECUVoltage)) {
          float f = response->responseData[0];
          f = f / 10.45;
          unsigned int u = f * 1000; // ((A*256)+B)/1000
          //sprintf_P(btdata2, PSTR("41 42 %02X %02X\r\n>"), highByte(u), lowByte(u));
          DPRINT("ECU Voltage (v): "); DPRINT(response->responseData[0]); DPRINT(" h:"); DPRINT(highByte(u)); DPRINT(" l:"); DPRINTLN(lowByte(u)); 
          freePacket(response);
        }
        break;
      }
      case iacv: {
        if (HDPacket *response = sendShit(kHDECUFeatureIACV)) {
          DPRINT("IACV: "); DPRINTLN(response->responseData[0]);
          freePacket(response);
        }
        break;
      }
      case noCommand:
      default:
        DPRINTLN("???");
        break;
    }
  }
}
