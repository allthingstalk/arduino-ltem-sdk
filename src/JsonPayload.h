#ifndef JSON_PAYLOAD_H_
#define JSON_PAYLOAD_H_

#include "Payload.h"

class JsonPayload : public Payload {
public:
	void set(char* assetName, char* json);
	
	virtual char* getString();
	virtual unsigned char* getBytes();
    virtual unsigned int getSize();
    virtual void reset();
	
private:
	char* jsonString;

};

#endif