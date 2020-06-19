#ifndef AllThingsTalk_LTEM_h
#define AllThingsTalk_LTEM_h

#include "Arduino.h"
#include <Utils.h>
#include "Sodaq_R4X.h"
#include "Sodaq_MQTT.h"
#include "Sodaq_R4X_MQTT.h"
#include "ArduinoJson.h"
#include "CborPayload.h"
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
    void debugPort(Stream &debugSerial, bool verbose = false, bool verboseAT = false);
    bool init();
    bool connect();
    bool disconnect();
    bool isConnected();
    bool send(CborPayload &payload);
    template<typename T> bool send(char *asset, T value);
    bool registerDevice(const char* deviceSecret, const char* partnerId);
    bool sendSMS(char* number, char* message);
    bool setOperator(const char* apn);
    char* getFirmwareVersion();
    char* getFirmwareRevision();
    char* getIMEI();
    char* getICCID();
    char* getIMSI();
    char* getOperator();
    void reboot();
    void loop();

    // Callbacks (Receiving Data)
    bool setActuationCallback(String asset, void (*actuationCallback)(bool payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(int payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(double payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(float payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(const char* payload));
    bool setActuationCallback(String asset, void (*actuationCallback)(String payload));

private:
    static AllThingsTalk_LTEM* instance;
    template<typename T> void debug(T message, char separator = '\n');
    template<typename T> void debugVerbose(T message, char separator = '\n');

    String generateUniqueID();
    bool connectNetwork();
    bool connectMqtt();
    void maintainMqtt();
    bool justBooted = true;
    void showDiagnosticInfo();
    HardwareSerial *_modemSerial;
    Stream *debugSerial;
    APICredentials *_credentials;

    bool debugVerboseEnabled;
    bool isSubscribed;
    char* _APN;
    int pingInterval = 30; // Seconds
    unsigned long previousPing;
    bool intentionallyDisconnected;

    // Actuations / Callbacks
    static const int maximumActuations = 32;
    bool callbackEnabled = true;         // Variable for checking if callback is enabled
	static void mqttCallback(const char* p_topic, const uint8_t *p_payload, size_t p_length);
    ActuationCallback actuationCallbacks[maximumActuations];
    int actuationCallbackCount = 0;
    bool tryAddActuationCallback(String asset, void *actuationCallback, int actuationCallbackArgumentType);
    ActuationCallback *getActuationCallbackForAsset(String asset);
};

#endif