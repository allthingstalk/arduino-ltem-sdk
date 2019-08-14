#include "LTEmModem.h" 
#include "Utils.h"

#include<String.h>

static inline bool is_timedout(uint32_t from, uint32_t nr_ms) __attribute__((always_inline));
static inline bool is_timedout(uint32_t from, uint32_t nr_ms)
{
    return (millis() - from) > nr_ms;
}

typedef struct NameValuePair {
    const char* Name;
    bool Value;
} NameValuePair;

const uint8_t nConfigCount = 3;
static NameValuePair nConfig[nConfigCount] = {
    { "AUTOCONNECT", true },
    { "CR_0354_0338_SCRAMBLING", true },
    { "CR_0859_SI_AVOID", true },
    //{ "COMBINE_ATTACH", false },
    //{ "CELL_RESELECTION", false },
    //{ "ENABLE_BIP", false },
};

LTEmModem::LTEmModem(HardwareSerial &serial, Stream &monitor, APICredentials &credentials, bool fullDebug, ATT_CALLBACK_SIGNATURE) : Device()
{
	this->_serial = &serial;
	this->_monitor = &monitor;
	this->_credentials = &credentials;
	
	this->_inputBufferSize = 256;
	this->_callback = callback;
	this->_fullDebug = fullDebug;
	
	this->_txEnablePin = -1; //txEnablePin;
	this->_cid = 1; //cid;
	
	#ifdef _VARIANT_SODAQ_SFF_
		this->_onOffPin = 49; //onoffPin;		
		this->_saraR4XXTogglePin = 15; //saraR4XXTogglePin;
	#else 
		#ifdef _VARIANT_SODAQ_SARA_
			this->_onOffPin = 27; //onoffPin;
			this->_saraR4XXTogglePin = 52; //saraR4XXTogglePin;
		#else
			this->_onOffPin = 35;
			this->_saraR4XXTogglePin = 31;
		#endif	
	#endif
}

bool LTEmModem::init(char *apn)
{
	this->_apn = apn;
	
	_serial->begin(getDefaultBaudRate());
	while ((!_serial)) {}
	
	_monitor->println("Init Buffer");
	initBuffer(); // safe to call multiple times

	_monitor->println("Setting on/off pin");
    init(_onOffPin, _saraR4XXTogglePin);

	_monitor->println("Setting TX Enable Pin");
    setTxEnablePin(_txEnablePin);
	
	_monitor->println("Setting ON");
	if(!on())
		return false;
	
	purgeAllResponsesRead(); //deleting all buffer data
	
	println("AT+CGSN"); //getting imei number
	if (readResponse() != ResponseOK)
		return false;
	
	_monitor->print("IMEI number: ");
	_monitor->println(_imei);
	
	println("AT+CCID"); //getting iccid number
	if (readResponse() != ResponseOK)
		return false;
	
	_monitor->print("ICCID number: ");
	_monitor->println(_iccid);
	
	println("AT+CIMI"); //getting cimi number
	if (readResponse() != ResponseOK)
		return false;
	
	_monitor->print("IMSI number: ");
	_monitor->println(_cimi);
	
	println("AT+CGMR"); //getting firmware version
	if (readResponse() != ResponseOK)
		return false;
	
	_monitor->print("Firmware version: ");
	_monitor->println(_firmware);
	
	println("AT+CEDRXS=0");
	if (readResponse() != ResponseOK)
		return false;
	
	println("ATE0");
	if (readResponse() != ResponseOK)
		return false;
	
	println("AT+URAT=7");
	if (readResponse() != ResponseOK)
		return false;
	
	println("AT+CMEE=2");
	if (readResponse() != ResponseOK)
		return false;
	
	_monitor->println("Set radio active");
	if(!setRadioActive(true))
		return false;
	
	connect();
	
//	if (!initTCP())
//		return false;
	
	if (!initMQTT())
		return false;
	
	return true;
}

void LTEmModem::disconnect()
{
	println("AT+CGATT=0");
	readResponse();
}

