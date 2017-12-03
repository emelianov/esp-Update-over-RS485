## esp-update-over-RS485 comprehancive example

Example should work at ESP32 using HardwareSerial and at ESP8266 using SoftwareSerial

### Implemented features

1. File upload to master filesystem
2. OTA update of master firmeware
3. Send over Serial/RS-485 file from master filesystem to slave filesystem
4. Update over Serial/RS-485 slave firmware using firmware image stored on master filesystem

### Notes

1. Slave ID is hardcoded to 1.
2. After joining to Wi-Fi network node interface is available as http://esp.local for macOS (both for ESP8266 and ESP32) os http://esp for Windows (ESP8266 only). For Linux or Windows (ESP32) you should dig for ip address.
3. Arduino/ES8266 2.4[RC2] is required to build project for ESP8266
4. Serial is configured at GPIO 26/25 (RX/TX), 5 (Max485 transmit enable) for ESP32 and D2/D3 an D4 for ESP8266 (according to NodeMCU pin mapping).

### Libraries
* HTTP server library for ESP8266/ESP32 Arduino cores (https://github.com/emelianov/ESPWebServer)
Slight fixed as was unable to build original for ESP32
* Run 2017 (https://github.com/emelianov/Run)

## Contributions

https://github.com/emelianov/esp-Update-over-RS485

Alexander Emelianov a.m.emelianov@gmail.com


