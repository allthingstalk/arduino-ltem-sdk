/*    _   _ _ _____ _    _              _____     _ _     ___ ___  _  __
 *   /_\ | | |_   _| |_ (_)_ _  __ _ __|_   _|_ _| | |__ / __|   \| |/ /
 *  / _ \| | | | | | ' \| | ' \/ _` (_-< | |/ _` | | / / \__ \ |) | ' <
 * /_/ \_\_|_| |_| |_||_|_|_||_\__, /__/ |_|\__,_|_|_\_\ |___/___/|_|\_\
 *                             |___/
 *
 * Copyright 2020 AllThingsTalk
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
 *
 * WHAT DOES THIS SKETCH DO:
 * -------------------------
 * When you have created a device on the AllThingsTalk Maker platform (https://maker.allthingstalk.com/)
 * with an asset 'counter' of kind sensor and type integer, this asset will be updated every 5 seconds.
 * If the counter exceeds 100 the counter will be reset to 1
 * 
 */
 
#include <AllThingsTalk_LTEM.h>
#include "keys.h"

#define debugSerial SerialUSB
#define modemSerial Serial1

APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
AllThingsTalk_LTEM modem(modemSerial, credentials, APN);
// You can choose if you want to send payloads using "CborPayload" or "JsonPayload"
// Check the README of this SDK to learn about the differences
CborPayload payload;
//JsonPayload payload;


int sendInterval = 5; // Sending interval in seconds
int counter = 1; // Initial value of counter (resets after reaching 10)
unsigned long previousMillis;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial  && millis() < 10000) {}
  modem.debugPort(debugSerial); // Set port for serialMonitor, true:full debug mode
  modem.init();
}

void loop() {  
  att.loop(); // Keep the network and connection towards AllThingsTalk alive
  if (millis() - previousMillis > sendInterval * 1000) {  // Send message every sendInterval seconds
    payload.reset();
    payload.set("counter", counter); // Sends current counter state to "counter" asset on AllThingsTalk
    debugSerial.print("Counter = ");
    debugSerial.println(count);
    if (modem.send(payload)){
      counter++;
      if (counter > 10) {
        counter = 1;
      }
    }
    previousMillis = millis();
  }
}
