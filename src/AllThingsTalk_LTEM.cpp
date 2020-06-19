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

void AllThingsTalk_LTEM::debugPort(Stream &debugSerial, bool verbose, bool verboseAT) {
    debugVerboseEnabled = verbose;
    this->debugSerial = &debugSerial;
    debug("");
    debug("------------- AllThingsTalk LTE-M SDK Debug Output Begin -------------");
    if (verbose) {
        if (verboseAT) {
            debugVerbose("Debug Level: Verbose + AT Commands");
            r4x.setDiag(debugSerial);
        } else {
            debugVerbose("Debug Level: Verbose");
        }
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

char* r4x_mqtt_APN;
bool r4x_mqttConnectNetwork() {
    if (r4x.isConnected()) {
        return true;
    } else {
        return r4x.connect(r4x_mqtt_APN, URAT, MNOPROF, OPERATOR, M1_BAND_MASK, NB1_BAND_MASK);
    }
}

//TODO: PINS
//TODO: LED
bool AllThingsTalk_LTEM::init() {
	AllThingsTalk_LTEM::instance = this; // Important - Static pointer to our class
    mqtt.setServer(_credentials->getSpace(), 1883);
    mqtt.setAuth(_credentials->getDeviceToken(), "arbitrary");
    mqtt.setClientId(generateUniqueID().c_str());
    mqtt.setKeepAlive(300);
    if (callbackEnabled) { 
        mqtt.setPublishHandler(this->mqttCallback);
    }
    _modemSerial->begin(r4x.getDefaultBaudrate()); // The transport layer is a Sodaq_R4X
    r4x.init(&saraR4xxOnOff, *_modemSerial);
    r4x_mqtt_APN = this->_APN;
    r4x_mqtt.setR4Xinstance(&r4x, r4x_mqttConnectNetwork);
    mqtt.setTransport(&r4x_mqtt);
    
    return connect();
}

bool AllThingsTalk_LTEM::disconnect() {
    debug("Disconnecting from LTE-M and AllThingsTalk...");
    if (r4x.disconnect()) {
        intentionallyDisconnected = true;
        debug("Successfully disconnected from LTE-M Network and AllThingsTalk");
        return true;
    } else {
        debug("Failed to disconnect from the network");
        return false;
    }
}

bool AllThingsTalk_LTEM::isConnected() {
    return r4x.isConnected();
}

bool AllThingsTalk_LTEM::connect() {
    intentionallyDisconnected = false;
    if (connectNetwork() && connectMqtt()) {
        return true;
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
            showDiagnosticInfo(); // Shows FW, IMEI, ICCID, IMSI, etc
            return true;
        } else {
            debug("Failed to connect to network!");
            return false;
        }
    }
}

bool AllThingsTalk_LTEM::connectMqtt() {
    debug("Connecting to MQTT...");
    int connectRetry = 0;
    // Use mqtt.ping to check if there's a real connection towards broker. Try 10 times before giving up.
    while (!mqtt.ping() && connectRetry < 10) {
        connectRetry++;
    }
    if (connectRetry != 10) {
        debug("Successfully connected to MQTT!");
        if (callbackEnabled) {
            // Build the subscribe topic
            char command_topic[256];
            snprintf(command_topic, sizeof command_topic, "%s%s%s", "device/", _credentials->getDeviceId(), "/asset/+/command");
            if (mqtt.subscribe(command_topic)) { // Subscribe to it
                debugVerbose("Successfully subscribed to MQTT.");
            } else {
                debugVerbose("Failed to subscribe to MQTT!");
            }
        }
        return true;
    } else {
        debug("Failed to connect to MQTT!");
        return false;
    }
}

void AllThingsTalk_LTEM::maintainMqtt() {
    if (callbackEnabled && !intentionallyDisconnected) { // Only maintain MQTT connection constantly if there's anything to wait for and if user didn't intentionally disconnect
        if (mqtt.loop()) return; // If something is received, skip the rest of the method this time (saves energy)
        if (millis() - previousPing >= pingInterval*1000) {
            if (!mqtt.ping()) {
                connectMqtt(); // Establish an MQTT connection again if ping failed
                debugVerbose("MQTT Ping failed this time. Reconnecting AllThingsTalk...");
            }
            previousPing = millis();
        }
    }
}

bool AllThingsTalk_LTEM::send(CborPayload &payload) {
    if (!intentionallyDisconnected) {
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
    } else {
        debug("You're trying to send a message but you've disconnected from the network. Execute connect() to re-connect.");
    }
}

template<typename T> bool AllThingsTalk_LTEM::send(char *asset, T value) {
    if (!intentionallyDisconnected) {
        if (isConnected()) {
            char topic[128];
            snprintf(topic, sizeof topic, "%s%s%s%s%s", "device/", _credentials->getDeviceId(), "/asset/", asset, "/state");
            DynamicJsonDocument doc(256);
            char JSONmessageBuffer[256];
            doc["value"] = value;
            serializeJson(doc, JSONmessageBuffer);
            if (mqtt.publish(topic, (unsigned char*)JSONmessageBuffer, strlen(JSONmessageBuffer), 0, 0)) {
                debug("> Message Published to AllThingsTalk (JSON)");
                debugVerbose("Asset:", ' ');
                debugVerbose(asset, ',');
                debugVerbose(" Value:", ' ');
                debugVerbose(value);
                return true;
            } else {
                debug("> Failed to Publish Message to AllThingsTalk (JSON)");
                return false;
            }
        }
    } else {
        debug("You're trying to send a message but you've disconnected from the network. Execute connect() to re-connect.");
    }
}



bool AllThingsTalk_LTEM::registerDevice(const char* deviceSecret, const char* partnerId) {
  // TO BE IMPLEMENTED
}

bool AllThingsTalk_LTEM::sendSMS(char* number, char* message) {
    // TO BE IMPLEMENTED
}

void AllThingsTalk_LTEM::showDiagnosticInfo() {
    if (justBooted) {
        getOperator();
        getICCID();
        getIMSI();
        getIMEI();
        getFirmwareVersion();
        getFirmwareRevision();
        justBooted = false;
    }
}

char* AllThingsTalk_LTEM::getFirmwareVersion() {
    char buffer[128];
    if (r4x.getFirmwareVersion(buffer, sizeof(buffer))) {
        debug("LTE-M Modem Firmware Version: ", ' ');
        debug(buffer);
        return buffer;
    } else {
        debug("Couldn't get the firmware version.");
        return "ERROR";
    }
}

char* AllThingsTalk_LTEM::getFirmwareRevision() {
    char buffer[128];
    if (r4x.getFirmwareRevision(buffer, sizeof(buffer))) {
        debug("LTE-M Modem Firmware Revision: ", ' ');
        debug(buffer);
        return buffer;
    } else {
        debug("Couldn't get the firmware revision.");
        return "ERROR";
    }
}

char* AllThingsTalk_LTEM::getIMEI() {
    char buffer[128];
    if (r4x.getIMEI(buffer, sizeof(buffer))) {
        debug("IMEI:", ' ');
        debug(buffer);
        return buffer;
    } else {
        debug("Couldn't get IMEI.");
        return "ERROR";
    }
}

char* AllThingsTalk_LTEM::getICCID() {
    char buffer[128];
    if (r4x.getCCID(buffer, sizeof(buffer))) {
        debug("ICCID:", ' ');
        debug(buffer);
        return buffer;
    } else {
        debug("Couldn't get ICCID.");
        return "ERROR";
    }
}

char* AllThingsTalk_LTEM::getIMSI() {
    char buffer[128];
    if (r4x.getIMSI(buffer, sizeof(buffer))) {
        debug("IMSI:", ' ');
        debug(buffer);
        return buffer;
    } else {
        debug("Couldn't get IMSI.");
        return "ERROR";
    }
}

char* AllThingsTalk_LTEM::getOperator() {
    char buffer[128];
    if (r4x.getOperatorInfoString(buffer, sizeof(buffer))) {
        debug("Operator Info:", ' ');
        debug(buffer);
        return buffer;
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
    maintainMqtt();
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

template bool AllThingsTalk_LTEM::send(char *asset, bool value);
template bool AllThingsTalk_LTEM::send(char *asset, char *value);
template bool AllThingsTalk_LTEM::send(char *asset, const char *value);
template bool AllThingsTalk_LTEM::send(char *asset, String value);
template bool AllThingsTalk_LTEM::send(char *asset, int value);
template bool AllThingsTalk_LTEM::send(char *asset, byte value);
template bool AllThingsTalk_LTEM::send(char *asset, short value);
template bool AllThingsTalk_LTEM::send(char *asset, long value);
template bool AllThingsTalk_LTEM::send(char *asset, float value);
template bool AllThingsTalk_LTEM::send(char *asset, double value);