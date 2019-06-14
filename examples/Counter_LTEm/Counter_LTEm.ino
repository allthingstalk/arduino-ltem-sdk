#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>
#include <Device.h>

#include "keys.h"

#define debugSerial SerialUSB
#define loraSerial Serial1
#define powerPin SARA_ENABLE
#define enablePin -1

enum lightColor {
  red,
  green,
  blue,
  yellow,
  magenta,
  cyan,
  white,
  off
};

void callback(const char* data);

APICredentials credentials(SPACE, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(loraSerial, debugSerial, credentials, false, callback);

CborPayload payload;

unsigned long previousMillis;

void setLight(lightColor color, bool animate = false, int animateCount = 3)
{
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

  switch (color)
  {
    case red:
      if (animate)
      {
        for (int i = 0; i < animateCount; i++)
        {
          digitalWrite(LED_RED, LOW);
          delay(500);
          digitalWrite(LED_RED, HIGH);
          delay(500);
        }
      }
      digitalWrite(LED_RED, LOW);
      break;

    case green:
      if (animate)
      {
        for (int i = 0; i < animateCount; i++)
        {
          digitalWrite(LED_GREEN, LOW);
          delay(500);
          digitalWrite(LED_GREEN, HIGH);
          delay(500);
        }
      }
      digitalWrite(LED_GREEN, LOW);
      break;

    case blue:
      if (animate)
      {
        for (int i = 0; i < animateCount; i++)
        {
          digitalWrite(LED_BLUE, LOW);
          delay(500);
          digitalWrite(LED_BLUE, HIGH);
          delay(500);
        }
      }
      digitalWrite(LED_BLUE, LOW);
      break;  

    case yellow:
      if (animate)
      {
        for (int i = 0; i < animateCount; i++)
        {
          digitalWrite(LED_RED, LOW);
          digitalWrite(LED_GREEN, LOW);
          delay(500);
          digitalWrite(LED_RED, HIGH);
          digitalWrite(LED_GREEN, HIGH);
          delay(500);
        }
      }
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      break;

    case magenta:
      if (animate)
      {
        for (int i = 0; i < animateCount; i++)
        {
          digitalWrite(LED_RED, LOW);
          digitalWrite(LED_BLUE, LOW);
          delay(500);
          digitalWrite(LED_RED, HIGH);
          digitalWrite(LED_BLUE, HIGH);
          delay(500);
        }
      }
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_BLUE, LOW);
      break;

    case cyan:
      if (animate)
      {
        for (int i = 0; i < animateCount; i++)
        {
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_BLUE, LOW);
          delay(500);
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_BLUE, HIGH);
          delay(500);
        }
      }
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
      break;

    case white:
      if (animate)
      {
        for (int i = 0; i < animateCount; i++)
        {
          digitalWrite(LED_RED, LOW);
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_BLUE, LOW);
          delay(500);
          digitalWrite(LED_RED, HIGH);
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_BLUE, HIGH);
          delay(500);
        }
      }
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
      break;
  }
}

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial  && millis() < 10000) {}

  initiateLights();

  setLight(blue);
  if (modem.init(APN)) //wake up modem
  {
    setLight(green, true);
  }
  else
  {
    setSetupError("Modem init failed");
  }
}


int count = 1;
void loop() {  
  if (millis() - previousMillis > 5000) //send message every 5seconds
  {
    payload.reset();
    payload.set("counter", count);

    setLight(white);
    if (modem.send(payload))
    {
      count++;
      if (count > 100) count = 1;

      setLight(green);
    }
    else
    {
      setLight(red);
    }
    delay(1000);
    setLight(off);

    previousMillis = millis();
  }
}

void setSetupError(char* message)
{
  debugSerial.println(message);
  
  setLight(red, true, 10);
  
  exit(0);
}

void initiateLights()
{
  //initiating light pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

void callback(const char* data)
{
  debugSerial.println("**************** IN CALLBACK ********************");
  
  debugSerial.println(data);
}
