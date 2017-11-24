#pragma once

#include <serialUpdate.h>
#ifdef ESP8266
 #define SRX D2
 #define STX D3
 #define SENA D4
 SoftwareSerial Serial1(SRX,STX); 
 SerialUpdate<SoftwareSerial> su(&Serial1, SENA, SENA);
#else
 #define SRX 26
 #define STX 25
 #define SENA 5
 HardwareSerial Serial1(1);
 SerialUpdate<HardwareSerial> su(&Serial1, SENA, SENA);
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
      su.sendData();
      return 5000;
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
      taskAddWithDelay(restartESP, 500);
}
/*
void updateUploadHandle() {
  size_t sketchSpace;
  BUSY
    HTTPUpload& upload = web->upload();
    switch (upload.status) {
    case UPLOAD_FILE_START:
    //WiFiUDP::stopAll();
     #ifdef ESP8266
      sketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
     #else
      sketchSpace = ESP32_SKETCH_SIZE;
     #endif
     Serial.println(sketchSpace);
      if(!Update.begin(sketchSpace)){//start with max available size
        Update.printError(Serial);
      }
      break;
    case UPLOAD_FILE_WRITE:
    Serial.print(".");
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
        break;
    case UPLOAD_FILE_END:
    Serial.println("Write");
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        }
        break;
    default:
        Update.printError(Serial);
      }
  IDLE
}
*/
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
    su.begin();
    taskAdd(dataExchange);
    taskAddWithDelay(dataSend, 5000);
    return RUN_DELETE;
};
