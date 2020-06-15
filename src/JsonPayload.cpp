#include "ArduinoJson.h"
#include "JsonPayload.h"

template void JsonPayload::set(char *assetName, bool value);
template void JsonPayload::set(char *assetName, char *value);
template void JsonPayload::set(char *assetName, const char *value);
template void JsonPayload::set(char *assetName, String value);
template void JsonPayload::set(char *assetName, int value);
template void JsonPayload::set(char *assetName, byte value);
template void JsonPayload::set(char *assetName, short value);
template void JsonPayload::set(char *assetName, long value);
template void JsonPayload::set(char *assetName, float value);
template void JsonPayload::set(char *assetName, double value);

template<typename T> void JsonPayload::set(char* assetName, T value) {
    reset();
	savedAssetName = assetName;
	DynamicJsonDocument doc(256);
	doc["value"] = value;
    serializeJson(doc, JSONmessageBuffer);
}

char* JsonPayload::getAssetName() {
	return savedAssetName;
}

void JsonPayload::reset() {
	JSONmessageBuffer[0] = 0;
}

char* JsonPayload::getString() {
	return JSONmessageBuffer;
}

unsigned char* JsonPayload::getBytes() {
	return string2ByteArray(JSONmessageBuffer);
}

unsigned char* JsonPayload::string2ByteArray(char* input) {
    return (unsigned char*)input;
}

unsigned int JsonPayload::getSize() {
	return sizeof(JSONmessageBuffer);
}