#pragma once

//To upload through terminal you can use: curl -F "image=@firmware.tar" ehsensor8.local/update
#ifndef ESP8266
#include <Update.h>
#endif
#define ESP32_SKETCH_SIZE 1310720
extern Web* web;

void updateHandle() {
      if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
        return web->requestAuthentication();
      }
      BUSY
      web->sendHeader("Connection", "close");
      web->sendHeader("Refresh", "10; url=/");
      web->send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      taskAddWithDelay(restartESP, 500);
}

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
    Serial.println(upload.status);
          Update.printError(Serial);
      }
  IDLE
}

uint32_t updateInit() {
    web->on("/update", HTTP_POST, updateHandle, updateUploadHandle);//Update firmware
    return RUN_DELETE;
};
