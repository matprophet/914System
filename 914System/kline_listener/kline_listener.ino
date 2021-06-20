#include <AltSoftSerial.h>
#include <SoftwareSerial.h>

#define RX_PIN 6
#define TX_PIN 7

AltSoftSerial kline = AltSoftSerial(RX_PIN, TX_PIN);

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("setup() start, delaying...");

    kline.begin(10400);
}

void loop() {
  // put your main code here, to run repeatedly:

  if (kline.available() > 0) {
     byte b = kline.read();
     Serial.println(b, HEX);   
  }
}
