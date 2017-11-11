#include <RSerial.h>
#include <SoftwareSerial.h>
#include <Run.h>


#define RX D2
#define TX D3
#define ENA D4

SoftwareSerial SSerial(RX,TX); 
RSerial<SoftwareSerial> sl(&SSerial); 

uint32_t dataExchange() {
  sl.taskMaster();
  return RUN_NOW;
}
uint32_t dataSend() {
  if (sl.isIdle()) {
    Serial.println("Sending...");
    sl.send();
  }
  return 5000;
}

void setup() {
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  pinMode(ENA, OUTPUT);
  digitalWrite(ENA, HIGH);
  pinMode(D1, OUTPUT);
  digitalWrite(D1, HIGH);
  Serial.begin(74880);
  SSerial.begin(38400);
  if (sl.fillFrame(0xAA, "TEST")) Serial.println("Frame prepared");
  taskAdd(dataExchange);
  taskAdd(dataSend);
}

void loop() {
  taskExec();
}