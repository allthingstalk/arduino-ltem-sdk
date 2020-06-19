#include "APICredentials.h"

APICredentials::APICredentials(const char* broker, const char* deviceToken, const char* deviceId)
{
	_space = broker;
	_deviceToken = deviceToken;
	_deviceId = deviceId;
}
		
const char* APICredentials::getSpace()
{
	return _space;
}

const char* APICredentials::getDeviceToken()
{
	return _deviceToken;
}

const char* APICredentials::getDeviceId()
{
	return _deviceId;
}

void APICredentials::setDeviceToken(const char* deviceToken)
{
	_deviceToken = deviceToken;
}

void APICredentials::setDeviceId(const char* deviceId)
{
	_deviceId = deviceId;
}
