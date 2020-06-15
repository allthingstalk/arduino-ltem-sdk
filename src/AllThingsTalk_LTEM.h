#ifndef AllThingsTalk_LTEM_h
#define AllThingsTalk_LTEM_h

#include "Arduino.h"
#include <Utils.h>
#include "Sodaq_R4X.h"
#include "Sodaq_MQTT.h"
#include "Sodaq_R4X_MQTT.h"
#include "ArduinoJson.h"
#include "CborPayload.h"
#include "JsonPayload.h"
#include "APICredentials.h"

// TODO: ABP + DEVICE + OTHERS

class AllThingsTalk_LTEM {
public:
    AllThingsTalk_LTEM(HardwareSerial &modemSerial, APICredentials &credentials, char* APN);
    void debugPort(Stream &debugSerial, bool verbose = false);
    bool init();
    bool connect();
    bool disconnect();
    bool isConnected();
    //bool send(Payload &payload);
	bool send(CborPayload &payload);
	bool send(JsonPayload &payload);
    bool setCallback();
    bool registerDevice(const char* deviceSecret, const char* partnerId);
    bool sendSMS(char* number, char* message);
    char* getFirmwareVersion();
    char* getIMEI();
    char* getOperator();
    bool setOperator(const char* apn);
    void reboot();
    void loop();

private:
    template<typename T> void debug(T message, char separator = '\n');
    template<typename T> void debugVerbose(T message, char separator = '\n');
    bool connectNetwork();
    bool connectMqtt();
    String generateUniqueID();
    HardwareSerial *_modemSerial;
    APICredentials *_credentials;
    Stream *debugSerial;
    bool debugVerboseEnabled;
    char* _APN;
};

#endif