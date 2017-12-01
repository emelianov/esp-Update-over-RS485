//
// Serial/RS-485 File transfer and update implementation
// (c)2017 Alexander Emelianov a.m.emelianov@gmail.com
// https://github.com/emelianov/esp-Update-over-RS485
//
// Slave node example

#define BAUDRATE 38400
#define SLAVE_ID 1

#include <serialUpdate.h>
#ifdef ESP8266
 #define SRX D2
 #define STX D3
 #define SENA D4
 SoftwareSerial Serial1(SRX,STX); 
 SerialUpdate<SoftwareSerial> su(&Serial1, SENA);
#else
 #define SRX 26
 #define STX 25
 #define SENA 5
 HardwareSerial Serial1(1);
 SerialUpdate<HardwareSerial> su(&Serial1, SENA);
 #include <Update.h>
#endif

void setup() {
#ifdef ESP8266
  Serial.begin(74880);
#else
  Serial.begin(115200);
#endif
    pinMode(STX, OUTPUT);
    pinMode(SRX, INPUT);
    pinMode(SENA, OUTPUT);
#ifdef ESP8266
    Serial1.begin(BAUDRATE, SRX, STX);
#else
    Serial1.begin(BAUDRATE, SERIAL_8N1, SRX, STX);
#endif
  SPIFFS.begin(true);
  sl.slave(SLAVE_ID);
  Serial.println("Ready!");
}
void loop() {
    su.taskSlave();
}