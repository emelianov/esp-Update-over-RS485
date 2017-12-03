#pragma once
#include <ESP8266WebServer.h>
#include <detail/RequestHandlersImpl.h> // for StaticRequestHandler::getContentType(path);
#define BUSY ;
#define IDLE ;
String adminUsername = ADMINNAME;
String adminPassword = ADMINPASS;
void handleGenericFile();
void handlePrivate();
void listFile();
uint32_t restartESP();
uint32_t formatSPIFFS();
void handleReboot();
void handleFormat();
void handleFile();
void handleFileUpload();

class Web : public ESP8266WebServer {
public:
  Web(uint16_t port) : ESP8266WebServer(port) {
    begin();
    onNotFound(handleGenericFile);
  }
  bool handleFileRead(String path){
    if(path.endsWith(F("/"))) path += F("index.html");
    String contentType = StaticRequestHandler::getContentType(path);
    String pathWithGz = path + F(".gz");
    if(SPIFFS.exists(pathWithGz))
      path += F(".gz");
    if(SPIFFS.exists(path)){
      sendHeader("Connection", "close");
      sendHeader("Cache-Control", "no-store, must-revalidate");
      sendHeader("Access-Control-Allow-Origin", "*");
      File file = SPIFFS.open(path, "r");
      size_t sent = streamFile(file, contentType);
      Serial.println(sent);
      file.close();
      return true;
    }
    return false;
  }
};
  
Web* web;

uint32_t webLoop() {
  web->handleClient();
  return 100;
}

uint32_t webInit() {
  web = new Web(80);
  web->on("/private", handlePrivate);
  web->on("/", listFile);
  web->on("/list", listFile);
  web->on("/reboot", handleReboot);
  web->on("/format", handleFormat);
  web->on("/edit", HTTP_POST, handleFile, handleFileUpload);
  taskAdd(webLoop);
  Serial.print("[HTTP started]");
  return RUN_DELETE;
}

void handleGenericFile() {
  BUSY
  if(!web->handleFileRead(web->uri()))
    web->send(404, "text/plain", "FileNotFound");
  IDLE
}

