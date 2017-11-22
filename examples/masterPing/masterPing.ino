#include <Run.h>
#define RX 16
//#define RX D2
#define TX 17
//#define TX D3
#ifndef ESP8266
 #define D1 19
 #define D4 17
#endif

HardwareSerial Serial1(1);
//SoftwareSerial SSerial(RX,TX); 
//SerialUpdate<SoftwareSerial> su(&SSerial, 5, 5);
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
  Serial1.begin(38400,SERIAL_8N1, 26, 25);
  su.begin();
  taskAdd(dataExchange);
  taskAddWithDelay(dataSend, 5000);
}

void loop() {
  taskExec();
}