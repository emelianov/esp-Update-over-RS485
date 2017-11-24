# esp-Update-over-RS485

## 1. Packet exchange helper library API
```
struct packetHeader {
	uint8_t sig[MAGIC_SIG_LENGTH];	// Packet signature
	uint8_t id;						// From/To slave ID
	uint8_t flags;					// Packet flags (reserved for future use)
	uint8_t command;
	uint16_t dataSize;				// Packet data size (Full packet size is sizeof(header) + data size + sizeof(crc32)
};

union packetFrame {
	packetHeader header;
	uint8_t raw[FRAME_LENGTH];
};

template <typename T> class RSerial {
	RSerial(T* serial);
	RSerial(T* serial, int16_t max485_rx, int16_t max485_tx);
	void taskSlave();
	status_t taskMaster();
	void clean();
	status_t send();
	status_t receive();
	virtual status_t processPacketMaster();
	virtual status_t processPacketSlave();
	bool isIdle();
	void debugPrintPacket();
	bool fillFrame(uint8_t com, const char* data, uint8_t slaveId = 0);
	bool fillFrame(uint8_t com, const uint8_t* data, uint16_t len, uint8_t slaveId = 0);
	bool fillFrame(uint8_t com, File data, uint8_t slaveId = 0);
	template <typename R> bool fillFrame(uint8_t com, const R &data, uint8_t slaveId = 0);
};
```

## 2. Resources used
* Arduino (https://github.com/arduino/Arduino)
* Arduino core for the ESP32 (https://github.com/espressif/arduino-esp32)
* ESP8266 core for Arduino (https://github.com/esp8266/Arduino)
* HTTP server library for ESP8266/ESP32 Arduino cores (https://github.com/emelianov/ESPWebServer)
* Run 2017 (https://github.com/emelianov/Run)
