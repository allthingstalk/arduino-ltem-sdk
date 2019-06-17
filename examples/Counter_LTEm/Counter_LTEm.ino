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
 * When you have created a device on the AllThingsTalk Maker platform (https://maker.allthingstalk.com/)
 * with an asset 'counter', this asset will be updated every 5 seconds.
 * If the counter exceeds 100 the counter will be reset to 1
 * 
 */
 
#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>
#include <Device.h>

#include "keys.h"

#define debugSerial SerialUSB
#define loraSerial Serial1

void callback(const char* data);

APICredentials credentials(SPACE, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(loraSerial, debugSerial, credentials, false, callback);

CborPayload payload;

unsigned long previousMillis;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial  && millis() < 10000) {}

  if (modem.init(APN)) //wake up modem
  {
    debugSerial.print("Modem init succeeded");
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
    
    if (modem.send(payload))
    {
      count++;
      if (count > 100) count = 1;
    }
    else
    {
      debugSerial.print("Couldn't sent payload");
    }

    previousMillis = millis();
  }
}

void setSetupError(char* message)
{
  debugSerial.println(message);
  
  exit(0);
}

void callback(const char* data)
{
  debugSerial.println("**************** IN CALLBACK ********************");
  
  debugSerial.println(data);
}
