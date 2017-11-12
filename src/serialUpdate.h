#pragma once
#include <RSerial.h>
#ifndef ESP8266
 #include <Update.h>
#endif

#define GET_VERSION 0x10
#define BEGIN_UPDATE 0x11
#define IMAGE_DATA 0x12
#define END_UPDATE 0x13

template <typename T> class SerialUpdate : public RSerial<T> {
public:
	String version;
	SerialUpdate(T* serial, String ver = "") : RSerial<T>(serial) {
		version = ver;
	}
	void begin() {
		this->fillFrame(GET_VERSION, ":GET");
		this->send();
	}
	bool isReady() {
		return version != "";
	}
	void sendData() {
		this->fillFrame(IMAGE_DATA, "DATA ");
		this->send();
	}
	void end() {
		uint32_t lcrc = 0xFFFFF;
		this->fillFrame(END_UPDATE, lcrc);
		this->send();
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
		default:
			return RSerial<T>::processPacketSlave();
		}
	}
	status_t processPacketMaster() {
		switch (this->_buf.header.command) {
		case GET_VERSION:
			for (uint16_t i = 0; i < this->_buf.header.dataSize; i++) {
				version += (char)this->_bur.raw[sizeof(packetHeader) + i];
			}
			break;
		case BEGIN_UPDATE:
			break;
		case IMAGE_DATA:
			break;
		case END_UPDATE:
			break;
		default:
			return RSerial<T>::processPacketMaster();
		}
		this->_state = RS_IDLE;
		return this->_state;
	}
};