void LTEmModem::init(int onoffPin, int8_t saraR4XXTogglePin)
{
    if (_onOffPin >= 0) {
        // First write the output value, and only then set the output mode.
        digitalWrite(_onOffPin, LOW);
        pinMode(_onOffPin, OUTPUT);
    }

    // always set this because its optional and can be -1
    if (_saraR4XXTogglePin >= 0) {
        pinMode(_saraR4XXTogglePin, OUTPUT);
    }
}

bool LTEmModem::initTCP()
{
	_monitor->println("Trying to make tcp connection");
	
	purgeAllResponsesRead(); // Remove all data on buffer
		
	int retryCount = 1;
	int localPort = random(10000) + 10000;
	
	int socketID = createTCPSocket(localPort);
	
	while (socketID == SOCKET_FAIL && retryCount <= 5)
	{
		_monitor->println("Retrying...");
		delay(500);
		socketID = createTCPSocket(localPort);
		retryCount++;
	}
	
	if (socketID != SOCKET_FAIL)
	{	
		_monitor->println("Setting up TCP");
		return setUpTCP(socketID, _credentials->getSpace(), 1883); 
		//return setUpTCP(socketID, "m15.cloudmqtt.com", 11206); 
	}
	else 
		return false;
}

bool LTEmModem::initMQTT()
{
	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
	println("AT+UMQTT=12,1"); //clean mqtt session
	if (readResponse<int, int>(_umqttParser, &value1, &value2) == ResponseOK)
	{		
		ok = value1 == 12 && value2 == 1; 
	}
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
	
	value1 = 0;
	value2 = 0;
	println("AT+UMQTTC=7,0");
	
	if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 7 && value2 == 1;
    }
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
	
    print("AT+UMQTT=");
    print('0');
    print(',');
    print("\"");
    print("test");
    print("\"");
	println();
	
	///wait until you receive UMQTT:0,1
	
	if (readResponse<int, int>(_umqttParser, &value1, &value2) == ResponseOK)
	{		
		ok = value1 == 0 && value2 == 1;
	}
    
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
	
	ok = false;
	value1 = 0;
	value2 = 0;

    print("AT+UMQTT=");
    print('1');
    print(',');
	//print(11206);
    print(1883);
	println();
	
	///wait until you receive UMQTT:1,1
	if (readResponse<int, int>(_umqttParser, &value1, &value2) == ResponseOK)
	{	
		ok = value1 == 1 && value2 == 1;
	}
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
	
	
	ok = false;
	value1 = 0;
	value2 = 0;
	
    print("AT+UMQTT=");
    print('2');
    print(',');
    print("\"");
	//print("m15.cloudmqtt.com");
    print(_credentials->getSpace());
    print("\"");
    print(',');
	//print(11206);
    print(1883);
	println();
	
	///wait until you receive UMQTT:2,1
	if (readResponse<int, int>(_umqttParser, &value1, &value2) == ResponseOK)
	{	
		ok = value1 == 2 && value2 == 1;
	}
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
	
	ok = false;
	value1 = 0;
	value2 = 0;
	
    print("AT+UMQTT=");
    print('4');
    print(',');
    print("\"");
	//print("zlozpmdl");
    print(_credentials->getDeviceToken());
    print("\"");
    print(',');
    print("\"");
	//print("n1vyWdAKiX8v");
    print("password");
    print("\"");
    //print(";");
	println();
	
	///wait until you receive UMQTT:4,1
	if (readResponse<int, int>(_umqttParser, &value1, &value2) == ResponseOK)
	{	
		ok = value1 == 4 && value2 == 1;
	}
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
	
	/*ok = false;
	value1 = 0;
	value2 = 0;
	println("AT+UMQTTWTOPIC=2,1,256"); //set QoS 2(publish exactly once), Will retain 1(remain on broker), max msg length
	
	if (readResponse<int, int>(_umqttTopicParser, &value1, &value2) == ResponseOK)
	{	
		ok = value1 == 1;
	}
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}*/	
	
	ok = false;
	value1 = 0;
	value2 = 0;
	
    print("AT+UMQTT=");
    print('1');
    print('0');
    print(',');
    print('0');
	println();
	
	///wait until you receive UMQTT:10,1
	if (readResponse<int, int>(_umqttParser, &value1, &value2) == ResponseOK)
	{	
		ok = value1 == 10 && value2 == 1;
	}
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
	
	ok = false;
	value1 = 0;
	value2 = 0;
	
	println("AT+UMQTTC=1");
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 1 && value2 == 1;
    }
	
	if (!ok)
	{
		_monitor->println("Error");
		return false;
	}
		
	ok = false;
	value1 = 0;
	value2 = 0;
	
	readResponse<int, int>(_uumqttcParser, &value1, &value2); //Don't check on ResponseOK, because you get only get +UUMQTTC 1,0 without an OK
	ok = value1 == 1 && value2 == 0;
	
	return ok;
}

