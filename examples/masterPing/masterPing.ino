#include <SerialUpdate.h>
//#include <SoftwareSerial.h>
#include <Run.h>
//16
#define RX 16
//D2
//17
#define TX 17
//D3
//D1 D4
#ifndef ESP8266
 #define D1 19
 #define D4 17
#endif
//SoftwareSerial SSerial(RX,TX); 
HardwareSerial Serial1(2);
//SerialUpdate<SoftwareSerial> su(&SSerial, D4, D4);
SerialUpdate<HardwareSerial> su(&Serial1, 5, 5);

uint32_t dataExchange() {
  su.taskMaster();
  return RUN_NOW;
}
uint32_t dataSend() {
 // if (su.isReady()) {
    if (su.isIdle()) {
      Serial.println("Sending...");
      su.sendData();
    }
  //}
  return 5000;
}

void setup() {
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT);
  pinMode(5, OUTPUT);
  //pinMode(D1, OUTPUT);
  Serial.begin(74880);
  //SSerial.begin(38400);
  //SSerial.setTransmitEnablePin(D4);
  Serial1.begin(38400);
  su.begin();
  taskAdd(dataExchange);
  taskAddWithDelay(dataSend, 1000);
}

void loop() {
  taskExec();
}