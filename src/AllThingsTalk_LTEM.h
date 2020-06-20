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
#include <Scheduler.h>

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
    bool connectionLed(); // Use to check if connection LED is enabled
    bool connectionLed(bool);
    bool connectionLed(int ledPin);
    bool connectionLed(bool state, int ledPin);

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

    void connectionLedFadeStart();
    void connectionLedFadeStop();
    void connectionLedFade();
    // Connection Signal LED Parameters
    #define UP 1
    #define DOWN 0
    bool ledEnabled                  = true;    // Default state for Connection LED
    int connectionLedPin             = LED_BUILTIN;
    bool schedulerActive             = false;   // Keep track of scheduler
    bool supposedToFade              = false;   // Know if Connection LED is supposed to fade
    bool supposedToStop              = false;   // Keep track if fading is supposed to stop
    bool fadeOut                     = false;   // Keep track if Connection LED is supposed to fade out
    bool fadeOutBlink                = false;   // Keep track if Connection LED is supposed to blink after fading out
    static const int minPWM          = 0;       // Minimum PWM
    static const int maxPWM          = 255;     // Maximum PWM
    static const byte fadeIncrement  = 3;       // How smooth to fade
    static const int fadeInterval    = 10;      // Interval between fading steps
    static const int blinkInterval   = 100;     // Milliseconds between blinks at the end of connection led fade-out
    int fadeOutBlinkIteration        = 0;       // Keeps track of Connection LED blink iterations
    byte fadeDirection               = UP;      // Keep track which way should the LED Fade
    int fadeValue                    = 0;       // Keep track of current Connection LED brightness
    unsigned long previousFadeMillis;           // millis() timing Variable, just for fading
    unsigned long previousFadeOutMillis;        // Keeps track of time for fading out Connection LED
    unsigned long previousFadeOutBlinkMillis;   // Keeps track of time for blinks after fading out the LED
};

#endif