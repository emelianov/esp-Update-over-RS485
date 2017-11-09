#include <RSerial.h>
#include <SoftwareSerial.h>

#define RX D1
#define TX D4
#define ENA D2

SoftwareSerial SSerial(RX,TX); 
RSerial<HardwareSerial> sl(&Serial); 

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  pinMode(ENA, OUTPUT);
  digitalWrite(ENA, LOW);
  sl.taskSlave();
  delay(100);
}
