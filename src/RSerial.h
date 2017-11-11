#define MAGIC_SIG {0xFF, 0xEE, 0xDD, 0xCC}
#define MAGIC_SIG_LENGTH 4
#define FRAME_LENGTH 1024
#define MAX_RETRY_COUNT	5
#define RESPONSE_TIMEOUT 10000
#define MAX_SEND_TIME 10000

//For SoftwareSerial
#define ERX D1
#define ETX D4

struct packetHeader {
	uint8_t sig[MAGIC_SIG_LENGTH];
	uint8_t id;
	uint8_t flags;
	uint8_t command;
	uint16_t dataSize;
};
#define C_OK 1
#define C_ERROR 2
#define C_PING 3

union packetFrame {
	packetHeader header;
	uint8_t raw[FRAME_LENGTH];
};

enum status_t {RS_IDLE, RS_SEND, RS_WAIT, RS_OK, RS_RECEIVE, RS_READY, RS_ERROR, RS_TIMEOUT, RS_FAILED};

template <typename T> class RSerial {
public:
	RSerial(T* serial) {
		_serial = serial;
	}
	void taskSlave() {
		switch (_state) {
		case RS_IDLE:
		case RS_RECEIVE:
			_state = receiving();
			break;
		case RS_READY:
			_state = processPacketSlave();
			break;
		case RS_OK:
			fillFrame(C_OK, "OK");
			_state = send();
			break;
		case RS_ERROR:
			fillFrame(C_ERROR, "ERROR");
			_state = send();
			break;
		case RS_SEND:
			_state = sending();
			if (_state == RS_WAIT)
				_state = RS_IDLE;
			break;
		default:
			Serial.println("Unknown state");
		}
	}
	status_t taskMaster() {
		switch (_state) {
		case RS_SEND:
			_state = sending();
			break;
		case RS_RECEIVE:
		case RS_WAIT:
			_state = receiving();
			break;
		case RS_READY:
			_state = processPacketMaster();
			break;
		case RS_ERROR:
		case RS_TIMEOUT:
			_retryCount++;
			if (_retryCount < MAX_RETRY_COUNT) {
				_state = send();
				break;
			}
			Serial.println("Failed");
			receive();
			_state = RS_FAILED;
		case RS_IDLE:
		case RS_FAILED:
			break;
		default:
			Serial.println("Unknown state");
		}
	}
	uint32_t crc(uint16_t len) {
		return 0x11223344;
	}
	status_t send() {
		digitalWrite(ERX, HIGH);
		digitalWrite(ETX, HIGH);
		_pos = 0;
		_retryCount = 0;
		_state = RS_SEND;
		return _state;
	}
	status_t receive() {
		_pos = 0;
		digitalWrite(ERX, LOW);
		digitalWrite(ETX, LOW);
		_state= RS_IDLE;
		return _state;
	}
	status_t processPacketMaster() {
		debugPrintPacket();
		receive();
		return RS_IDLE;
	}
	status_t processPacketSlave() {
		debugPrintPacket();
		if (_buf.header.command == C_PING) {
			memcpy(&_buf, &_reply, _buf.header.dataSize);
			return send();
		}
		return RS_OK;
	}
	bool isIdle() {
		return _state == RS_IDLE || _state == RS_ERROR;
	}
	void debugPrintPacket() {
		for (uint8_t i = 0; i < _buf.header.dataSize; i++) {
			Serial.print(_buf.raw[sizeof(packetHeader) + i], HEX);
			Serial.print(" ");
		}
		Serial.println();
	}
	bool fillFrame(uint8_t com, char* data) {
		uint16_t len = strlen(data);
		if (len > FRAME_LENGTH - sizeof(packetHeader)) return false;
		memcpy(_reply.header.sig, _sig, MAGIC_SIG_LENGTH);
		_reply.header.command = com;
		_reply.header.dataSize = len + sizeof(uint32_t);
		memcpy(&_reply.raw[sizeof(packetHeader)], data, len);
		uint32_t curCrc = crc(len + sizeof(packetHeader));
		memcpy(&_reply.raw[len + sizeof(packetHeader)], &curCrc, sizeof(uint32_t));
		_pos = 0;
		return true;
	}
protected:
	uint32_t	_start;
	uint8_t		_id = 0;
	uint16_t	_pos = 0;
	status_t	_state = RS_IDLE;
	uint8_t		_sig[MAGIC_SIG_LENGTH] = MAGIC_SIG;
	uint8_t		_retryCount = 0;
	packetFrame	_buf;
	packetFrame	_reply;
	T*			_serial;
	status_t prepareReply() {
		return RS_SEND;
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
	status_t receiving() {
		uint32_t start = millis();
digitalWrite(ERX, LOW);
digitalWrite(ETX, LOW);
		status_t state = RS_IDLE;
		while (_serial->available()) {
			//if (start - millis() > MAX_SEND_TIME)
			//	return RS_RECEIVE;
			_buf.raw[_pos] = _serial->read();
			//Serial.print(_buf.raw[_pos], HEX);
			//Serial.print(" ");
			if (_pos < MAGIC_SIG_LENGTH && _buf.raw[_pos] != _sig[_pos]) {
				_pos = 0;
				Serial.println();
				continue;
			}
			_pos++;
			
			if ((_pos > sizeof(packetHeader) && _pos >= _buf.header.dataSize + sizeof(packetHeader)) || _pos >= FRAME_LENGTH) {
				_serial->flush();
				_pos = 0;
				//Serial.println();
				if (_buf.header.dataSize >= FRAME_LENGTH || crc(_buf.header.dataSize) != extractCrc()) {
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
	status_t sending() {
		uint32_t start = millis();
digitalWrite(ERX, HIGH);
digitalWrite(ETX, HIGH);
		while (_pos < sizeof(packetHeader) + _reply.header.dataSize) {
			//if (start - millis() > MAX_SEND_TIME)
			//	return RS_SEND;
			//Serial.print(_reply.raw[_pos], HEX);
			//Serial.print(" ");
			_serial->write(_reply.raw[_pos]);
			//_serial->print(_reply.raw[_pos], HEX);
			_pos++;
		}
		_pos = 0;
		return RS_WAIT;
	}
};