#define APNAME "EW"
#define APPASS "iMpress6264"
#define DEFAULT_NAME "esp"
#define ADMINNAME "admin"
#define ADMINPASS "password"
#define VERSION "0.0"
#define BAUDRATE 38400

#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #include <ESP8266mDNS.h>
 #include <ESP8266LLMNR.h>
 #include <FS.h>
#else
 #include <WiFi.h>
 #include <ESPmDNS.h>
 #include <SPIFFS.h>
#endif

#include <Run.h>
#include "web.h"
#include "update.h"
#include "push.h"

uint32_t wifiWait() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    if (!MDNS.begin(DEFAULT_NAME)) {
      Serial.print("[mDNS: failed]");
    } else {
      MDNS.addService("http", "tcp", 80);  // Add service to MDNS-SD
      Serial.print("[mDNS: started]");
    }
    // LLMNR
   #ifdef ESP8266
    LLMNR.begin(DEFAULT_NAME);
    Serial.println("[LLMNR: started]");
   #endif
    return RUN_DELETE;
  }
  Serial.print(".");
  return 500;
}

uint32_t restartESP() {
  ESP.restart();
  return RUN_DELETE;
}

uint32_t formatSPIFFS() {
  SPIFFS.end();
  SPIFFS.format();
  SPIFFS.begin();
  return RUN_DELETE;
}

void setup() {
#ifdef ESP8266
  Serial.begin(74880);
#else
  Serial.begin(115200);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(APNAME, APPASS);
  taskAdd(wifiWait);
  SPIFFS.begin(true);
  taskAdd(webInit);
  taskAdd(updateInit);
  taskAdd(pushInit);
}

void loop() {
  taskExec();
  yield();
}