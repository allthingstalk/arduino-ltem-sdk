#ifndef LED_h
#define LED_h

#include <Arduino.h>

class LED
{
	public:
		enum lightColor {
		  RED,
		  GREEN,
		  BLUE,
		  YELLOW,
		  MAGENTA,
		  CYAN,
		  WHITE,
		  OFF
		};
		
		LED();
		void setLight(lightColor color, bool animate = false, int animateCount = 3);
};

#endif

