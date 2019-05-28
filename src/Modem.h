#ifndef Modem_h
#define Modem_h

#include "Credentials.h"
#include "Payload.h"
#include "Options.h"

class Modem
{
	public:	
		virtual unsigned int getDefaultBaudRate();
		virtual void setOptions(Options* options);
		virtual void send(Payload* payload);
		virtual bool init();
};
	
#endif