bool LTEmModem::mqttConnected()
{
	print("AT+UMQTTC=8,\"");
	print(_credentials->getSpace());
	println("\"");
	
	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 8 && value2 == 1;
    }
	
	if (!ok)
	{
		println("AT+UMQTTER");
		readResponse();
	}
	
	return ok;
}

bool LTEmModem::listen(char* actuator)
{
	char buffer[255];
	
	//sprintf(buffer, "device/%s/asset/%s/feed", _credentials->getDeviceId(), actuator);
	sprintf(buffer, "device/%s/asset/%s/command", _credentials->getDeviceId(), actuator);
	
	return subscribeMqttMessage(buffer);
}

void LTEmModem::process()
{
	readMqttMessage();
}

void LTEmModem::initBuffer()
{
    // make sure the buffers are only initialized once
    if (!_isBufferInitialized) {
        this->_inputBuffer = static_cast<char*>(malloc(this->_inputBufferSize));

        _isBufferInitialized = true;
    }
}

void LTEmModem::setTxEnablePin(int8_t txEnablePin)
{
     if (_txEnablePin != -1) {
        pinMode(_txEnablePin, OUTPUT);
        digitalWrite(_txEnablePin, LOW);
    }
}

bool LTEmModem::connect()
{	
	print("AT+CGDCONT=1,\"IP\",\"");
	print(_apn);
	println("\"");

	println("AT+CGATT?"); 
	
	_monitor->println("Waiting for connection");
	while (!isConnected()) 
	{
		_monitor->print(".");
		delay(1000);
	}
	_monitor->println();
	
	return true;
}

bool LTEmModem::publishMqttMessage(const char* topic, double value)
{
	purgeAllResponsesRead();
	
    print("AT+UMQTTC=2,0,0,\"");
    print(topic);
    print("\",");
    print("\"{\"value\":");
    print(value, 2);
    print("}\"");
    println();

	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 2 && value2 == 1;
    }
	
	if (!ok)
		_monitor->println("Could not publish message");
	
	return ok;
}

bool LTEmModem::publishMqttMessage(const char* topic, int value)
{
	purgeAllResponsesRead();
	
    print("AT+UMQTTC=2,0,0,\"");
    print(topic);
    print("\",");
    print("\"{\"value\":");
    print(value);
    print("}\"");
    println();

	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 2 && value2 == 1;
    }
	
	if (!ok)
		_monitor->println("Could not publish message");
	
	return ok;
}

bool LTEmModem::publishMqttMessage(const char* topic, char* value, bool json)
{
	purgeAllResponsesRead();
	
    print("AT+UMQTTC=2,0,0,\"");
	
	if (json)
	{
		print("device/");
		print(_credentials->getDeviceId());
		print("/asset/");
		print(topic);
		print("/state");
		print("\",\"");
		print(value);
		print("\"");
	}
	else
	{
		print(topic);
		print("\",");
		print("\"{\"value\":\"");
		print(value);
		print("\"}\"");
	}
    
    println();

	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 2 && value2 == 1;
    }
	
	if (!ok)
		_monitor->println("Could not publish message");
	
	return ok;
}

