#ifndef LTEmModem_h
#define LTEmModem_h

#include "APICredentials.h"	
#include "Device.h"
#include "GeoLocation.h"
#include "Sodaq_OnOffBee.h"
#include "Options.h"
#include "LTEmOptions.h"
#include "Payload.h"

enum ResponseTypes {
    ResponseNotFound = 0,
    ResponseOK = 1,
    ResponseError = 2,
    ResponsePrompt = 3,
    ResponseTimeout = 4,
    ResponseEmpty = 5,
    ResponsePendingExtra = 6,
};
		
class LTEmModem : public Device<LTEmOptions>
{
	#define CR "\r"
	#define LF "\n"
	#define CRLF "\r\n"
	
	#define NOW (uint32_t)millis()

	#define DEFAULT_TIMEOUT 120
	#define SOCKET_FAIL -1
	#define ATT_CALLBACK_SIGNATURE void (*callback)(const char*)
	
	#define SODAQ_AT_DEVICE_DEFAULT_READ_MS 5000
	#define SODAQ_AT_DEVICE_TERMINATOR CRLF
	#define SODAQ_AT_DEVICE_TERMINATOR_LEN (sizeof(SODAQ_AT_DEVICE_TERMINATOR) - 1)
	#define SODAQ_NBIOT_DEFAULT_CID 0
	#define UINT8_MAX 255
	
	public:
		LTEmModem(HardwareSerial &serial, Stream &monitor, APICredentials &credentials, bool fullDebug = false, ATT_CALLBACK_SIGNATURE = NULL);
		
		bool init(char *apn);
		unsigned int getDefaultBaudRate();
		void setOptions(Options* options);
		bool send(Payload &payload);
		
		bool isConnected();
		
		bool initTCP();
		bool initMQTT();
		
		bool listen(char* actuator);
		bool mqttConnected();
		
		void disconnect();
		void process();
		
		using Device<LTEmOptions>::send;
		
	private:
		typedef ResponseTypes(*CallbackMethodPtr)(ResponseTypes& response, const char* buffer, size_t size, void* parameter, void* parameter2);
		
		void (*_callback)(const char*);
		
		int timedRead(uint32_t timeout) const;
		int createSocket(uint16_t localPort);
		int createTCPSocket(uint16_t localPort = 0);
		
		bool connect();
		bool setRadioActive(bool on);		
		
		bool on();
		bool off();
		bool isOn() const;
		bool isAlive();
		
		bool setUpTCP(uint8_t socket, const char* remoteIP, const uint16_t remotePort);
		
		size_t readBytesUntil(char terminator, char* buffer, size_t length, uint32_t timeout);
		size_t readBytes(uint8_t* buffer, size_t length, uint32_t timeout);
		size_t readLn(char* buffer, size_t size, uint32_t timeout = 10000);
		size_t readLn() { return readLn(_inputBuffer, _inputBufferSize); };
		
		static bool startsWith(const char* pre, const char* str);
		
		void initBuffer();
		void init(int onoffPin, int8_t saraR4XXTogglePin);
		void setTxEnablePin(int8_t txEnablePin);
		void setTxPowerIfAvailable(bool on);
		void purgeAllResponsesRead();
		void reboot();

		void setOnOff(Sodaq_OnOffBee& onoff) { _onoff = &onoff; }
		
		Sodaq_OnOffBee* _onoff;
		APICredentials* _credentials;
		
		int _receivedUDPResponseSocket = 0;
		
		int8_t _onOffPin;
		int8_t _txEnablePin;
		int8_t _saraR4XXTogglePin;
		int8_t _cid;
		int8_t _lastRSSI;
		uint8_t _CSQtime;
		
		uint32_t _startOn;
		
		char* _apn;
		char* _inputBuffer;
		char* _ipAddress;
		char* _imei;
		char* _iccid;
		char* _cimi;
		char* _firmware;
		
		size_t _inputBufferSize;
		size_t _pendingUDPBytes = 0;
		
		bool publishMqttMessage(const char* topic, double value);
		bool publishMqttMessage(const char* topic, int value);
		bool publishMqttMessage(const char* topic, char* value);
		bool publishMqttMessage(const char* topic, unsigned char* value, int size);
		bool publishMqttMessage(const char* topic, GeoLocation* geoLocation);
		
		bool subscribeMqttMessage(const char* topic);
		bool readMqttMessage();
		
		size_t print(const __FlashStringHelper*);
		size_t print(const String&);
		size_t print(const char[]);
		size_t print(char);
		size_t print(unsigned char, int = DEC);
		size_t print(int, int = DEC);
		size_t print(unsigned int, int = DEC);
		size_t print(long, int = DEC);
		size_t print(unsigned long, int = DEC);
		size_t print(double, int = 2);
		size_t print(const Printable&);

		size_t println(const __FlashStringHelper*);
		size_t println(const String& s);
		size_t println(const char[]);
		size_t println(char);
		size_t println(unsigned char, int = DEC);
		size_t println(int, int = DEC);
		size_t println(unsigned int, int = DEC);
		size_t println(long, int = DEC);
		size_t println(unsigned long, int = DEC);
		size_t println(double, int = 2);
		size_t println(const Printable&);
		size_t println(void);
		
		bool _disableDiag;
		bool _isBufferInitialized;
		bool _fullDebug;
		
		HardwareSerial* _serial;
		Stream* _monitor;
		
		static ResponseTypes _createSocketParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* socket, uint8_t* dummy);
		static ResponseTypes _nconfigParser(ResponseTypes& response, const char* buffer, size_t size, bool* nconfigEqualsArray, uint8_t* dummy);
		static ResponseTypes _csqParser(ResponseTypes& response, const char* buffer, size_t size, int* rssi, int* ber);
		static ResponseTypes _cgattParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* result, uint8_t* dummy);
		static ResponseTypes _umqttParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* value2);
		static ResponseTypes _umqttcParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* value2);
		static ResponseTypes _uumqttcParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* value2);
		static ResponseTypes _umqttTopicParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, int* dummy); //UMQTTWTOPIC
		static ResponseTypes _cgpAddrParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, char** value2);
		static ResponseTypes _cedrxParser(ResponseTypes& response, const char* buffer, size_t size, int* value1, char** value2);
	
		ResponseTypes readResponse(char* buffer, size_t size, size_t* outSize, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
        {
            return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
        };

        ResponseTypes readResponse(char* buffer, size_t size,
                                   CallbackMethodPtr parserMethod, void* callbackParameter, void* callbackParameter2 = NULL,
                                   size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS);

        ResponseTypes readResponse(size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
        {
            return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
        };

        ResponseTypes readResponse(CallbackMethodPtr parserMethod, void* callbackParameter,
                                   void* callbackParameter2 = NULL, size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
        {
            return readResponse(_inputBuffer, _inputBufferSize,
                                parserMethod, callbackParameter, callbackParameter2,
                                outSize, timeout);
        };

        template<typename T1, typename T2>
        ResponseTypes readResponse(ResponseTypes(*parserMethod)(ResponseTypes& response, const char* parseBuffer, size_t size, T1* parameter, T2* parameter2),
                                   T1* callbackParameter, T2* callbackParameter2,
                                   size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
        {
            return readResponse(_inputBuffer, _inputBufferSize, (CallbackMethodPtr)parserMethod,
                                (void*)callbackParameter, (void*)callbackParameter2, outSize, timeout);
        };
};

#endif