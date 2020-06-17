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

class ActuationCallback {
public:
    String asset;
    void *actuationCallback;
    int actuationCallbackArgumentType;
    //void execute(JsonVariant variant);
};

class AssetProperty {
public:
    String name;
    String title;
    String assetType;
    String dataType;
};

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
    bool registerDevice(const char* deviceSecret, const char* partnerId);
    bool sendSMS(char* number, char* message);
    char* getFirmwareVersion();
    char* getIMEI();
    char* getOperator();
    bool setOperator(const char* apn);
    void reboot();
    void loop();
	
	static void mqttCallback(const char* p_topic, const uint8_t *p_payload, size_t p_length);
	static void handlePacket(uint8_t *pckt, size_t len);
    static const int maximumActuations = 32;
    ActuationCallback actuationCallbacks[maximumActuations];
    int actuationCallbackCount = 0;
    bool tryAddActuationCallback(String asset, void *actuationCallback, int actuationCallbackArgumentType);
    ActuationCallback *getActuationCallbackForAsset(String asset);

    // Callbacks (Receiving Data)
    // These will return 
    bool setActuationCallback(String asset, void (*actuationCallback)(bool payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(int payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(double payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(float payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(const char* payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(String payload));

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
    bool isSubscribed;
    unsigned long previousPing;
    int pingInterval = 30; // Seconds

    // Actuations / Callbacks
    bool callbackEnabled = true;           // Variable for checking if callback is enabled
    //void mqttCallback(char* p_topic, const uint8_t *p_payload, unsigned int p_length); // ESP8266 Specific Line
    static AllThingsTalk_LTEM* instance; // Internal callback saving for non-ESP devices (e.g. MKR)
     // Static is only for MKR
	
};

#endif