#include "AllThingsTalk_LTEM.h"

#define URAT            SODAQ_R4X_LTEM_URAT
#define MNOPROF         MNOProfiles::SIM_ICCID
#define OPERATOR        AUTOMATIC_OPERATOR
#define M1_BAND_MASK    BAND_MASK_UNCHANGED
#define NB1_BAND_MASK   BAND_MASK_UNCHANGED

Sodaq_R4X r4x;
Sodaq_SARA_R4XX_OnOff saraR4xxOnOff;
Sodaq_R4X_MQTT r4x_mqtt;

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
    mqtt.setServer(_credentials->getSpace(), 1883);
    mqtt.setAuth(_credentials->getDeviceToken(), "arbitrary");
    mqtt.setClientId(generateUniqueID().c_str());
    mqtt.setKeepAlive(300);
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

// bool AllThingsTalk_LTEM::send(Payload &payload) {
    // if (isConnected()) {
        // if (payload.getPayloadType() == "cbor") {
            // char* topic;
            // int length = strlen(_credentials->getDeviceId()) + 14;  // 14 fixed chars + deviceId
            // topic = new char[length];
            // sprintf(topic, "device/%s/state", _credentials->getDeviceId());
            // topic[length-1] = 0;
            // return mqtt.publish(topic, payload.getBytes(), payload.getSize(), 0, 0);
        // } else if (payload.getPayloadType() == "json") {
            // char topic[128];
            // snprintf(topic, sizeof topic, "%s%s%s%s%s", "device/", _credentials->getDeviceId(), "/asset/", payload.getAssetName(), "/state");
            // debug("> Message Published to AllThingsTalk (JSON)");
            // debugVerbose("Asset:", ' ');
            // debugVerbose(payload.getAssetName(), ',');
            // debugVerbose(" Value:", ' ');
            // debugVerbose(payload.getString());
            // return mqtt.publish(topic, payload.getBytes(), payload.getSize(), 0, 0);
            // //return publishMqttMessage(assetName.c_str(), (char*)json.c_str(), true);
        // }
    // } else {
        // debug("Can't send message because you're not connected!");
        // return false;
    // }
// }

bool AllThingsTalk_LTEM::setCallback() {
    // TODO
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
    mqtt.loop();
}