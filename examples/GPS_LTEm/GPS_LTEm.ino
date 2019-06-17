/*    _   _ _ _____ _    _              _____     _ _     ___ ___  _  __
 *   /_\ | | |_   _| |_ (_)_ _  __ _ __|_   _|_ _| | |__ / __|   \| |/ /
 *  / _ \| | | | | | ' \| | ' \/ _` (_-< | |/ _` | | / / \__ \ |) | ' <
 * /_/ \_\_|_| |_| |_||_|_|_||_\__, /__/ |_|\__,_|_|_\_\ |___/___/|_|\_\
 *                             |___/
 *
 * Copyright 2019 AllThingsTalk
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * WHAT DOES THIS SKETCH DO:
 * -------------------------
 * Send gps coordinates every 30 seconds to the AllThingsTalk platform to 
 * 
 * 
 * Explanation of the lights:
 * --------------------------
 * Blue: initialization with modem and making connection with backend
 * Magenta: subscribe to message
 * White: sending data
 * Green: blinking (initialisation successful), normal (sending successful)
 * Red: blinking (error initialisation, you have to reset to modem), normal (error in sending)
 */
 
#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>

#include <Wire.h>
#include <Sodaq_UBlox_GPS.h>

#include "keys.h"

#define debugSerial SerialUSB
#define loraSerial Serial1

void callback(const char* data);

APICredentials credentials(SPACE, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(loraSerial, debugSerial, credentials, false, callback);

CborPayload payload;

unsigned long previousMillis;

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
  while (!debugSerial && millis() < 10000) {}

  //factory reset modem, just for testing purposes
  //should be deleted in real sketch
  loraSerial.println("+UFACTORY");
  delay(2000);

  initiateLights();
  

  sodaq_gps.init(GPS_ENABLE);

  if (modem.init(APN)) //wake up modem
  {
    debugSerial.println("MQTT connection successful");
    setLight(green, true);     
  }
  else
  {
    setSetupError("Modem init failed");
  }
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

void setSetupError(char* message)
{
  debugSerial.println(message);

  setLight(red, true, 10);
  
  exit(0);
}

void loop() {  
  if (millis() - previousMillis > 30000) //send message every 30seconds
  {
    uint32_t timeout = 900L * 1000;
    if (sodaq_gps.scan(false, timeout)) //get gps fix 
    {
      GeoLocation geoLocation(sodaq_gps.getLat(), sodaq_gps.getLon(), 0);

      setLight(white);
      payload.reset();
      payload.set("Location", geoLocation);
      
      if (modem.send(payload))
      {
        setLight(green);
      }
      else
      {
        setLight(red);
      }
    }

    previousMillis = millis();
  }
}

void callback(const char* data)
{
  debugSerial.println("**************** IN CALLBACK ********************");
  
  debugSerial.println(data);
}
