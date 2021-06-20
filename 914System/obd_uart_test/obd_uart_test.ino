/*************************************************************************
* Testing sketch for Freematics OBD-II UART Adapter V1/V2/V2.1
* Performs AT command-set test
* Reads and prints several OBD-II PIDs value
* Reads and prints motion sensor data if available
* Distributed under BSD
* Visit https://freematics.com/products for more product information
* Written by Stanley Huang <stanley@freematics.com.au>
*************************************************************************/

#include <OBD2UART.h>

COBD obd;
bool hasMEMS;

void testATcommands()
{
    static const char cmds[][6] = {"ATZ\r", "ATI\r", "ATH0\r", "ATRV\r", "0100\r", "010C\r", "0902\r"};
    char buf[128];

    for (byte i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        const char *cmd = cmds[i];
        Serial.print("Sending ");
        Serial.println(cmd);
        if (obd.sendCommand(cmd, buf, sizeof(buf))) {
            char *p = strstr(buf, cmd);
            if (p)
                p += strlen(cmd);
            else
                p = buf;
            while (*p == '\r') p++;
            while (*p) {
                Serial.write(*p);
                if (*p == '\r' && *(p + 1) != '\r')
                    Serial.write('\n');
                p++;
            }
            Serial.println();
        } else {
            Serial.println("Timeout");
        }
        delay(1000);
    }
    Serial.println();
}

void readPIDSingle()
{
    int value;
    Serial.print('[');
    Serial.print(millis());
    Serial.print(']');
    Serial.print("RPM=");
    if (obd.readPID(PID_RPM, value)) {
      Serial.print(value);
    }
    Serial.println();
}

void readPIDMultiple()
{
    static const byte pids[] = {PID_SPEED, PID_ENGINE_LOAD, PID_THROTTLE, PID_COOLANT_TEMP};
    int values[sizeof(pids)];
    if (obd.readPID(pids, sizeof(pids), values) == sizeof(pids)) {
      Serial.print('[');
      Serial.print(millis());
      Serial.print(']');
      for (byte i = 0; i < sizeof(pids) ; i++) {
        Serial.print((int)pids[i] | 0x100, HEX);
        Serial.print('=');
        Serial.print(values[i]);
        Serial.print(' ');
       }
       Serial.println();
    }
}

void readBatteryVoltage()
{
  Serial.print('[');
  Serial.print(millis());
  Serial.print(']');
  Serial.print("Battery:");
  Serial.print(obd.getVoltage(), 1);
  Serial.println('V');
}

void readMEMS()
{
  int16_t acc[3] = {0};
  int16_t gyro[3] = {0};
  int16_t mag[3] = {0};
  int16_t temp = 0;

  if (!obd.memsRead(acc, gyro, mag, &temp)) return;

  Serial.print('[');
  Serial.print(millis());
  Serial.print(']');

  Serial.print("ACC:");
  Serial.print(acc[0]);
  Serial.print('/');
  Serial.print(acc[1]);
  Serial.print('/');
  Serial.print(acc[2]);

  Serial.print(" GYRO:");
  Serial.print(gyro[0]);
  Serial.print('/');
  Serial.print(gyro[1]);
  Serial.print('/');
  Serial.print(gyro[2]);

  Serial.print(" MAG:");
  Serial.print(mag[0]);
  Serial.print('/');
  Serial.print(mag[1]);
  Serial.print('/');
  Serial.print(mag[2]);

  Serial.print(" TEMP:");
  Serial.print((float)temp / 10, 1);
  Serial.println("C");
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  
  //for (;;) {
    Serial.println("setup()");

    delay(1000);
    byte version = obd.begin();
    Serial.print("Freematics OBD-II Adapter ");
    if (version > 0) {
      Serial.println("detected");
      Serial.print("OBD firmware version ");
      Serial.print(version / 10);
      Serial.print('.');
      Serial.println(version % 10);
    //  break;
    } else {
      Serial.println("not detected");
    }
  //}

  // send some commands for testing and show response for debugging purpose
  testATcommands();

  hasMEMS = obd.memsInit();
  Serial.print("MEMS:");
  Serial.println(hasMEMS ? "Yes" : "No");
  
  // initialize OBD-II adapter
  do {
    Serial.println("Connecting...");
  } while (!obd.init());
  Serial.println("OBD connected!");

  char buf[64];
  if (obd.getVIN(buf, sizeof(buf))) {
      Serial.print("VIN:");
      Serial.println(buf);
  }
  
  uint16_t codes[6];
  byte dtcCount = obd.readDTC(codes, 6);
  if (dtcCount == 0) {
    Serial.println("No DTC"); 
  } else {
    Serial.print(dtcCount); 
    Serial.print(" DTC:");
    for (byte n = 0; n < dtcCount; n++) {
      Serial.print(' ');
      Serial.print(codes[n], HEX);
    }
    Serial.println();
  }
  delay(5000);
}

void loop()
{
  readPIDSingle();
  readPIDMultiple();
  readBatteryVoltage();
  if (hasMEMS) {
    readMEMS();
  }
}
