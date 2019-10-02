#ifndef APICredentials_h
#define APICredentials_h

#include "Arduino.h"

class APICredentials
{
	public:
		APICredentials(const char* space, const char* deviceToken = NULL, const char* deviceId = NULL);
		
		const char* getSpace();
		const char* getDeviceToken();
		const char* getDeviceId();
		
		void setDeviceToken(const char* deviceToken);
		void setDeviceId(const char* deviceId);
		
	private:
		const char* _space;
		const char* _deviceToken;
		const char* _deviceId;
};

#endif