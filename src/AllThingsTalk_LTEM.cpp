#include "AllThingsTalk_LTEM.h"
#include <ArduinoJson.h>


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
    debug("------------- AllThingsTalk LTE-M Serial Begin -------------");
    if (verbose) {
        debugVerbose("Debug Level: Verbose. You'll also see AT Commands coming in and out from the Modem.");
        r4x.setDiag(debugSerial);
    } else {
        debug("Debug Level: Normal")
    }
}

bool AllThingsTalk_LTEM::init() {
    //TODO: PINS
    //TODO: LED
    mqtt.setServer(_credentials->getSpace(), 1883);
    mqtt.setAuth(_credentials->getDeviceToken(), "arbitrary");
    // TODO: ADD UNIQUE CLIENT ID GENERATION (MUST!)
    mqtt.setClientId("UFOHWEUFUEHWOHUFEWfwefwefwe");
    mqtt.setKeepAlive(300); // Seconds I suppose
    _modemSerial->begin(r4x.getDefaultBaudrate()); // The transport layer is a Sodaq_R4X
    r4x.init(&saraR4xxOnOff, *_modemSerial);
    //r4x.setDiag(DEBUG_STREAM);
    //r4x_mqtt.setR4Xinstance(&r4x, &AllThingsTalk_LTEM::connectNetwork); // Inform our mqtt instance that we use r4x as the transport
    //r4x_mqtt.setR4Xinstance(&r4x, std::bind(&AllThingsTalk_LTEM::connectNetwork, this));
    r4x_mqtt.setR4Xinstance(&r4x, []{ return true; } );
    mqtt.setTransport(&r4x_mqtt);
    return connect();
}

bool AllThingsTalk_LTEM::connect() {
    if (connectNetwork()) {
        connectMqtt();
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
        //mqtt.openMQTT();
    } else {
        debug("Already connected to MQTT!");
    }
}

bool AllThingsTalk_LTEM::disconnect() {
    return r4x.disconnect();
}

bool AllThingsTalk_LTEM::isConnected() {
    return r4x.isConnected();
}

bool AllThingsTalk_LTEM::send(Payload &payload) {
    if (isConnected()) {
        String rawPayload(payload.getString());
        int index = rawPayload.indexOf("|"); // Check if it's a JSON
        bool ok;
        if (index == -1) { // This is a CBOR Message
            char* topic;
            int length = strlen(_credentials->getDeviceId()) + 14;  // 14 fixed chars + deviceId
            topic = new char[length];
            sprintf(topic, "device/%s/state", _credentials->getDeviceId());
            topic[length-1] = 0;
            return mqtt.publish(topic, payload.getBytes(), payload.getSize(), 0, 0);
            delete(topic);
        } else {
            String assetName = rawPayload.substring(0, index);
            String jsonPayload = rawPayload.substring(index+1);
            char topic[128];
            snprintf(topic, sizeof topic, "%s%s%s%s%s", "device/", _credentials->getDeviceId(), "/asset/", assetName.c_str(), "/state");
            DynamicJsonDocument doc(256);
            char JSONmessageBuffer[256];
            doc["value"] = jsonPayload;
            serializeJson(doc, JSONmessageBuffer);
            debug("> Message Published to AllThingsTalk (JSON)");
            debugVerbose("Asset:", ' ');
            debugVerbose(assetName, ',');
            debugVerbose(" Value:", ' ');
            debugVerbose(jsonPayload);
            return mqtt.publish(topic, string2ByteArray(JSONmessageBuffer), payload.getSize(), 0, 0);
            //return publishMqttMessage(assetName.c_str(), (char*)json.c_str(), true);
        }
    } else {
        debug("Can't send message because you're not connected!");
        return false;
    }
}

BYTE* AllThingsTalk_LTEM::string2ByteArray(char* input) {
    int loop;
    int i;
    BYTE* output;
    loop = 0;
    i = 0;
    while(input[loop] != '\0') {
        output[i++] = input[loop++];
    }
    return output;
}


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