#include<LED.h>

#if defined(_VARIANT_SODAQ_SFF_) || defined(_VARIANT_SODAQ_SARA_)
	#define ledRed 		LED_RED
	#define ledGreen	LED_GREEN
	#define ledBlue 	LED_BLUE
#else
	#define ledRed		RED
	#define ledGreen	GREEN
	#define	ledBlue		BLUE
#endif

LED::LED()
{
	pinMode(ledRed, OUTPUT);
	pinMode(ledGreen, OUTPUT);
	pinMode(ledBlue, OUTPUT);

	digitalWrite(ledRed, HIGH);
	digitalWrite(ledGreen, HIGH);
	digitalWrite(ledBlue, HIGH);
}

void LED::setLight(lightColor color, bool animate, int animateCount)
{
	digitalWrite(ledRed, HIGH);
	digitalWrite(ledGreen, HIGH);
	digitalWrite(ledBlue, HIGH);

	switch (color)
	{
		case RED:
		  if (animate)
		  {
			for (int i = 0; i < animateCount; i++)
			{
			  digitalWrite(ledRed, LOW);
			  delay(500);
			  digitalWrite(ledRed, HIGH);
			  delay(500);
			}
		  }
		  digitalWrite(ledRed, LOW);
		  break;

		case GREEN:
		  if (animate)
		  {
			for (int i = 0; i < animateCount; i++)
			{
			  digitalWrite(ledGreen, LOW);
			  delay(500);
			  digitalWrite(ledGreen, HIGH);
			  delay(500);
			}
		  }
		  digitalWrite(ledGreen, LOW);
		  break;

		case BLUE:
		  if (animate)
		  {
			for (int i = 0; i < animateCount; i++)
			{
			  digitalWrite(ledBlue, LOW);
			  delay(500);
			  digitalWrite(ledBlue, HIGH);
			  delay(500);
			}
		  }
		  digitalWrite(ledBlue, LOW);
		  break;  

		case YELLOW:
		  if (animate)
		  {
			for (int i = 0; i < animateCount; i++)
			{
			  digitalWrite(ledRed, LOW);
			  digitalWrite(ledGreen, LOW);
			  delay(500);
			  digitalWrite(ledRed, HIGH);
			  digitalWrite(ledGreen, HIGH);
			  delay(500);
			}
		  }
		  digitalWrite(ledRed, LOW);
		  digitalWrite(ledGreen, LOW);
		  break;

		case MAGENTA:
		  if (animate)
		  {
			for (int i = 0; i < animateCount; i++)
			{
			  digitalWrite(ledRed, LOW);
			  digitalWrite(ledBlue, LOW);
			  delay(500);
			  digitalWrite(ledRed, HIGH);
			  digitalWrite(ledBlue, HIGH);
			  delay(500);
			}
		  }
		  digitalWrite(ledRed, LOW);
		  digitalWrite(ledBlue, LOW);
		  break;

		case CYAN:
		  if (animate)
		  {
			for (int i = 0; i < animateCount; i++)
			{
			  digitalWrite(ledGreen, LOW);
			  digitalWrite(ledBlue, LOW);
			  delay(500);
			  digitalWrite(ledGreen, HIGH);
			  digitalWrite(ledBlue, HIGH);
			  delay(500);
			}
		  }
		  digitalWrite(ledGreen, LOW);
		  digitalWrite(ledBlue, LOW);
		  break;

		case WHITE:
		  if (animate)
		  {
			for (int i = 0; i < animateCount; i++)
			{
			  digitalWrite(ledRed, LOW);
			  digitalWrite(ledGreen, LOW);
			  digitalWrite(ledBlue, LOW);
			  delay(500);
			  digitalWrite(ledRed, HIGH);
			  digitalWrite(ledGreen, HIGH);
			  digitalWrite(ledBlue, HIGH);
			  delay(500);
			}
		  }
		  digitalWrite(ledRed, LOW);
		  digitalWrite(ledGreen, LOW);
		  digitalWrite(ledBlue, LOW);
		  break;
	}
}
