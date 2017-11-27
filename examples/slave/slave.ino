#include <RSerial.h>

#define RX 16
//#define RX D2
#define TX 17
//#define TX D3

HardwareSerial Serial1(1);
RSerial<HardwareSerial> sl(&Serial1, 5, 5);

void setup() {
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT);
  pinMode(5, OUTPUT);
  //pinMode(D1, OUTPUT);
  //digitalWrite(5, LOW);
  //digitalWrite(D1, LOW);
  Serial.begin(115200);
  Serial.println("123");
  Serial1.begin(38400,SERIAL_8N1, 26, 25);
  sl.receive();
  Serial.println("Ready");
}
void loop() {
    sl.taskSlave();
}