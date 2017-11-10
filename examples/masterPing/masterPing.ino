#include <RSerial.h>
#include <SoftwareSerial.h>

#define RX D2
#define TX D3
#define ENA D4

SoftwareSerial SSerial(RX,TX); 
RSerial<SoftwareSerial> sl(&SSerial); 

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
}

void loop() {
  Serial.print("Sending...");
  sl.taskMaster();
  Serial.println("done.");
  delay(5000);
}