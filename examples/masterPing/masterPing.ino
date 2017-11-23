#define APNAME "EW"
#define APPASS "iMpress6264"
#define DEFAULT_NAME "esp32"
#define VERSION "0.0"
#define BAUDRATE 38400

#include <serialUpdate.h>
#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #include <ESP8266mDNS.h>
 #include <ESP8266LLMNR.h>
 #include <FS.h>
 #include <SoftwareSerial.h>
 #define RX D2
 #define TX D3
 #define ENA D4
 SoftwareSerial Serial1(RX,TX); 
 SerialUpdate<SoftwareSerial> su(&Serial1, ENA, ENA);
#else
 #include <WiFi.h>
 #include <ESPmDNS.h>
 #include <SPIFFS.h>
 #define RX 26
 #define TX 25
 #define ENA 5
 HardwareSerial Serial1(1);
 SerialUpdate<HardwareSerial> su(&Serial1, ENA, ENA);
#endif

#include <Run.h>
#include "web.h"
#include "update.h"

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
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT);
  pinMode(ENA, OUTPUT);
#ifdef ESP8266
  Serial.begin(74880);
  Serial1.begin(BAUDRATE, RX, TX);
#else
  Serial.begin(115200);
  Serial1.begin(BAUDRATE, SERIAL_8N1, RX, TX);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(APNAME, APPASS);
  taskAdd(wifiWait);
  SPIFFS.begin(true);
  taskAdd(webInit);
  taskAdd(updateInit);
  su.begin();
  taskAdd(dataExchange);
  taskAddWithDelay(dataSend, 5000);
}

void loop() {
  taskExec();
  yield();
}