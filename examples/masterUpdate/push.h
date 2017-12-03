#pragma once

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

#define ESP32_SKETCH_SIZE 1310720

extern Web* web;

uint32_t dataExchange() {
  su.taskMaster();
  return RUN_NOW;
}
uint32_t dataSend() {
  //if (su.isReady()) {
    if (su.isIdle()) {
      Serial.println("Sending...");
      //su.sendData();
      File one = SPIFFS.open("/slave.ino.node32s.bin");
      //su.sendFile("/push.h", one);
      su.sendUpdate(one);
      return RUN_DELETE;
    }
  //}
  Serial.println("Busy");
  return 5000;
}

void pushHandle() {
      if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
        return web->requestAuthentication();
      }
      BUSY
      web->sendHeader("Connection", "close");
      web->sendHeader("Refresh", "10; url=/");
      web->send(200, "text/plain", "OK");
      taskAddWithDelay(dataSend, 500);
}

uint32_t pushInit() {
    pinMode(STX, OUTPUT);
    pinMode(SRX, INPUT);
    pinMode(SENA, OUTPUT);
#ifdef ESP8266
    Serial1.begin(BAUDRATE, SRX, STX);
#else
    Serial1.begin(BAUDRATE, SERIAL_8N1, SRX, STX);
#endif
    web->on("/push", HTTP_GET, pushHandle);//Update remote firmware
    su.begin(1);
    taskAdd(dataExchange);
    return RUN_DELETE;
};