bool LTEmModem::publishMqttMessage(const char* topic, unsigned char* value, int size)
{
	purgeAllResponsesRead();
	
    print("AT+UMQTTC=2,0,1,1,\"");
    print(topic);
    print("\",\"");
	
	for (int i = 0; i < size; ++i)
	{
		//print(value[i]);
		print(static_cast<char>(NIBBLE_TO_HEX_CHAR(HIGH_NIBBLE(value[i]))));
		print(static_cast<char>(NIBBLE_TO_HEX_CHAR(LOW_NIBBLE(value[i]))));
	}

	print("\"");
    println();

	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 2 && value2 == 1;
    }
	
	if (!ok)
		_monitor->println("Could not publish message");
	
	return ok;
}

bool LTEmModem::publishMqttMessage(const char* topic, GeoLocation* geoLocation)
{
	purgeAllResponsesRead();
	
    print("AT+UMQTTC=2,0,0,\"");
    print(topic);
    print("\",");
    print("\"{\"value\":{\"latitude\": ");
    print(geoLocation->latitude, 6);
    print(", \"longitude\": ");
	print(geoLocation->longitude, 6);
	print(", \"altitude\": ");
	print(geoLocation->altitude, 6);
	print("}}\"");
    println();

	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 2 && value2 == 1;
    }
	
	if (!ok)
		_monitor->println("Could not publish message");
	
	return ok;
}



bool LTEmModem::subscribeMqttMessage(const char* topic)
{
	purgeAllResponsesRead();
	
    print("AT+UMQTTC=4,0,\"");
    print(topic);
    println("\"");

	bool ok = false;
    int value1 = 0;
	int value2 = 0;
	
    if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 4 && value2 == 1;
    }
	
	if (!ok)
		return false;
	
	ok = false;
    value1 = 0;
	value2 = 0;
	
	readResponse<int, int>(_uumqttcParser, &value1, &value2); //Don't check on ResponseOK, because you get only get +UUMQTTC 4,1 without an OK
	ok = value1 == 4 && value2 == 1;
	
	return ok;
}

bool LTEmModem::readMqttMessage()
{
	bool ok = false;
	int value1 = 0;
	int value2 = 0;
	
    println("AT+UMQTTC=6");
	
	if (readResponse<int, int>(_umqttcParser, &value1, &value2) == ResponseOK) {
        ok = value1 == 6 && value2 == 1;
	}
	
	return ok;
}

void LTEmModem::purgeAllResponsesRead()
{
    uint32_t start = millis();

    // make sure all the responses within the timeout have been read
    while ((readResponse(0, 1000) != ResponseTimeout) && !is_timedout(start, 2000)) {}
}

bool LTEmModem::setRadioActive(bool on)
{
    print("AT+CFUN=");
    println(on ? "1" : "0");

    return (readResponse() == ResponseOK);
}

bool LTEmModem::isAlive()
{
    println("AT");

    return (readResponse(NULL, 450) == ResponseOK);
}

void LTEmModem::reboot()
{
    println("AT+CFUN=15"); // reset modem + sim
    
    // wait up to 2000ms for the modem to come up
    uint32_t start = millis();

    while ((readResponse() != ResponseOK) && !is_timedout(start, 5000)) { }
}

bool LTEmModem::isConnected()
{
    uint8_t value = 0;

    println("AT+CGATT?");

    if (readResponse<uint8_t, uint8_t>(_cgattParser, &value, NULL, 0, 10 * 1000) == ResponseOK) 
	{
        return (value == 1);
	}
    else
		return false;
}

// Turns the modem on and returns true if successful.
bool LTEmModem::on()
{
	if (_onOffPin >= 0) {
        digitalWrite(_onOffPin, HIGH);
    }
	
	if (_saraR4XXTogglePin >= 0) {
        pinMode(_saraR4XXTogglePin, OUTPUT);
        digitalWrite(_saraR4XXTogglePin, LOW);
        delay(2000);
        pinMode(_saraR4XXTogglePin, INPUT);
    }

    _startOn = millis();

    if (!isOn()) {
        if (_onoff) {
            _onoff->on();
        }
    }

	setTxPowerIfAvailable(true);

    // wait for power up
    bool timeout = true;

    for (uint8_t i = 0; i < 15; i++) {
        if (isAlive()) {
            timeout = false;
            break;
        }
    }

    if (timeout) {
        _monitor->println("Error: No Reply from Modem");
        return false;
    }

    return isOn(); // this essentially means isOn() && isAlive()
}

