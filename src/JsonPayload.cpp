#include "JsonPayload.h"

#include <Arduino.h>

void JsonPayload::set(char* assetName, char* json)
{
	char temp[300];
	sprintf(temp, "%s|%s", assetName, json);
	
	jsonString = &temp[0];
}

void JsonPayload::reset() {
	jsonString = "";
}

char* JsonPayload::getString()
{
	return jsonString;
}

unsigned char* JsonPayload::getBytes() {
	return NULL;
}

unsigned int JsonPayload::getSize() {
	return -1;
}
