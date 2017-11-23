#pragma once
#include <RSerial.h>
#ifdef ESP8266
 #include <FS.h>
#else
 #include <SPIFFS.h>
 #include <Update.h>
#endif

#define ACT_IDLE 0
#define GET_VERSION 0x10
#define BEGIN_UPDATE 0x11
#define IMAGE_DATA 0x12
#define END_UPDATE 0x13
#define FILE_CREATE 0x21
#define FILE_DATA 0x22
#define FILE_CLOSE 0x23
#define FILE_ERROR 0x24

template <typename T> class SerialUpdate : public RSerial<T> {
public:
	String version;
	SerialUpdate(T* serial, String ver = "") : RSerial<T>(serial) {
		version = ver;
	}
	SerialUpdate(T* serial, uint16_t rx, uint16_t tx, String ver = "") : RSerial<T>(serial, rx, tx) {
		version = ver;
	}
	void begin(uint8_t slaveId = 0) {
		this->_slaveId = slaveId;
		this->fillFrame(GET_VERSION, "GET", this->_slaveId);
		this->send();
	}
	bool isReady() {
		return version != "";
	}
	void sendData() {
		this->fillFrame(IMAGE_DATA, "DATA12345678901234567890", this->_slaveId);
		this->send();
	}
	void end() {
		uint32_t lcrc = 0xFFFFF;
		this->fillFrame(END_UPDATE, lcrc, this->_slaveId);
		this->send();
	}
	void sendFile(char* name, File dataSource) {
		this->fillFrame(FILE_CREATE, name, this->_slaveId);
		this->send();
		this->_action = FILE_CREATE;
		this->_stream = dataSource;
	}
	status_t processPacketSlave() {
		switch (this->_buf.header.command) {
		case GET_VERSION:
			this->fillFrame(GET_VERSION, version.c_str());
			this->send();
			break;
		case BEGIN_UPDATE:
			break;
		case IMAGE_DATA:
			break;
		case END_UPDATE:
			break;
		case FILE_CREATE:
			String newFileName = "";
			for (uint16_t i = 0; i < this->_buf.header.dataSize; i++) {
				newFileName += (char)this->_bur.raw[sizeof(packetHeader) + i];
			}
			if(!newFileName.startsWith("/")) newFileName = "/" + newFileName;
			this->_file = SPIFFS.open(newFileName, "w");
			if (this->_file) {
				this->fillFrame(C_OK, "File created");
			} else {
				this->fillFrame(FILE_ERROR, "Create failed");
			}
			this->_state = this->send();
			break;
		case FILE_DATA:
			if (this->_file) {
				uint16_t written = this->_file.write(&this->_buf.raw[sizeof(packetHeader)], this->_buf.header.dataSize);
				if (written < this->_buf.header.dataSize) {
					this->fillFrame(FILE_ERROR, "Write failed");
				} else {
					this->fillFrame(C_OK, "OK");
				}
			} else {
				this->fillFrame(FILE_ERROR, "Not open");
			}
			this->_state = this->send();
			break;
		case FILE_CLOSE:
			if (this->_file) {
				this->_file.close();
				this->fillFrame(C_OK, "OK");
			} else {
				this->fillFrame(FILE_ERROR, "Not open");
			}
			this->_state = this->send();
			break;
		default:
			return RSerial<T>::processPacketSlave();
		}
	}
	status_t processPacketMaster() {
	Serial.println("PM");
		switch (this->_buf.header.command) {
		case GET_VERSION:
			for (uint16_t i = 0; i < this->_buf.header.dataSize; i++) {
				version += (char)this->_buf.raw[sizeof(packetHeader) + i];
			}
			break;
		case FILE_ERROR:
			this->_action = FILE_ERROR;
			break;
		default:
			this->_state = RSerial<T>::processPacketMaster();
			Serial.println(this->_state);
			return this->_state;
		}
		this->_state = RS_IDLE;
		return this->_state;
	}
private:
	uint8_t _slaveId;
	uint8_t _action = ACT_IDLE;
	File _stream;
};