// Turns the modem off and returns true if successful.
bool LTEmModem::off()
{
    // No matter if it is on or off, turn it off.
    if (_onoff) {
        _onoff->off();
    }

    setTxPowerIfAvailable(false);

    return !isOn();
}

// Returns true if the modem is on.
bool LTEmModem::isOn() const
{
    if (_onoff) {
        return _onoff->isOn();
    }

    // No onoff. Let's assume it is on.
    return true;
}

void LTEmModem::setTxPowerIfAvailable(bool on)
{
	_monitor->print("Setting TX power to: ");
	_monitor->println(on);
	
    if (_txEnablePin != -1) {
        digitalWrite(_txEnablePin, on);
    }
}

int LTEmModem::createSocket(uint16_t localPort)
{
	print("AT+USOCR=17,");
	println(localPort);
    
    uint8_t socket;

    if (readResponse<uint8_t, uint8_t>(_createSocketParser, &socket, NULL) == ResponseOK) {
        return socket;
    }

    return SOCKET_FAIL;
}

int LTEmModem::createTCPSocket(uint16_t localPort)
{
	int value1 = 0;
	char* value2;
		
	println("AT+CGPADDR=1");
	
	if (readResponse<int, char*>(_cgpAddrParser, &value1, &value2) == ResponseOK)
	{
		_ipAddress = value2;
		_monitor->print("your ip-address: ");
		_monitor->println(_ipAddress);
	}

	print("AT+USOCR=6,");
	println(localPort);

	uint8_t socket;

	if (readResponse<uint8_t, uint8_t>(_createSocketParser, &socket, NULL) == ResponseOK) 
	{
		return socket;
	}

	return SOCKET_FAIL;
}

bool LTEmModem::setUpTCP(uint8_t socket, const char* remoteIP, const uint16_t remotePort)
{
    print("AT+USOCO=");
    print(socket);
    print(',');
    print("\"");
    print(remoteIP);
    print("\"");
    print(',');
    println(remotePort);

    return readResponse() == ResponseOK;
}

