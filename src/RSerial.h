#pragma once
#ifndef ESP8266
 #include <rom/crc.h>
#endif
// Packet signature
#define MAGIC_SIG {0xFF, 0xEE, 0xDD, 0xCC}
#define MAGIC_SIG_LENGTH 4
// Maximim packet length
#define FRAME_LENGTH 1024
// Packet resend count if CRC or timeout failure
#define MAX_RETRY_COUNT	5
#define RESPONSE_TIMEOUT 10000
// Data transfer timeout
#define MAX_SEND_TIME 2000

//For SoftwareSerial debug
#define ERX D1
#define ETX D4

struct packetHeader {
	uint8_t sig[MAGIC_SIG_LENGTH];	// Packet signature
	uint8_t id;						// From/To slave ID
	uint8_t flags;					// Packet flags (reserved for future use)
	uint8_t command;
	uint16_t dataSize;				// Packet data size (Full packet size is sizeof(header) + data size + sizeof(crc32)
};

#define C_OK 1
#define C_ERROR 2
#define C_PING 3
#define C_BADCRC 4

union packetFrame {
	packetHeader header;
	uint8_t raw[FRAME_LENGTH];
};

enum status_t {RS_IDLE, RS_SEND, RS_WAIT, RS_OK, RS_RECEIVE, RS_READY, RS_ERROR, RS_TIMEOUT, RS_FAILED, RS_DONE, RS_CRC_ERROR};

#ifdef ESP8266
// CRC32 calculation for ESP8266. Building function is used for ESP32
uint32_t crc32_le(uint32_t crc, const uint8_t *data, size_t length)
{
    uint32_t i;
    bool bit;
    uint8_t c;
    
    while (length--) {
        c = *data++;
        for (i = 0x80; i > 0; i >>= 1) {
            bit = crc & 0x80000000;
            if (c & i) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x04c11db7;
            }
        }
    }
    return crc;
}
#endif

