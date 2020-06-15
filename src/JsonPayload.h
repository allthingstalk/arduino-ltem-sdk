#ifndef JSON_PAYLOAD_H_
#define JSON_PAYLOAD_H_

#include "Payload.h"

class JsonPayload : public Payload {
public:
	template<typename T> void set(char* assetName, T value);
	char* getAssetName();

	virtual char* getString();
	virtual unsigned char* getBytes();
    virtual unsigned int getSize();
    virtual void reset();
private:
	char* savedAssetName;
	char JSONmessageBuffer[256];
	unsigned char* string2ByteArray(char* input);
};

#endif