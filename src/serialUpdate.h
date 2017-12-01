//
// Serial/RS-485 File transfer and update implementation
// (c)2017 Alexander Emelianov a.m.emelianov@gmail.com
// https://github.com/emelianov/esp-Update-over-RS485

#pragma once
#include <RSerial.h>
#ifdef ESP8266
 #include <FS.h>
#else
 #include <SPIFFS.h>
 #include <Update.h>
#endif

#define ESP32_SKETCH_SIZE 1310720

#define ACT_IDLE 0
#define CANCEL_ALL 0x09
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
	SerialUpdate(T* serial, const char* ver = "", uint8_t id = 0) : RSerial<T>(serial, id) {
		version = ver;
	}
	SerialUpdate(T* serial, const char* ver = "", uint8_t id, uint16_t tx) : RSerial<T>(serial, id, tx) {
		version = ver;
	}
	void begin(uint8_t slaveId = 0) {	// Initialize connection to slave
		this->_slaveId = slaveId;
		this->fillFrame(GET_VERSION, "GET", this->_slaveId);
		this->send();
	}
	void slave(const char* ver = "", uint8_t id = 0) {	// Switch to slave mode
		if (strlen(ver) > 0) this->vervion = ver;
		if (id != 0) this->_id = id;
		receive();
	}
	bool isReady() {	// if begin was success
		return version != "";
	}
	//void sendData() {
	//	this->fillFrame(IMAGE_DATA, "DATA12345678901234567890", this->_slaveId);
	//	this->send();
	//}
	//void end() {
	//	uint32_t lcrc = 0xFFFFF;
	//	this->fillFrame(END_UPDATE, lcrc, this->_slaveId);
	//	this->send();
	//}
	bool sendFile(char* name, File dataSource) {		// Send file to slave
		this->fillFrame(FILE_CREATE, name, this->_slaveId);
		this->send();
		this->_action = FILE_CREATE;
		this->_stream = dataSource;
		Serial.println("Send: initiated");
		return true;
	}
	bool sendUpdate(File dataSource) {		// Send firmware update to slave
		this->fillFrame(BEGIN_UPDATE, "DUMMY", this->_slaveId);
		this->send();
		this->_action = BEGIN_UPDATE;
		this->_stream = dataSource;
		Serial.println("Update: initiated");
		return true;
	}
	bool cancel() {		// Cancel current send operation
		this->_action = ACT_IDLE;
		if (this->_stream) {
			this->_straem.close();
		}
		this->fillFrame(CANCEL_ALL, "CANCEL", this->_slaveId);
		this->send();		
	}
	status_t processPacketSlave() {
		uint32_t blockSize;
		uint32_t sketchSpace;
	Serial.println(this->_buf.header.command, HEX);
		String newFileName = "";
		switch (this->_buf.header.command) {
		case GET_VERSION:
			this->fillFrame(GET_VERSION, version.c_str());
			this->send();
			break;
		case CANCEL_ALL:
			if (this->_file) {
				this->_file.close();
			}
			this->_action = ACT_IDLE;
			this->fillFrame(C_OK, "OK");
			this->_state = this->send();
			break;
		case BEGIN_UPDATE:
    		//WiFiUDP::stopAll();
     		#ifdef ESP8266
      			sketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
     		#else
      			sketchSpace = ESP32_SKETCH_SIZE;
     		#endif
      		if(Update.begin(sketchSpace)){//start with max available size
        		this->fillFrame(C_OK, "Update start");
      		} else {
      			Update.printError(Serial);
      			this->fillFrame(FILE_ERROR, "Update start failed");
      		}
      		this->_state = this->send();
			break;
		case IMAGE_DATA:
    		Serial.print(".");
    		blockSize = this->_buf.header.dataSize - sizeof(uint32_t);
        	if(Update.write(&this->_buf.raw[sizeof(packetHeader)], blockSize) == blockSize){
        		this->fillFrame(C_OK, "Update upload");
        	} else {
        		Update.printError(Serial);
        		this->fillFrame(FILE_ERROR, "Update upload failed");
        	}
        	this->_state = this->send();
			break;
		case END_UPDATE:
    		Serial.println("Write");
        	if(Update.end(true)){ //true to set the size to the current progress
          		Serial.printf("Update Success. \nRebooting...\n");
          		this->fillFrame(C_OK, "Update success");
        	} else {
        		this->fillFrame(C_OK, "Update failed");
        	}
        	this->_state = this->send();
			break;
		case FILE_CREATE:
			for (uint16_t i = 0; i < this->_buf.header.dataSize; i++) {
				newFileName += (char)this->_buf.raw[sizeof(packetHeader) + i];
			}
			if(!newFileName.startsWith("/")) newFileName = "/" + newFileName;
			this->_file = SPIFFS.open(newFileName, "w");
			if (this->_file) {
				Serial.println("File created");
				this->fillFrame(C_OK, "File created");
			} else {
				Serial.println("Create failed");
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
			if (this->_action == FILE_CREATE || this->_action == FILE_DATA) {
				if (this->_stream.available()) {
					if (this->fillFrame(FILE_DATA, this->_stream)) {
						this->_action = FILE_DATA;
						this->_state = this->send();
					}
				} else {
					if (this->fillFrame(FILE_CLOSE, "EOF")) {
						this->_stream.close();
						this->_action = FILE_CLOSE;
						this->_state = this->send();
					}
				}
			} else if (this->_action == BEGIN_UPDATE || this->_action == IMAGE_DATA) {
				if (this->_stream.available()) {
					Serial.println(".");
					if (this->fillFrame(IMAGE_DATA, this->_stream)) {
						this->_action = IMAGE_DATA;
						this->_state = this->send();
					}
				} else {
					if (this->fillFrame(END_UPDATE, "EOF")) {
						Serial.println("End update");
						this->_stream.close();
						this->_action = END_UPDATE;
						this->send();
					}
				}			
			} else {
				this->_stream.close();
				this->_action = ACT_IDLE;
			}
			return this->_state;
		}
		this->_state = RS_IDLE;
		return this->_state;
	}
private:
	uint8_t _slaveId;
	uint8_t _action = ACT_IDLE;
	File _file;
	File _stream;
};