template <typename T> class RSerial {
public:
	RSerial(T* serial) {
		_serial = serial;
	}
	RSerial(T* serial, int16_t max485_rx, int16_t max485_tx) {
		_serial = serial;
		_rxPin = max485_rx;
		_txPin = max485_tx;
		digitalWrite(_rxPin, LOW);
	}
	// Slave loop function
	void taskSlave() {
		switch (_state) {
		case RS_IDLE:
		case RS_RECEIVE:
			_state = receiving(RS_RECEIVE);
			break;
		case RS_READY:
		Serial.println("READY");
			fillFrame(C_OK,"OK");
			send();
			break;
		case RS_CRC_ERROR:
			fillFrame(C_BADCRC,"ERR");
			send();
			break;
		case RS_ERROR:
			fillFrame(C_ERROR,"ERR");
			send();
			break;
		case RS_SEND:
			_state = sending();
			break;
		case RS_DONE:
			receive();
			break;
		default:
			;
			//Serial.print("Unknown state: ");
			//Serial.println(_state);
		}
	}
	// Master loop function
	status_t taskMaster() {
		switch (_state) {
		case RS_SEND:
			_state = sending();
			break;
		case RS_DONE:
			receive();
			_state = RS_WAIT;
			break;
		case RS_RECEIVE:
		case RS_WAIT:
			if (millis() - _start > MAX_SEND_TIME) {
				_state = RS_ERROR;
			} else {
				_state = receiving(RS_RECEIVE);
			}
			break;
		case RS_READY:
			if (_buf.header.command == C_BADCRC) {
				_state = RS_ERROR;
							Serial.println("Error");
			} else {
				_state = this->processPacketMaster();
				//_state = RS_IDLE;
							Serial.println("Ready");
			}
			break;
		case RS_ERROR:
			_retryCount++;
			if (_retryCount >  MAX_RETRY_COUNT) {
				_state = RS_FAILED;
			} else {
				_state = send();
			}
			break;
		case RS_TIMEOUT:
		case RS_IDLE:
		case RS_FAILED:
		default:
			;
			//Serial.println("Unknown state");
		}
	}
	// Returns packet data CRC32
	uint32_t crc(packetFrame* data) {
		return crc32_le(0, (uint8_t*)data, data->header.dataSize - sizeof(uint32_t));
	}
	void clean() {
		_pos = 0;
		_retryCount = 0;
	}
	// Initiate sending of frame buffer
	status_t send() {
		//_serial->flush();
		//enableSend();
		_pos = 0;
		//_retryCount = 0;
		_state = RS_SEND;
		Serial.println("Send");
		_start = millis();
		return _state;
	}
	status_t receive() {
		//_serial->flush();
		_pos = 0;
		//enableReceive();
		_state = RS_IDLE;
		_start = millis();
		return _state;
	}
	virtual status_t processPacketMaster() {
		debugPrintPacket();
		if (_buf.header.command == C_BADCRC) {
			Serial.println("CRC");
			return send();
		}
		return RS_IDLE;
	}
	virtual status_t processPacketSlave() {
		debugPrintPacket();
		if (_buf.header.command == C_PING) {
			memcpy(&_reply, &_buf, _buf.header.dataSize + sizeof(packetHeader));
			return send();
		}
		return RS_OK;
	}
	bool isIdle() {
		return _state == RS_IDLE || _state == RS_ERROR || _state == RS_FAILED;
	}
	void debugPrintPacket() {
		for (uint8_t i = 0; i < _buf.header.dataSize; i++) {
			Serial.print(_buf.raw[sizeof(packetHeader) + i], HEX);
			Serial.print(" ");
		}
		Serial.println();
	}
	bool fillFrame(uint8_t com, const char* data, uint8_t slaveId = 0) {
		return fillFrame(com, (uint8_t*)data, strlen(data), slaveId);
	}
	bool fillFrame(uint8_t com, const uint8_t* data, uint16_t len, uint8_t slaveId = 0) {
		if (len > FRAME_LENGTH - sizeof(packetHeader) - sizeof(uint32_t)) return false;
		memcpy(_reply.header.sig, _sig, MAGIC_SIG_LENGTH);
		_reply.header.id = slaveId;
		_reply.header.flags = 0;
		_reply.header.command = com;
		_reply.header.dataSize = len + sizeof(uint32_t);
		memcpy(&_reply.raw[sizeof(packetHeader)], data, len);
		uint32_t curCrc = crc(&_reply);
		memcpy(&_reply.raw[len + sizeof(packetHeader)], &curCrc, sizeof(uint32_t));
		_pos = 0;
		return true;
	}
	bool fillFrame(uint8_t com, File data, uint8_t slaveId = 0) {
		memcpy(_reply.header.sig, _sig, MAGIC_SIG_LENGTH);
		_reply.header.id = slaveId;
		_reply.header.flags = 0;
		_reply.header.command = com;
		uint16_t len = sizeof(packetHeader);
		while (data.available() && len < FRAME_LENGTH - sizeof(packetHeader) - sizeof(uint32_t)) {
			_reply.raw[len] = data.read();
			len++;
		}
		_reply.header.dataSize = len + sizeof(uint32_t);
		uint32_t curCrc = crc(&_reply);
		memcpy(&_reply.raw[len + sizeof(packetHeader)], &curCrc, sizeof(uint32_t));
		_pos = 0;
		return true;
	}
	template <typename R> bool fillFrame(uint8_t com, const R &data, uint8_t slaveId = 0) {
		return fillFrame(com, (uint8_t*)data, sizeof(data));
	}
protected:
	uint8_t		_id = 0;
	uint16_t	_pos = 0;
	status_t	_state = RS_IDLE;
	uint8_t		_sig[MAGIC_SIG_LENGTH] = MAGIC_SIG;
	uint8_t		_retryCount = 0;
	packetFrame	_buf;
	packetFrame	_reply;
	T*			_serial;
	uint16_t	_txPin = -1;
	uint16_t	_rxPin = -1;
	uint32_t	_start = 0;

	void enableSend() {
		//digitalWrite(ERX, HIGH);//For SoftwareSerial debug
		//digitalWrite(ETX, HIGH);//For SoftwareSerial debug
		//return;
		if (_txPin >= 0) {
			_serial->flush();
			digitalWrite(_txPin, HIGH);
			//digitalWrite(_rxPin, HIGH);
			delay(100);
		}
	}
	void enableReceive() {
		//digitalWrite(ERX, LOW);//For SoftwareSerial debug
		//digitalWrite(ETX, LOW);//For SoftwareSerial debug
		//return;
		if (_rxPin >= 0) {
			delay(10);
			digitalWrite(_rxPin, LOW);
			digitalWrite(_txPin, LOW);
			_serial->flush();
		}
	}
	uint32_t extractCrc() {
		uint32_t cr = _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 3];
		cr = cr << 8;
		cr += _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 2];
		cr = cr << 8;
		cr += _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 1];
		cr = cr << 8;
		cr += _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 0];
		return cr;
	}
	status_t receiving(status_t defaultState = RS_IDLE) {
		status_t state = defaultState;
		//Serial.print(".");
		while (_serial->available()) {
			_buf.raw[_pos] = _serial->read();
			if (_pos < MAGIC_SIG_LENGTH && _buf.raw[_pos] != _sig[_pos]) {
				_pos = 0;
				if (_buf.raw[_pos] != _sig[_pos]) {
					Serial.print("d");
					Serial.print(_buf.raw[_pos], HEX);
					continue;
				}
			}
			Serial.print("r");
			Serial.print(_buf.raw[_pos], HEX);
			_pos++;
			if ((_pos >= sizeof(packetHeader) && _pos >= _buf.header.dataSize + sizeof(packetHeader)) || _pos >= FRAME_LENGTH) {
				_serial->flush();
				_pos = 0;
				//Serial.println();
				if (_buf.header.dataSize >= FRAME_LENGTH || crc(&_buf) != extractCrc()) {
					//Serial.println(extractCrc(), HEX);
					Serial.println("Got error");
					state = RS_ERROR;
					break;
				}
				Serial.println("Got packet");
				state = RS_READY;
				break;
			}
		}
		return state;
	}
	status_t sending(status_t defaultState = RS_SEND) {
		status_t state = defaultState;
		//_serial->end();
		//_serial->begin(38400);
		//while (_serial->available()) _serial->read();
		digitalWrite(_rxPin, HIGH);
		while (_pos < sizeof(packetHeader) + _reply.header.dataSize) {
			//if (millis() - _start > MAX_SEND_TIME)
			//	return RS_SEND;
			Serial.print("s");
			Serial.print(_reply.raw[_pos], HEX);
			_serial->write(_reply.raw[_pos]);
			//_serial->print(_reply.raw[_pos], HEX);
			_pos++;
		}
		//delay(_pos);
		digitalWrite(_rxPin, LOW);
		_serial->flush();
		_pos = 0;
		return RS_DONE;
	}
};