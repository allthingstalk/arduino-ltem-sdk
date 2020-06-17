#include "AllThingsTalk_LTEM.h"

#define URAT            SODAQ_R4X_LTEM_URAT
#define MNOPROF         MNOProfiles::SIM_ICCID
#define OPERATOR        AUTOMATIC_OPERATOR
#define M1_BAND_MASK    BAND_MASK_UNCHANGED
#define NB1_BAND_MASK   BAND_MASK_UNCHANGED

Sodaq_R4X r4x;
Sodaq_SARA_R4XX_OnOff saraR4xxOnOff;
Sodaq_R4X_MQTT r4x_mqtt;

AllThingsTalk_LTEM *AllThingsTalk_LTEM::instance = nullptr;

AllThingsTalk_LTEM::AllThingsTalk_LTEM(HardwareSerial &modemSerial, APICredentials &credentials, char* APN) {
    _modemSerial = &modemSerial;
    _credentials = &credentials;
    _APN = APN;
}

// Serial print (debugging)
template<typename T> void AllThingsTalk_LTEM::debug(T message, char separator) {
    if (debugSerial) {
        debugSerial->print(message);
        if (separator) {
            debugSerial->print(separator);
        }
    }
}

// Serial print (verbose debugging)
template<typename T> void AllThingsTalk_LTEM::debugVerbose(T message, char separator) {
    if (debugVerboseEnabled) {
        if (debugSerial) {
            debugSerial->print(message);
            if (separator) {
                debugSerial->print(separator);
            }
        }
    }
}

void AllThingsTalk_LTEM::debugPort(Stream &debugSerial, bool verbose) {
    debugVerboseEnabled = verbose;
    this->debugSerial = &debugSerial;
    debug("");
    debug("------------- AllThingsTalk LTE-M SDK Debug Output Begin -------------");
    if (verbose) {
        debugVerbose("Debug Level: Verbose. You'll also see AT Commands coming in and out from the Modem.");
        r4x.setDiag(debugSerial);
    } else {
        debug("Debug Level: Normal");
    }
}

String AllThingsTalk_LTEM::generateUniqueID() {
    // This is unique to Arduino Zero/M0-like boards.
    // Most other Arduino boards don't possess a unique serial number.
    volatile uint32_t val1, val2, val3, val4;
    volatile uint32_t *ptr1 = (volatile uint32_t *)0x0080A00C;
    val1 = *ptr1;
    volatile uint32_t *ptr = (volatile uint32_t *)0x0080A040;
    val2 = *ptr;
    ptr++;
    val3 = *ptr;
    ptr++;
    val4 = *ptr;
    char id[23];
    sprintf(id, "%8x%8x", val2, val4);
    debugVerbose("Generated Unique ID for this device:", ' ');
    debugVerbose(id);
    return id;
}

//TODO: PINS
//TODO: LED
bool AllThingsTalk_LTEM::init() {
	AllThingsTalk_LTEM::instance = this;
    mqtt.setServer(_credentials->getSpace(), 1883);
    mqtt.setAuth(_credentials->getDeviceToken(), "arbitrary");
    mqtt.setClientId(generateUniqueID().c_str());
    mqtt.setKeepAlive(300);
    if (callbackEnabled == true) {
        mqtt.setPublishHandler(this->mqttCallback);
    }

    _modemSerial->begin(r4x.getDefaultBaudrate()); // The transport layer is a Sodaq_R4X
    r4x.init(&saraR4xxOnOff, *_modemSerial);
    //r4x_mqtt.setR4Xinstance(&r4x, &AllThingsTalk_LTEM::connectNetwork); // Inform our mqtt instance that we use r4x as the transport
    //r4x_mqtt.setR4Xinstance(&r4x, std::bind(&AllThingsTalk_LTEM::connectNetwork, this));
    r4x_mqtt.setR4Xinstance(&r4x, []{ return true; } );
    mqtt.setTransport(&r4x_mqtt);
    return connect();
}

