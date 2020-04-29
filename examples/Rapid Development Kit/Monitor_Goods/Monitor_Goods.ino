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
 * 
 * 
 *
 * WHAT DOES THIS SKETCH DO:
 * -------------------------
 * It will monitor the temperature and humidity and send those values to AllThingsTalk Cloud
 * This sketch is part of the LTE-m kit
 * The LED debug
 * 
 * 
 * 
 * LED behaviour:
 * ---------------------------------
 * Blue: initialization with modem and making connection with backend
 * Yellow: reading temperature and humidity
 * White: sending data
 * Green: success
 * Red: error
 */
 
#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>
#include <LED.h>

#include <Adafruit_BME280.h>

#include "keys.h"

#define debugSerial SerialUSB
#define ltemSerial Serial1

void callback(const char* data);

APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(ltemSerial, debugSerial, credentials, false, callback); //false: disables full debug mode

CborPayload payload;
LED led;

Adafruit_BME280 tph; 

unsigned long previousMillis;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial && millis() < 10000) {}

  Wire.begin();
  delay(1000);

  initTPHSensor();  //make tph sensor ready for use

  led.setLight(led.BLUE);
  if (modem.init(APN)) //initialise modem
  {
    debugSerial.println("Modem init succeeded");
    led.setLight(led.GREEN, true);
  }
  else
  {
    debugSerial.println("Modem init failed");
    setSetupError("Modem init failed");
  }
}

float temperature;
float humidity;

void loop() {  
  led.setLight(led.YELLOW);
  
  temperature = tph.readTemperature();
  humidity = tph.readHumidity();
  
  delay(1000);
  led.setLight(led.OFF);

  payload.reset();
  payload.set("temp", temperature);
  payload.set("hum", humidity);

  debugSerial.print("Temperature: ");
  debugSerial.println(temperature);
  debugSerial.print("Humidity: ");
  debugSerial.println(humidity);

  led.setLight(led.WHITE); //white light, start sending
  debugSerial.println("Trying to send payload");
  if (modem.send(payload))
  {
    debugSerial.println("Sending succeeded");
    led.setLight(led.GREEN, true); //green light, sending successful
  }
  else
  {
    debugSerial.println("Sending failed");
    led.setLight(led.RED, true); //red light, sending unsuccessful
  }
  delay(1000);
  led.setLight(led.OFF);

  delay(30000);
}

void initTPHSensor()
{
  if (!tph.begin()) 
  {
    setSetupError("Could not initialize TPH sensor, please check wiring");
    exit(0);
  }
}

void setSetupError(char* message)
{
  debugSerial.println(message);
  led.setLight(led.RED, true, 10);
  
  exit(0);
}

void callback(const char* data)
{
  debugSerial.println("**************** IN CALLBACK ********************");
  
  debugSerial.println(data);
}