ResponseTypes LTEmModem::_createSocketParser(ResponseTypes& response, const char* buffer, size_t size,uint8_t* socket, uint8_t* dummy)
{
    if (!socket) {
        return ResponseError;
    }

    int socketID;

    if (sscanf(buffer, "%d", &socketID) == 1) {
        if (socketID <= UINT8_MAX) {
            *socket = socketID;
        }
        else {
            return ResponseError;
        }

        return ResponseEmpty;
    }

    if (sscanf(buffer, "+USOCR: %d", &socketID) == 1) {
        if (socketID <= UINT8_MAX) {
            *socket = socketID;
        }
        else {
            return ResponseError;
        }

        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_nconfigParser(ResponseTypes& response, const char* buffer, size_t size, bool* nconfigEqualsArray, uint8_t* dummy)
{
    if (!nconfigEqualsArray) {
        return ResponseError;
    }

    char name[32];
    char value[32];

    if (sscanf(buffer, "+NCONFIG: \"%[^\"]\",\"%[^\"]\"", name, value) == 2) {
        for (uint8_t i = 0; i < nConfigCount; i++) {
            if (strcmp(nConfig[i].Name, name) == 0) {
                if (strcmp(nConfig[i].Value ? "TRUE" : "FALSE", value) == 0) {
                    nconfigEqualsArray[i] = true;

                    break;
                }
            }
        }

        return ResponsePendingExtra;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_csqParser(ResponseTypes& response, const char* buffer, size_t size, int* rssi, int* ber)
{
    if (!rssi || !ber) {
        return ResponseError;
    }

    if (sscanf(buffer, "+CSQ: %d,%d", rssi, ber) == 2) {
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_cgattParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* result, uint8_t* dummy)
{
    if (!result) {
        return ResponseError;
    }

    int val = 0;

    if (sscanf(buffer, "+CGATT: %d", &val) == 1) {
        *result = val;
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_umqttcParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* value2)
{
	if (!value1 || !value2) {
        return ResponseError;
    }

    if (sscanf(buffer, "+UMQTTC: %d,%d", value1, value2) == 2) {		
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_uumqttcParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* value2)
{
	if (!value1 || !value2) {
        return ResponseError;
    }

    if (sscanf(buffer, "+UUMQTTC: %d,%d", value1, value2) == 2) {		
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_umqttParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* value2)
{
	if (!value1 || !value2) {
        return ResponseError;
    }

    if (sscanf(buffer, "+UMQTT: %d,%d", value1, value2) == 2) {
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_umqttTopicParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* dummy)
{
	if (!value1) {
        return ResponseError;
    }

    if (sscanf(buffer, "+UMQTTWTOPIC: %d", value1) == 1) {		
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_cedrxParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, char** value2)
{
	if (!value1 || !value2) {
        return ResponseError;
    }
	
	if (sscanf(buffer, "CEDRXP: %d,%s", value1, value2) == 2) {
		return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes LTEmModem::_cgpAddrParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, char** value2)
{
	if (!value1 || !value2) {
        return ResponseError;
    }

	uint8_t part1;
	uint8_t part2;
	uint8_t part3;
	uint8_t part4;
	
	if (sscanf(buffer, "+CGPADDR: %d,%u.%u.%u.%u", value1, part1, part2, part3, part4) == 5) {
		char buf[15];
		char* buf2;
		
		snprintf(buf, 15, "%u.%u.%u.%u", part1, part2, part3, part4);
		buf2 = &buf[0];
		value2 = &buf2;
				
		return ResponseEmpty;
    }
	
    return ResponseError;
}

size_t LTEmModem::print(const char buffer[])
{
    if (_fullDebug) _monitor->print(buffer);

    return _serial->print(buffer);
}

size_t LTEmModem::print(char value)
{
	if (_fullDebug) _monitor->print(value);

    return _serial->print(value);
};

size_t LTEmModem::print(unsigned char value, int base)
{
    if (_fullDebug) _monitor->print(value, base);

    return _serial->print(value, base);
};

size_t LTEmModem::print(int value, int base)
{
    if (_fullDebug) _monitor->print(value, base);

    return _serial->print(value, base);
};

size_t LTEmModem::print(unsigned int value, int base)
{
    if (_fullDebug) _monitor->print(value, base);

    return _serial->print(value, base);
};

size_t LTEmModem::print(long value, int base)
{
    if (_fullDebug) _monitor->print(value, base);

    return _serial->print(value, base);
};

size_t LTEmModem::print(unsigned long value, int base)
{
	if (_fullDebug) _monitor->println(value, base);

    return _serial->print(value, base);
};

size_t LTEmModem::print(double value, int digits)
{
	if (_fullDebug) _monitor->println(value, digits);

    return _serial->print(value, digits);
};

size_t LTEmModem::println(const __FlashStringHelper* ifsh)
{
    size_t n = print(ifsh);
    n += println();
    return n;
}

size_t LTEmModem::println(const String& s)
{
    size_t n = print(s);
    n += println();
    return n;
}

size_t LTEmModem::println(const char c[])
{
    size_t n = print(c);
    n += println();
    return n;
}

size_t LTEmModem::println(char c)
{
    size_t n = print(c);
    n += println();
    return n;
}

size_t LTEmModem::println(unsigned char b, int base)
{
    size_t i = print(b, base);
    return i + println();
}

size_t LTEmModem::println(int num, int base)
{
    size_t i = print(num, base);
    return i + println();
}

size_t LTEmModem::println(unsigned int num, int base)
{
    size_t i = print(num, base);
    return i + println();
}

size_t LTEmModem::println(long num, int base)
{
    size_t i = print(num, base);
    return i + println();
}

size_t LTEmModem::println(unsigned long num, int base)
{
    size_t i = print(num, base);
    return i + println();
}

size_t LTEmModem::println(double num, int digits)
{
    if (_fullDebug) _monitor->println(num);

    return _serial->println(num, digits);
}

size_t LTEmModem::println(const Printable& x)
{
    size_t i = print(x);
    return i + println();
}

size_t LTEmModem::println(void)
{
    if (_fullDebug) _monitor->println();
    size_t i = print('\r');

    return i;
}

unsigned int LTEmModem::getDefaultBaudRate()
{
	return 115200;
}

bool LTEmModem::startsWith(const char* pre, const char* str)
{
  return (strncmp(pre, str, strlen(pre)) == 0);
}

int LTEmModem::timedRead(uint32_t timeout) const
{
    int c;
    uint32_t _startMillis = millis();

    do {
        c = _serial->read();

        if (c >= 0) {
            return c;
        }
    } while (millis() - _startMillis < timeout);

    return -1; // -1 indicates timeout
}

ResponseTypes LTEmModem::readResponse(char* buffer, size_t size,
                                        CallbackMethodPtr parserMethod, void* callbackParameter, void* callbackParameter2,
                                        size_t* outSize, uint32_t timeout)
{
    ResponseTypes response = ResponseNotFound;
    uint32_t from = NOW;

    do {
        // 250ms,  how many bytes at which baudrate?
        int count = readLn(buffer, size, 250);
        //sodaq_wdt_reset();

        if (count > 0) {
            if (outSize) {
                *outSize = count;
            }

            if (_disableDiag && strncmp(buffer, "OK", 2) != 0) {
                _disableDiag = false;
            }

			if (_fullDebug)
			{
				_monitor->print("[rdResp]: ");
				_monitor->println(buffer);
			}
			
			if (buffer[0] == '\0' || buffer[0] == '\r')
				continue;
			
			if (startsWith("AT+CGSN", buffer)) //getting imei
			{
				char myBuffer[200];
				readLn(myBuffer, 200, 250);
				
				_imei = &myBuffer[0];
			}
			
			if (startsWith("AT+CCID", buffer)) //getting iccid
			{
				char myBuffer[200];
				readLn(myBuffer, 200, 250);
				
				_iccid = &myBuffer[7];
			}
			
			if (startsWith("AT+CIMI", buffer))
			{
				char myBuffer[200];
				readLn(myBuffer, 200, 250);
				
				_cimi = &myBuffer[0];
			}
			
			if (startsWith("AT+CGMR", buffer)) //getting firmware version
			{
				char myBuffer[200];
				readLn(myBuffer, 200, 250);
				
				_firmware = &myBuffer[0];
			}
			
            int param1, param2;
            if (sscanf(buffer, "+UFOTAS: %d,%d", &param1, &param2) == 2) { // Handle FOTA URC
                uint16_t blkRm = param1;
                uint8_t transferStatus = param2;

                _monitor->print("Unsolicited: FOTA: ");
                _monitor->print(blkRm);
                _monitor->print(", ");
                _monitor->println(transferStatus);

                continue;
            }
            else if (sscanf(buffer, "+NSONMI: %d,%d", &param1, &param2) == 2) { // Handle socket URC for N2
                int socketID = param1;
                int dataLength = param2;

                _monitor->print("Unsolicited: Socket ");
                _monitor->print(socketID);
                _monitor->print(": ");
                _monitor->println(dataLength);
				
                _receivedUDPResponseSocket = socketID;
                _pendingUDPBytes = dataLength;

                continue;
            }
            else if (sscanf(buffer, "+UUSORF: %d,%d", &param1, &param2) == 2) {
                int socketID = param1;
                int dataLength = param2;

                _monitor->print("Unsolicited: Socket ");
                _monitor->print(socketID);
                _monitor->print(": ");
                _monitor->println(dataLength);
				
                _receivedUDPResponseSocket = socketID;
                _pendingUDPBytes = dataLength;

                continue;
            }
			
			if (startsWith("+UUMQTTCM: 6", buffer)) {  //handle MQTT message
				process();
			}
			
			if (startsWith("{\"at\":", buffer)) {	//handle callback 
				_callback(buffer);
			}

            if (startsWith("AT", buffer)) {
                continue; // skip echoed back command
            }

			_disableDiag = false;

			if (startsWith("OK", buffer)) {
				return ResponseOK;
			}

			if (startsWith("ERROR", buffer) ||
					startsWith("+CME ERROR:", buffer) ||
					startsWith("+CMS ERROR:", buffer)) {
				return ResponseError;
			}

			if (parserMethod) {
				ResponseTypes parserResponse = parserMethod(response, buffer, count, callbackParameter, callbackParameter2);

				if ((parserResponse != ResponseEmpty) && (parserResponse != ResponsePendingExtra)) {
					return parserResponse;
				}
				else {
					// ?
					// ResponseEmpty indicates that the parser was satisfied
					// Continue until "OK", "ERROR", or whatever else.
				}

				// Prevent calling the parser again.
				// This could happen if the input line is too long. It will be split
				// and the next readLn will return the next part.
				// The case of "ResponsePendingExtra" is an exception to this, thus waiting for more replies to be parsed.
				if (parserResponse != ResponsePendingExtra) {
					parserMethod = 0;
				}
            }

            // at this point, the parserMethod has ran and there is no override response from it,
            // so if there is some other response recorded, return that
            // (otherwise continue iterations until timeout)
            if (response != ResponseNotFound) {
                _monitor->println("** response != ResponseNotFound");
                return response;
            }
        }

        delay(10);      // TODO Why do we need this delay?
    }
    while (!is_timedout(from, timeout));

    if (outSize) {
        *outSize = 0;
    }

	if (_fullDebug)
		_monitor->println("[rdResp]: timed out");
	
    return ResponseTimeout;
}

size_t LTEmModem::readBytesUntil(char terminator, char* buffer, size_t length, uint32_t timeout)
{
    if (length < 1) {
        return 0;
    }

    size_t index = 0;

    while (index < length) {
        int c = timedRead(timeout);

        if (c < 0 || c == terminator) {
            break;
        }

        *buffer++ = static_cast<char>(c);
        index++;
    }

    if (index < length) {
        *buffer = '\0';
    }

    return index;
}

size_t LTEmModem::readBytes(uint8_t* buffer, size_t length, uint32_t timeout)
{
    size_t count = 0;

    while (count < length) {
        int c = timedRead(timeout);

        if (c < 0) {
            break;
        }

        *buffer++ = static_cast<uint8_t>(c);
        count++;
    }

    // TODO distinguise timeout from empty string?
    // TODO return error for overflow?
    return count;
}

size_t LTEmModem::readLn(char* buffer, size_t size, uint32_t timeout)
{
    // Use size-1 to leave room for a string terminator
    size_t len = readBytesUntil(SODAQ_AT_DEVICE_TERMINATOR[SODAQ_AT_DEVICE_TERMINATOR_LEN - 1], buffer, size - 1, timeout);

    // check if the terminator is more than 1 characters, then check if the first character of it exists
    // in the calculated position and terminate the string there
    if ((SODAQ_AT_DEVICE_TERMINATOR_LEN > 1) && (buffer[len - (SODAQ_AT_DEVICE_TERMINATOR_LEN - 1)] == SODAQ_AT_DEVICE_TERMINATOR[0])) {
        len -= SODAQ_AT_DEVICE_TERMINATOR_LEN - 1;
    }

    // terminate string, there should always be room for it (see size-1 above)
    buffer[len] = '\0';

    return len;
}



void LTEmModem::setOptions(Options* options)
{
	
}

bool LTEmModem::send(Payload &payload)
{
	String temp(payload.getString());

	int index = temp.indexOf("|");
	
	bool ok;
	
	if (index == -1) //no json string
	{
		char* Mqttstring_buff;
		{
			int length = strlen(_credentials->getDeviceId()) + 14;  // 14 fixed chars + deviceId
			Mqttstring_buff = new char[length];
			sprintf(Mqttstring_buff, "device/%s/state", _credentials->getDeviceId());
			Mqttstring_buff[length-1] = 0;
		}
		
		ok = publishMqttMessage(Mqttstring_buff, payload.getBytes(), payload.getSize());
		
		delete(Mqttstring_buff);
	}
	else
	{
		String assetName = temp.substring(0, index);
		String json = temp.substring(index+1);
		
		ok = publishMqttMessage(assetName.c_str(), (char*)json.c_str(), true);
	}
	
	return ok;
}

