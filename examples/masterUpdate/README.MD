## esp-update-over-RS485 comprehancive example

Example should work at ESP32 using HardwareSerial and at ESP8266 using SoftwareSerial

### Implemented features

1. File upload to master filesystem
2. OTA update of master firmeware
3. Send over Serial/RS-485 file from master filesystem to slave filesystem
4. Update over Serial/RS-485 slave firmware using firmware image stored on master filesystem

### Libraries
* HTTP server library for ESP8266/ESP32 Arduino cores (https://github.com/emelianov/ESPWebServer)
Slight fixed as was unable to build original for ESP32
* Run 2017 (https://github.com/emelianov/Run)

