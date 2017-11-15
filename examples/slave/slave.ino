#include <RSerial.h>
#include <SoftwareSerial.h>

#define RX D2
#define TX D3
#define ENA D4

SoftwareSerial SSerial(RX,TX); 
RSerial<SoftwareSerial> sl(&SSerial, D4, D4); 

void setup() {
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT);
  pinMode(ENA, OUTPUT);
  pinMode(D1, OUTPUT);
  digitalWrite(ENA, LOW);
  digitalWrite(D1, LOW);
  Serial.begin(74880);
  Serial.println("123");
  SSerial.begin(38400);
  //SSerial.setTransmitEnablePin(ENA);
  sl.receive();
  Serial.println("Ready");
}
void loop() {
    sl.taskSlave();
}