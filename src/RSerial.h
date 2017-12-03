//
// Serial/RS-485 Master-Slave packet exchange protocol
// (c)2017 Alexander Emelianov a.m.emelianov@gmail.com
// https://github.com/emelianov/esp-Update-over-RS485

#pragma once

#ifndef ESP8266
 #include <rom/crc.h>	// for crc_le()
 #include <SPIFFS.h>
#else
 #include <FS.h>
#endif

// Packet signature
#define MAGIC_SIG {0xFF, 0xEE, 0xDD, 0xCC}
#define MAGIC_SIG_LENGTH 4
// Maximim packet length
#define FRAME_LENGTH 1024
// Byte transfer delay. 250uS works for 38400 RS-485 transfer. For Serial transfer should be comment out.
#define RS_BYTE_DELAY 250
// Packet resend count if CRC or timeout failure
#define MAX_RETRY_COUNT	5
#define RESPONSE_TIMEOUT 10000
// Data transfer timeout
#define MAX_SEND_TIME 3000

struct packetHeader {
	uint8_t sig[MAGIC_SIG_LENGTH];	// Packet signature
	uint8_t id;						// To slave ID
	uint32_t seq;					// Packet sequence #
	uint8_t command;
	uint16_t dataSize;				// Packet data size (Full packet size is sizeof(header) + data size + sizeof(crc32))
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
	RSerial(T* serial, uint8_t id = 0) {
		_serial = serial;
		_id = id;
	}
	RSerial(T* serial, uint8_t id, int16_t max485_tx) {
		_serial = serial;
		_id = id;
		_txPin = max485_tx;
	}
	// Slave loop function
	status_t taskSlave() {
		switch (_state) {
		case RS_IDLE:
		case RS_RECEIVE:
			_state = receiving(RS_RECEIVE);
			break;
		case RS_READY:
			Serial.println("READY");
			if (inSeq == 0 || _buf.header.seq == 0) {
				inSeq = 0;
			}
			if (_buf.header.seq > inSeq || _buf.header.seq == 0) {
				inSeq = _buf.header.seq;
				processPacketSlave();
			} else {
				Serial.println("SEQ out of sync");
				_state = RS_OK;
			}
			break;
		case RS_OK:
			fillFrame(C_OK,"OK");
			_state = send();
			break;
		case RS_CRC_ERROR:
			fillFrame(C_BADCRC,"CRC");
			_state = send();
			break;
		case RS_ERROR:
			fillFrame(C_ERROR,"ERR");
			_state = send();
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
		return _state;
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
			if (millis() - _start > MAX_SEND_TIME) {	// Check if no responce on sent packet within timeout period
				_state = RS_ERROR;		// Generate error if timeout reached
			} else {
				_state = receiving(RS_RECEIVE);
			}
			break;
		case RS_READY:
			if (_buf.header.command == C_BADCRC) {
				_state = RS_ERROR;		// Generate error on CRC mismach
				Serial.println("CRC Error");
			} else {
				_state = this->processPacketMaster();
				//_state = RS_IDLE;
				Serial.println("Ready");
			}
			break;
		case RS_ERROR:
			_retryCount++;
			if (_retryCount >  MAX_RETRY_COUNT) {	// Retry packet send until max retry count reached
				_retryCount = 0;
				Serial.println("Resend filed");
				_state = RS_FAILED;
			} else {
			Serial.println("Resending");
				_state = send();
			}
			break;
		case RS_TIMEOUT:
		case RS_IDLE:
		case RS_FAILED:
			clean();
		default:
			;
			//Serial.println("Unknown state");
		}
		return _state;
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
		//debugPrintPacket();
		if (_buf.header.command == C_BADCRC) {
			Serial.println("Bad CRC");
			return send();
		}
		return RS_IDLE;
	}
	virtual status_t processPacketSlave() {
		//debugPrintPacket();
		if (_buf.header.command == C_PING) {
			memcpy(&_reply, &_buf, _buf.header.dataSize + sizeof(packetHeader));
			return send();
		}
		_state = RS_OK;
		return RS_OK;
	}
	bool isIdle() {
		return _state == RS_IDLE || _state == RS_ERROR || _state == RS_FAILED;
	}
	void debugPrintPacket() {
	return;
		for (uint8_t i = 0; i < _buf.header.dataSize; i++) {
			Serial.printf("%c", (char)(_buf.raw[sizeof(packetHeader) + i]));
			Serial.print(" ");
		}
		Serial.println();
	}
	// Prepare frame to send chat*
	bool fillFrame(uint8_t com, const char* data, uint8_t slaveId = 0) {
		return fillFrame(com, (uint8_t*)data, strlen(data), slaveId);
	}
	// Prepare frame to send binary data
	bool fillFrame(uint8_t com, const uint8_t* data, uint16_t len, uint8_t slaveId = 0) {
		if (len > FRAME_LENGTH - sizeof(packetHeader) - sizeof(uint32_t)) return false;
		memcpy(_reply.header.sig, _sig, MAGIC_SIG_LENGTH);
		_reply.header.id = slaveId;
		_reply.header.seq = _seq;
		_seq++;
		_reply.header.command = com;
		_reply.header.dataSize = len + sizeof(uint32_t);
		memcpy(&_reply.raw[sizeof(packetHeader)], data, len);
		uint32_t curCrc = crc(&_reply);
		memcpy(&_reply.raw[len + sizeof(packetHeader)], &curCrc, sizeof(uint32_t));
		_pos = 0;
		return true;
	}
	// Prepare and fill frame with file content
	bool fillFrame(uint8_t com, File data, uint8_t slaveId = 0) {
		memcpy(_reply.header.sig, _sig, MAGIC_SIG_LENGTH);
		_reply.header.id = slaveId;
		_reply.header.seq = _seq;
		_seq++;
		_reply.header.command = com;
		uint16_t len = sizeof(packetHeader);
		while (data.available() && len < FRAME_LENGTH - sizeof(packetHeader) - sizeof(uint32_t)) {
			_reply.raw[len] = data.read();
			len++;
		}
		len -= sizeof(packetHeader);
		_reply.header.dataSize = len + sizeof(uint32_t);
		uint32_t curCrc = crc(&_reply);
		memcpy(&_reply.raw[len + sizeof(packetHeader)], &curCrc, sizeof(uint32_t));
		_pos = 0;
		return true;
	}
/*
	template <typename R> bool fillFrame(uint8_t com, const R &data, uint8_t slaveId = 0) {
		return fillFrame(com, (uint8_t*)data, sizeof(data));
	}
*/
	void fillSeq(uint32_t seqq = 0) {
		this->_reply.header.seq = seqq;
	}
protected:
	uint8_t		_id = 0;	// Slave ID
	uint16_t	_pos = 0;	// Current sending/receiving block position
	status_t	_state = RS_IDLE;	// Transmittion state
	uint8_t		_sig[MAGIC_SIG_LENGTH] = MAGIC_SIG;
	uint8_t		_retryCount = 0;	// Current sending retry count
	packetFrame	_buf;			// Receive buffer
	packetFrame	_reply;			// Send buffer
	T*			_serial;	// Serial port pointer
	uint16_t	_txPin = -1;		// RS-485 flow control pin
	uint32_t	_start = 0;		// Current send operation start time
	uint32_t	_seq = 0;	// Packet sequence #
	uint32_t	inSeq = 0;	// incoming packet sequence #

