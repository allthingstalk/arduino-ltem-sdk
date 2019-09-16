#ifndef APICredentials_h
#define APICredentials_h

#include "Arduino.h"

class APICredentials
{
	public:
		APICredentials(char* space, char* deviceToken = NULL, char* deviceId = NULL);
		
		char* getSpace();
		char* getDeviceToken();
		char* getDeviceId();
		
	private:
		char* _space;
		char* _deviceToken;
		char* _deviceId;
};

#endif