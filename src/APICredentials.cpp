#include "APICredentials.h"

APICredentials::APICredentials(char* broker, char* deviceToken, char* deviceId)
{
	_space = "api.allthingstalk.io";
	_deviceToken = deviceToken;
	_deviceId = deviceId;
}
		
char* APICredentials::getSpace()
{
	return _space;
}

char* APICredentials::getDeviceToken()
{
	return _deviceToken;
}

char* APICredentials::getDeviceId()
{
	return _deviceId;
}
