#ifndef Debuggable_h
#define Debuggable_h

#include <Arduino.h>

class Debuggable
{
	public:
		Debuggable();
		
		static void setMonitor(Stream* monitor)
		{
			_monitor = monitor;
		}
		
		static void print(char* debugString)
		{
			_monitor->print(debugString);
		}
		
		static void println(char* debugString)
		{
			_monitor->println(debugString);
		}
		
		static Stream* _monitor;
		
};

#endif