void handlePrivate() {
  BUSY
  char data[400];
  char* xml = ("<?xml version = \"1.0\" encoding=\"UTF-8\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><uptime>%ld</uptime></private></ctrl>");
  //sprintf(data, ("<?xml version = \"1.0\" encoding=\"UTF-8\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><uptime>%ld</uptime></private></ctrl>"), ESP.getFreeHeap(), WiFi.RSSI(),(uint32_t)millis()/1000);
  sprintf(data, xml, ESP.getFreeHeap(), WiFi.RSSI(),millis()/1000);
  web->sendHeader("Connection", "close");
  web->sendHeader("Cache-Control", "no-store, must-revalidate");
  web->send(200, "text/xml", data);
  IDLE
}
void listFile() {
  if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return web->requestAuthentication();
  }
  String output = F("<html><head><meta charset='utf-8'>\
  <title>WiFiSocket - Maintains</title>\
\
 <style>\
.well{background-image:-webkit-linear-gradient(top,#e8e8e8 0,#f5f5f5 100%);\
background-image:-o-linear-gradient(top,#e8e8e8 0,#f5f5f5 100%);\
background-image:-webkit-gradient(linear,left top,left bottom,from(#e8e8e8),to(#f5f5f5));\
background-image:linear-gradient(to bottom,#e8e8e8 0,#f5f5f5 100%);\
filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#ffe8e8e8', endColorstr='#fff5f5f5', GradientType=0);\
background-repeat:repeat-x;\
border-color:#dcdcdc;\
-webkit-box-shadow:inset 0 1px 3px rgba(0,0,0,.05),0 1px 0 rgba(255,255,255,.1);\
box-shadow:inset 0 1px 3px rgba(0,0,0,.05),0 1px 0 rgba(255,255,255,.1)}\
.well{min-height:20px;padding:19px;margin-bottom:20px;background-color:#f5f5f5;border:1px solid #e3e3e3;border-radius:4px;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.05);box-shadow:inset 0 1px 1px rgba(0,0,0,.05)}\
.well blockquote{border-color:#ddd;border-color:rgba(0,0,0,.15)}\
.container{padding-right:15px;padding-left:15px;margin-right:auto;margin-left:auto}\
.col-xs-9{position:relative;min-height:1px;padding-right:15px;padding-left:15px}\
body{font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;font-size:14px;line-height:1.42857143;color:#333;background-color:#fff}\
h4,h4{font-size:18px}\
 </style>\
\
  </head>\
  <body>\
\
<div class='col-xs-9'><h4>masterUpdate - Maintains</h4></div>\
 <div class='container'><div class='well'>\
 <b>Network settings</b>\
 <hr>\
 <form method='POST' action='/net' enctype='multipart/form-data'>\
  <table>\
  <tr><td>IP</td><td>");
  output += WiFi.localIP().toString();
  output += F("</td></tr>\
  </table>\
 </form>\
 </div></div>\
\
<div class='container'><div class='well'>\
 <b>Local file system</b>\
 <hr>\
  <form method='POST' action='/edit' enctype='multipart/form-data'>\
  Upload file to local filesystem:<br>\
   <input type='file' name='update'>\
   <input type='submit' value='Upload file'>\
  </form>");
  String path = web->hasArg("dir")?web->arg("dir"):"/";
 #ifdef ESP8266
  Dir dir = SPIFFS.openDir(path);
  while(dir.next()){
    File entry = dir.openFile("r");
 #else
  File dir = SPIFFS.open(path);
  File entry;
  while (entry = dir.openNextFile()) {
 #endif
    String filename = String(entry.name());
    output += F("<br>");
    output += F("<a href='");
    output += filename;
    output += F("'>");
    output += filename;
    output += F("</a>&nbsp<a href='/delete?file=");
    output += filename;
    output += F("'><font color=red>[delete]</font></a>");
        output += F("&nbspRS-485:&nbsp<a href='/send?file=");
    output += filename;
    output += F("'>[Send file]</a>");
    if (filename.endsWith(".bin")) {
      output += F("&nbsp<a href='/push?file=");
      output += filename;
      output += F("'>[Push update]</a>");
    }
    output += F("<br>");
    entry.close();
  }
 #ifndef ESP8266
  output += F("Used/Total ");
  output += SPIFFS.usedBytes();
  output += F("/");
  output += SPIFFS.totalBytes();
 #endif
  output += F("</div></div>\
\
<div class='container'><div class='well'>\
 <b>Firmware update</b>\
 <hr>\
 Current version: ");
 output += VERSION;
 output += F("\
  <form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update' accept='.tar'><input type='submit' value='Update'></form>\
</div></div>\
\
<div class='container'><div class='well'>\
 <b>Resets</b>\
 <hr>\
  <form method='POST' action='/list' enctype='multipart/form-data'>\
  <input type='button' value='Reboot' onClick='if(confirm(\'Reboot device?\')) window.location=\'/reboot\';return true;'><br>\
  <input type='button' value='Format filesystem' onClick='if(confirm(\'Format file system and destroy all stored data?\')) window.location=\'/format\';return true;'></form>\
</div></div>\
</body><html>");
  web->sendHeader("Connection", "close");
  web->sendHeader("Cache-Control", "no-store, must-revalidate");
  web->sendHeader("Access-Control-Allow-Origin", "*");
  web->send(200, "text/html", output);  
}
// Delete file callback
void handleDelete() {
  if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return web->requestAuthentication();
  }
  web->sendHeader("Connection", "close");
  web->sendHeader("Cache-Control", "no-store, must-revalidate");
  web->sendHeader("Refresh", "5; url=/list");
  String path;
  if(web->args() != 0) {
    path = web->arg(0);
    if(path != "/" && SPIFFS.exists(path)) {
      if (SPIFFS.remove(path)) {
        web->send(200, "text/plain", "OK");
        IDLE
        return;
      } else {
        web->send(404, "text/plain", "FileNotFound");
        IDLE
        return;
      }
    }
  }
  web->send(500, "text/plain", "ERROR");
  IDLE
}

void handleReboot() {
  if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return web->requestAuthentication();
  }
  web->sendHeader("Connection", "close");
  web->sendHeader("Cache-Control", "no-store, must-revalidate");
  web->sendHeader("Refresh", "7; url=/");
  taskAddWithDelay(restartESP, 500);
  web->send(200, "text/plain", "Rebooting...");
}

void handleFormat() {
  if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return web->requestAuthentication();
  }
  web->sendHeader("Connection", "close");
  web->sendHeader("Cache-Control", "no-store, must-revalidate");
  web->sendHeader("Refresh", "15; url=/list");
  taskAdd(formatSPIFFS);
  web->send(200, "text/plain", "Formatting...");
}

void handleFile() {
  if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return web->requestAuthentication();
  }
  web->sendHeader("Connection", "close");
  web->sendHeader("Cache-Control", "no-store, must-revalidate");
  web->sendHeader("Access-Control-Allow-Origin", "*");
  web->sendHeader("Refresh", "5; url=/list");
  web->send(200, "text/plain", "OK");  
}
File fsUploadFile;
void handleFileUpload(){
  if(!web->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return web->requestAuthentication();
  }
  if(web->uri() != "/edit") return;
  BUSY
  HTTPUpload& upload = web->upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile) {
      Serial.print(".");
      fsUploadFile.write(upload.buf, upload.currentSize);
    } else {
      Serial.print("X");
    }
  } else if(upload.status == UPLOAD_FILE_END){
    Serial.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.close();
  }
  IDLE
}