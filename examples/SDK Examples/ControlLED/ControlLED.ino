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
 * Lets you control the built in LED of your board via the AllThingsTalk platform
 * 
 * HOW TO MAKE IT WORK:
 * -------------------------
 * - Create a device on your https://maker.allthingstalk.com (if you don't already have it)
 * - Create an actuator asset "led" of type Boolean
 * - Enter your credentials in the keys.h file
 * - Send a "true" or "false" value from that asset and your LED turn on/off
 */
 
#include <AllThingsTalk_LTEM.h>
#include "keys.h"               // You can use this or define your credentials here in this sketch

#define debugSerial SerialUSB   // Serial port used for debug purposes
#define modemSerial Serial1     // Serial port used for communicating with the LTE-M Modem
#define ledPin      LED_BUILTIN // LED_BUILTIN is the default LED on most boards, but you can change this to any PIN of your liking
 
APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
AllThingsTalk_LTEM att(modemSerial, credentials, APN);

void setup() {
  debugSerial.begin(115200);                            // Initialize the debug serial at 115200 baud rate
  pinMode(ledPin, OUTPUT);                              // Initialize the LED Pin
  while (!debugSerial && millis() < 10000) {}           // Give some time for the debug serial to initialize (gives up after 10 seconds)
  att.debugPort(debugSerial);                           // Set Serial port for serial monitor. Add argument "true" to see verbose debug output.
  att.setActuationCallback("led", controlLed); // Needs to be run before init()
  att.init();                                           // Initialize AllThingsTalk LTE-M SDK
}

void loop() {
  att.loop();                                           // Keeps the connection alive and checks for incoming messages
}

void controlLed(bool ledState) {                        // This callback is called when a message from your AllThingsTalk Maker is received
  debugSerial.print("Received Message: ");
  debugSerial.println(ledState);
  if (ledState) {
    digitalWrite(ledPin, HIGH);
    debugSerial.println("LED Turned on!");
  } else {
    digitalWrite(ledPin, LOW);
    debugSerial.println("LED Turned off!");
  }
}