bool AllThingsTalk_LTEM::connect() {
    if (connectNetwork()) {
        if (connectMqtt()) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool AllThingsTalk_LTEM::connectNetwork() {
    debug("Connecting to Network...");
    if (r4x.isConnected()) {
        debug("Already connected to Network!");
        return true;
    } else {
        if (r4x.connect(_APN, URAT, MNOPROF, OPERATOR, M1_BAND_MASK, NB1_BAND_MASK)) {
            debug("Connected to Network!");
            return true;
        } else {
            debug("Failed to connect to network!");
            return false;
        }
    }
}

bool AllThingsTalk_LTEM::connectMqtt() {
    debug("Connecting to MQTT...");
    if (!r4x_mqtt.isAliveMQTT()) {
        int connectRetry = 0;
        while (!mqtt.open() && connectRetry < 5) {
            connectRetry++;
        }
        if (r4x_mqtt.isAliveMQTT()) {
            debug("Successfully connected to MQTT!");
            return true;
        } else {
            debug("Failed to connect to MQTT!");
            return false;
        }
    } else {
        debug("Already connected to MQTT!");
        return true;
    }
}

bool AllThingsTalk_LTEM::disconnect() {
    return r4x.disconnect();
}

bool AllThingsTalk_LTEM::isConnected() {
    return r4x.isConnected();
}

bool AllThingsTalk_LTEM::send(CborPayload &payload) {
    if (isConnected()) {
        char* topic;
        int length = strlen(_credentials->getDeviceId()) + 14;  // 14 fixed chars + deviceId
        topic = new char[length];
        sprintf(topic, "device/%s/state", _credentials->getDeviceId());
        topic[length-1] = 0;
        if (mqtt.publish(topic, payload.getBytes(), payload.getSize(), 0, 0)) {
            debug("> Message Published to AllThingsTalk (CBOR)");
            return true;
        } else {
            debug("> Failed to Publish Message to AllThingsTalk (CBOR)");
            return false;
        }
    }
}

bool AllThingsTalk_LTEM::send(JsonPayload &payload) {
    if (isConnected()) {
        char topic[128];
        snprintf(topic, sizeof topic, "%s%s%s%s%s", "device/", _credentials->getDeviceId(), "/asset/", payload.getAssetName(), "/state");
        if (mqtt.publish(topic, payload.getBytes(), payload.getSize(), 0, 0)) {
            debug("> Message Published to AllThingsTalk (JSON)");
            debugVerbose("Asset:", ' ');
            debugVerbose(payload.getAssetName(), ',');
            debugVerbose(" Value:", ' ');
            debugVerbose(payload.getString());
            return true;
        } else {
            debug("> Failed to Publish Message to AllThingsTalk (JSON)");
            return false;
        }
    }
}

bool AllThingsTalk_LTEM::registerDevice(const char* deviceSecret, const char* partnerId) {
  // TO BE IMPLEMENTED
}

bool AllThingsTalk_LTEM::sendSMS(char* number, char* message) {
    // TO BE IMPLEMENTED
}

char* AllThingsTalk_LTEM::getFirmwareVersion() {
    char versionBuffer[128];
    if (r4x.getFirmwareVersion(versionBuffer, sizeof(versionBuffer))) {
        debug("LTE-M Modem Firmware Version: ", ',');
        debug(versionBuffer);
        return versionBuffer;
    } else {
        debug("Couldn't get the firmware version.");
        return "ERROR";
    }
}

char* AllThingsTalk_LTEM::getIMEI() {
    char imeiBuffer[128];
    if (r4x.getIMEI(imeiBuffer, sizeof(imeiBuffer))) {
        debug("IMEI: ", ',');
        debug(imeiBuffer);
        return imeiBuffer;
    } else {
        debug("Couldn't get IMEI.");
        return "ERROR";
    }
}

char* AllThingsTalk_LTEM::getOperator() {
    char operatorBuffer[128];
    if (r4x.getOperatorInfoString(operatorBuffer, sizeof(operatorBuffer))) {
        debug("Operator Info: ", ',');
        debug(operatorBuffer);
        return operatorBuffer;
    } else {
        debug("Couldn't get Operator Info.");
        return "ERROR";
    }
}

bool AllThingsTalk_LTEM::setOperator(const char* apn) {
    debug("Setting operator (APN)...");
    if (r4x.setApn(apn)) {
        debug("APN Successfully set to: ", ' ');
        debug(apn);
        return true;
    } else {
        debug("Failed to set APN.");
        return false;
    }
}

void AllThingsTalk_LTEM::reboot() {
    debug("Rebooting modem...");
    // Reboot is a private method of R4X. Check if we need it.
    //r4x.reboot();
}

void AllThingsTalk_LTEM::loop() {
    if (mqtt.loop()) {
		return;
	}
    if (!mqtt.isConnected()) {
        isSubscribed = false;
    }
    if (callbackEnabled == true && !isSubscribed) {
        // Build the subscribe topic
        char command_topic[256];
        snprintf(command_topic, sizeof command_topic, "%s%s%s", "device/", _credentials->getDeviceId(), "/asset/+/command");
        if (mqtt.subscribe(command_topic)) { // Subscribe to it
            debugVerbose("Successfully subscribed to MQTT.");
            isSubscribed = true;
        } else {
            debugVerbose("Failed to subscribe to MQTT!");
            isSubscribed = false;
        }
    }

    // if (millis() - previousPing >= pingInterval*1000) {
        // if (!mqtt.ping()) {
            // debugVerbose("MQTT Ping failed this time.");
        // }
        // previousPing = millis();
    // }
}

// Add boolean callback (0)
bool AllThingsTalk_LTEM::setActuationCallback(String asset, void (*actuationCallback)(bool payload)) {
    debugVerbose("Adding a Boolean Actuation Callback for Asset:", ' ');
    return tryAddActuationCallback(asset, (void*) actuationCallback, 0);
}

// Add integer callback (1)
bool AllThingsTalk_LTEM::setActuationCallback(String asset, void (*actuationCallback)(int payload)) {
    debugVerbose("Adding an Integer Actuation Callback for Asset:", ' ');
    return tryAddActuationCallback(asset, (void*) actuationCallback, 1);
}

// Add double callback (2)
bool AllThingsTalk_LTEM::setActuationCallback(String asset, void (*actuationCallback)(double payload)) {
    debugVerbose("Adding a Double Actuation Callback for Asset:", ' ');
    return tryAddActuationCallback(asset, (void*) actuationCallback, 2);
}

// Add float callback (3)
bool AllThingsTalk_LTEM::setActuationCallback(String asset, void (*actuationCallback)(float payload)) {
    debugVerbose("Beware that the maximum value of float in 32-bit systems is 2,147,483,647");
    debugVerbose("Adding a Float Actuation Callback for Asset:", ' ');
    return tryAddActuationCallback(asset, (void*) actuationCallback, 3);
}

// Add char callback (4)
bool AllThingsTalk_LTEM::setActuationCallback(String asset, void (*actuationCallback)(const char* payload)) {
    debugVerbose("Adding a Char Actuation Callback for Asset:", ' ');
    return tryAddActuationCallback(asset, (void*) actuationCallback, 4);
}

// Add String callback (5)
bool AllThingsTalk_LTEM::setActuationCallback(String asset, void (*actuationCallback)(String payload)) {
    debugVerbose("Adding a String Actuation Callback for Asset:", ' ');
    return tryAddActuationCallback(asset, (void*) actuationCallback, 5);
}

// Actual saving of added callbacks
bool AllThingsTalk_LTEM::tryAddActuationCallback(String asset, void (*actuationCallback), int actuationCallbackArgumentType) {
   if (actuationCallbackCount >= maximumActuations) {
       debug("");
       debug("You've added too many actuations. The maximum is", ' ');
       debug(maximumActuations);
       return false;
   }
    callbackEnabled = true;
    actuationCallbacks[actuationCallbackCount].asset = asset;
    actuationCallbacks[actuationCallbackCount].actuationCallback = actuationCallback;
    actuationCallbacks[actuationCallbackCount].actuationCallbackArgumentType = actuationCallbackArgumentType;
    actuationCallbackCount++;
    debugVerbose(asset);
    return true;
}

// Retrieve a specific callback based on asset
ActuationCallback *AllThingsTalk_LTEM::getActuationCallbackForAsset(String asset) {
    for (int i = 0; i < actuationCallbackCount; i++) {
        if (asset == actuationCallbacks[i].asset) {
            debugVerbose("Found Actuation Callback for Asset:", ' ');
            debugVerbose(actuationCallbacks[i].asset);
            return &actuationCallbacks[i];
        }
    }
    return nullptr;
}

// Asset name extraction from MQTT topic
String extractAssetNameFromTopic(String topic) {
    // Topic is formed as: device/ID/asset/NAME/state
    const int devicePrefixLength = 38;  // "device/ID/asset/"
    const int stateSuffixLength = 8;  // "/state"
    return topic.substring(devicePrefixLength, topic.length()-stateSuffixLength);
}

/* void ActuationCallback::execute(JsonVariant variant) {
}
*/
void AllThingsTalk_LTEM::handlePacket(uint8_t *pckt, size_t len)
{
	instance->debugVerbose("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@éééé");
}


// MQTT Callback for receiving messages
void AllThingsTalk_LTEM::mqttCallback(const char* p_topic, const uint8_t *p_payload, size_t p_length) {
	instance->debugVerbose("--------------------------------------");
    instance->debug("< Message Received from AllThingsTalk");

    String payload;
    String topic(p_topic);
    
    instance->debugVerbose("Raw Topic:", ' ');
    instance->debugVerbose(p_topic);

    // Whole JSON Payload
    for (uint8_t i = 0; i < p_length; i++) payload.concat((char)p_payload[i]);
    instance->debugVerbose("Raw JSON Payload:", ' ');
    instance->debugVerbose(payload);
    
    // Deserialize JSON
    DynamicJsonDocument doc(256);
    char json[256];
    for (int i = 0; i < p_length; i++) {
        json[i] = (char)p_payload[i];
    }
    auto error = deserializeJson(doc, json);
    if (error) {
        instance->debug("Parsing JSON failed. Code:", ' ');
        instance->debug(error.c_str());
        return;
    }

    // Extract time from JSON Document
    instance->debugVerbose("Message Time:", ' ');
    const char* time = doc["at"];
    instance->debugVerbose(time);

    String asset = extractAssetNameFromTopic(topic);
    instance->debugVerbose("Asset Name:", ' ');
    instance->debugVerbose(asset);

    // Call actuation callback for this specific asset
    ActuationCallback *actuationCallback = instance->getActuationCallbackForAsset(asset);
    if (actuationCallback == nullptr) {
        instance->debug("Error: There's no actuation callback for this asset.");
        return;
    }

    // Create JsonVariant which we'll use to to check data type and convert if necessary
    JsonVariant variant = doc["value"].as<JsonVariant>();

    //actuationCallback->execute(variant);


/*
    instance->debug("------------------------------------------");
    if (variant.is<bool>()) instance->debug("Value type: BOOLEAN");
    if (variant.is<int>()) instance->debug("Value type: INTEGER");
    if (variant.is<double>()) instance->debug("Value type: DOUBLE");
    if (variant.is<float>()) instance->debug("Value type: FLOAT");
    if (variant.is<char*>()) instance->debug("Value type: CHAR*");
    instance->debug("------------------------------------------");
*/

    // BOOLEAN
    if (actuationCallback->actuationCallbackArgumentType == 0 && variant.is<bool>()) {
        bool value = doc["value"];
        instance->debugVerbose("Called Actuation for Asset:", ' ');
        instance->debugVerbose(actuationCallback->asset, ',');
        instance->debugVerbose(" Payload Type: Boolean, Value:", ' ');
        instance->debugVerbose(value);
        reinterpret_cast<void (*)(bool payload)>(actuationCallback->actuationCallback)(value);
        return;
    }
    
    // INTEGER
    if (actuationCallback->actuationCallbackArgumentType == 1 && variant.is<int>() && variant.is<double>()) {
        int value = doc["value"];
        instance->debugVerbose("Called Actuation for Asset:", ' ');
        instance->debugVerbose(actuationCallback->asset, ',');
        instance->debugVerbose(" Payload Type: Integer, Value:", ' ');
        instance->debugVerbose(value);
        reinterpret_cast<void (*)(int payload)>(actuationCallback->actuationCallback)(value);
        return;
    }

    // DOUBLE
    if (actuationCallback->actuationCallbackArgumentType == 2 && variant.is<double>()) {
        double value = doc["value"];
        instance->debugVerbose("Called Actuation for Asset:", ' ');
        instance->debugVerbose(actuationCallback->asset, ',');
        instance->debugVerbose(" Payload Type: Double, Value:", ' ');
        instance->debugVerbose(value);
        reinterpret_cast<void (*)(double payload)>(actuationCallback->actuationCallback)(value);
        return;
    }

    // FLOAT
    if (actuationCallback->actuationCallbackArgumentType == 3 && variant.is<float>()) {
        float value = doc["value"];
        instance->debugVerbose("Called Actuation for Asset:", ' ');
        instance->debugVerbose(actuationCallback->asset, ',');
        instance->debugVerbose(" Payload Type: Float, Value:", ' ');
        instance->debugVerbose(value);
        reinterpret_cast<void (*)(float payload)>(actuationCallback->actuationCallback)(value);
        return;
    }
    
    // CONST CHAR*
    if (actuationCallback->actuationCallbackArgumentType == 4 && variant.is<char*>()) {
        const char* value = doc["value"];
        instance->debugVerbose("Called Actuation for Asset:", ' ');
        instance->debugVerbose(actuationCallback->asset, ',');
        instance->debugVerbose(" Payload Type: const char*, Value:", ' ');
        instance->debugVerbose(value);
        reinterpret_cast<void (*)(const char* payload)>(actuationCallback->actuationCallback)(value);
        return;
    }
    
    // STRING
    if (actuationCallback->actuationCallbackArgumentType == 5 && variant.is<char*>()) {
        String value = doc["value"];
        instance->debugVerbose("Called Actuation for Asset:", ' ');
        instance->debugVerbose(actuationCallback->asset, ',');
        instance->debugVerbose(" Payload Type: String, Value:", ' ');
        instance->debugVerbose(value);
        reinterpret_cast<void (*)(String payload)>(actuationCallback->actuationCallback)(value);
        return;
    }
    
    // JSON ARRAY
    if (variant.is<JsonArray>()) {
        instance->debug("Receiving Arrays is not yet supported!");
        return;
    }
    
    // JSON OBJECT
    if (variant.is<JsonObject>()) {
        instance->debug("Receiving Objects is not yet supported!");
        return;
    }
}