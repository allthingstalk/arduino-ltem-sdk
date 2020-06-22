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
 * WHAT DOES THIS SKETCH DO:
 * -------------------------
 * It will monitor the temperature and humidity and send those values to AllThingsTalk Cloud
 * This sketch is part of the AllThingsTalk LTE-M Rapid Development Kit
 *
 * LED behaviour:
 * ---------------------------------
 * Blue: initialization with att and making connection with backend
 * Yellow: reading temperature and humidity
 * White: sending data
 * Green: success
 * Red: error
 * 
 * Dependencies:
 * --------------------------
 * This sketch uses external libraries that were added alongside the sketch itself for convenience.
 * - "Adafruit Unified Sensor" by Adafruit
 * - "Adafruit BME280" by Adafruit
 * - "Seeed BME280" by Seeed Studio
 */

#include <AllThingsTalk_LTEM.h>
#include <LED.h>
#include "Adafruit_BME280.h"
#include "Seeed_BME280.h"
#include "keys.h"

#define debugSerial SerialUSB
#define modemSerial Serial1

APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
AllThingsTalk_LTEM att(modemSerial, credentials, APN);
CborPayload payload;
Adafruit_BME280 tph1;
BME280 tph2;
LED led;

unsigned long previousMillis;
float temperature, humidity;
uint8_t sensorType;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial && millis() < 10000) {}
  att.debugPort(debugSerial);
  Wire.begin();
  delay(1000);
  initTphSensor();
  led.setLight(led.BLUE);
  if (att.init()) {
    led.setLight(led.GREEN, true);
  } else {
    setSetupError("AllThingsTalk LTE-M Initialization Failed");
  }
}

void initTphSensor() {
  if (tph1.begin()) {
    // Sensor will use Adafruit Library
    sensorType = 1;
  } else if (tph2.init()) {
    // Sensor will use Seeed Library
    sensorType = 2;
  } else {    
    setSetupError("Could not initialize TPH sensor, please check wiring");
    exit(0);
  }
}

bool readTphData() {
  if (sensorType == 1) {
    temperature = tph1.readTemperature();
    humidity = tph1.readHumidity();
    return true;
  } else if (sensorType == 2) {
    temperature = tph2.getTemperature();
    humidity = tph2.getHumidity();
    return true;
  } else {
    debugSerial.println("Failed to get Temperature/Humidity data this time.");
    return false;
  }
}

void setSetupError(char* message) {
  debugSerial.println(message);
  led.setLight(led.RED, true, 10);
  exit(0);
}

void loop() {
  att.loop(); // Keep the network and connection to AllThingsTalk alive
  led.setLight(led.YELLOW);
  readTphData();
  delay(1000);
  led.setLight(led.OFF);
  payload.reset();
  payload.set("temp", temperature);
  payload.set("hum", humidity);
  debugSerial.print("Temperature: ");
  debugSerial.println(temperature);
  debugSerial.print("Humidity: ");
  debugSerial.println(humidity);
  led.setLight(led.WHITE);
  if (att.send(payload)) {
    led.setLight(led.GREEN, true);
  } else {
    led.setLight(led.RED, true);
  }
  delay(1000);
  led.setLight(led.OFF);
  delay(30000);
}