	// Returns packet data CRC32
	uint32_t crc(packetFrame* data) {
		return crc32_le(0, (uint8_t*)data, data->header.dataSize - sizeof(uint32_t));
	}
	
	void enableSend() {			// Switch to sender mode for RS-485
		if (_txPin >= 0) {
			//_serial->flush();
			//delay(100);
			digitalWrite(_txPin, HIGH);
			delay(20);
			//delay(100);
		}
	}
	
	void enableReceive() {		// Switch to receiver mode for RS-485
		if (_txPin >= 0) {
			//delay(10);
			//_serial->flush();
			//delay(200);
			digitalWrite(_txPin, LOW);
			//delay(100);
			delay(20);
			//_serial->flush();
		}
	}
	
	uint32_t extractCrc() {		// Returns CRC value from packet
		uint32_t cr = _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 3];
		cr = cr << 8;
		cr += _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 2];
		cr = cr << 8;
		cr += _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 1];
		cr = cr << 8;
		cr += _buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t) + 0];
		return cr;
	}
	
	status_t receiving(status_t defaultState = RS_IDLE) {	// Receiving incoming data to buffer
		enableReceive();
		status_t state = defaultState;
		//Serial.print(".");
		while (_serial->available()) {
			_buf.raw[_pos] = _serial->read();
			if (_pos < MAGIC_SIG_LENGTH && _buf.raw[_pos] != _sig[_pos]) {
				_pos = 0;
				if (_buf.raw[_pos] != _sig[_pos]) {
					//Serial.print("d");
					//Serial.print(_buf.raw[_pos], HEX);
					continue;
				}
			}
			//Serial.print("r");
			//Serial.print(_buf.raw[_pos], HEX);
			_pos++;
			if ((_pos >= sizeof(packetHeader) && _pos >= _buf.header.dataSize + sizeof(packetHeader)) || _pos >= FRAME_LENGTH) {
				//_serial->flush();
				_pos = 0;
				if (_buf.header.id == this->_id) {		// Check destination ID of packet
					if (_buf.header.dataSize >= FRAME_LENGTH) {
						Serial.println("Too long");
						state = RS_ERROR;
						break;
					}
					if (crc(&_buf) != extractCrc()) {	// Check received data CRC
						Serial.println();
						Serial.println(extractCrc(), HEX);
						Serial.println(crc(&_buf), HEX);
						Serial.println("CRC error");
						state = RS_CRC_ERROR;
						break;
					}
					Serial.println("Got packet");
					state = RS_READY;
				} else {
					Serial.println("Alien packet");
					state = RS_IDLE;
				}
				//debugPrintPacket();
				break;
			}
		}
		return state;
	}
	status_t sending(status_t defaultState = RS_SEND) {		// Send outgoing data buffer
		while (_serial->available()) {
			_serial->read();
		}
		delay(20);
		enableSend();
		status_t state = defaultState;
		uint16_t dSize = sizeof(packetHeader) + _reply.header.dataSize;
		time_t startTransfer = millis();
		while (_pos < dSize) {
		#ifdef RS_BYTE_DELAY
			delayMicroseconds(RS_BYTE_DELAY);
		#endif
			//Serial.print("s");
			//Serial.print(_reply.raw[_pos], HEX);
			_serial->write(_reply.raw[_pos]);
			_pos++;
		}
		//_serial->flush();
		//int32_t flushTime = (500 * _pos / 1024) - (millis() - startTransfer);
		//Serial.println(flushTime);
		//if (flushTime > 0) delay(flushTime);
		delay(10);
		_pos = 0;
		return RS_DONE;
	}
};