#define MAGIC_SIG {0xFF, 0xEE, 0xDD, 0xCC}
#define MAGIC_SIG_LENGTH 4
#define FRAME_LENGTH 1024

struct packetHeader {
	uint8_t sig[MAGIC_SIG_LENGTH];
	uint8_t id;
	uint8_t flags;
	uint8_t command;
	uint16_t dataSize;
};
union packetFrame {
	packetHeader header;
	uint8_t raw[FRAME_LENGTH];
};

enum status_t {RS_IDLE, RS_SEND, RS_WAIT, RS_RECEIVE, RS_READY, RS_ERROR};
template <typename T> class RSerial {
public:
	RSerial(T* serial) {
		_serial = serial;
	}
	void taskSlave() {
		//if (_state == RS_READY) {
		//	_state = prepareReply();
		//	_state = send();
		//}
		_state = receive();
	}
	status_t taskMaster() {
		if (send() == RS_IDLE) _pos = 0;
	}
	uint32_t crc(uint16_t len) {
		return 0x11223344;
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
	
	uint8_t		_id = 0;
	uint16_t	_pos = 0;
	status_t	_state = RS_IDLE;
	uint8_t		_sig[MAGIC_SIG_LENGTH] = MAGIC_SIG;
	packetFrame	_buf;
	packetFrame	_reply;
	T*			_serial;
	status_t prepareReply() {
		return RS_SEND;
	}
	status_t receive() {
		status_t state = RS_IDLE;
		while (_serial->available()) {
			_buf.raw[_pos] = _serial->read();
			Serial.print(_buf.raw[_pos], HEX);
			Serial.print(" ");
			if (_pos < MAGIC_SIG_LENGTH && _buf.raw[_pos] != _sig[_pos]) {
				_pos = 0;
				Serial.println();
				continue;
			}
			_pos++;
			
			if ((_pos > sizeof(packetHeader) && _pos >= _buf.header.dataSize + sizeof(packetHeader)) || _pos >= FRAME_LENGTH) {
				_serial->flush();
				_pos = 0;
				Serial.println();
				if (_buf.header.dataSize >= FRAME_LENGTH || crc(_buf.header.dataSize) != (uint32_t)_buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t)]) {
					state = RS_ERROR;
					break;
				}
				state = RS_READY;
				break;
			}
		}
		return state;
	}
	status_t send() {
		while (_pos < sizeof(packetHeader) + _reply.header.dataSize) {
			_serial->write(_reply.raw[_pos]);
			//_serial->print(_reply.raw[_pos], HEX);
			_pos++;
		}
		return RS_IDLE;
	}
};


/*
	bool taskSlave2() {
		bool _error = false;
		while (_serial->available()) {
			_buf.raw[_pos] = _serial->read();
			if (_pos < MAGIC_SIG_LENGTH && _buf.raw[_pos] != _sig[_pos]) {
				_pos = 0;
				continue;
			}
			_pos++;
			if ((_pos >= sizeof(packetHeader) && _pos >= _buf.header.dataSize + sizeof(packetHeader)) || _pos >= FRAME_LENGTH) {
				_serial->flush();
				if (_buf.header.dataSize > FRAME_LENGTH || crc(_buf.header.dataSize) != (uint32_t)_buf.raw[_buf.header.dataSize + sizeof(packetHeader) - sizeof(uint32_t)]) {
					_error = true;
					break;
				}
				if (prepareReply()) {
					_pos = 0;
					send();
				}
			}
		}
	}
*/