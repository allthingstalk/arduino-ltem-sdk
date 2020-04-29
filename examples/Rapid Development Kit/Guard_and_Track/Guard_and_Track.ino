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
 * This sketch can be used as a lock mechanism for your bicycle, motor, car, ...
 * When you send an actuation 'Lock', the device comes in lock state and when there is movement
 * an alarm is triggered and gps coordinates are sent if possible.
 * As long as the device is in lock state and an alarm was triggered it will try to send
 * the location to the backend every minute
 * 
 * Explanation of the lights:
 * --------------------------
 * Blue: initialization with modem and making connection with backend
 * Magenta: subscribe to message
 * Yellow: trying to get GPS fix
 * White: sending data
 * Green: blinking (initialisation successful), normal (sending successful)
 * Red: blinking (error initialisation, you have to reset to modem), normal (error in sending)
 */
 
#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>
#include <LED.h>

#include <Wire.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <Sodaq_UBlox_GPS.h>
#include <Sodaq_LSM303AGR.h>

#include "keys.h"

#define debugSerial SerialUSB
#define ltemSerial Serial1
#define powerPin SARA_ENABLE
#define enablePin -1

struct unix {
  long get(int y, int m = 0, int d = 0, int h = 0, int i = 0, int s = 0) {
    setTime(h, i, s, d, m, y);
    adjustTime(-10800); // +3
    return now();
  }
} unix;

void callback(const char* data);

APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(ltemSerial, debugSerial, credentials, false, callback); //false: disables full debug mode
LED led;

CborPayload payload;
Sodaq_LSM303AGR accel;

unsigned long previousMillis;
volatile bool acc_int_flag = false;

double acc = 80.0;   // Acceleration% (100% = 8g)
double threshhold = 1.0;  //The lower the value, the quicker an interrupt will be generated
int acd = 0;  // Acceleration Duration

bool alarm = false;
bool lock = false;



void setup() {
  debugSerial.begin(115200);
  while (!debugSerial && millis() < 10000) {}

  initGPS();
  initAccelerometer();  
  accel.disableAccelerometer(); 

  led.setLight(led.BLUE);
  if (modem.init(APN)) //initialise modem
  {
    led.setLight(led.GREEN, true);

    debugSerial.println("Ready");
    debugSerial.println("Subscribe to lock asset");

    led.setLight(led.MAGENTA);
    if (modem.listen("lock"))
    {
      debugSerial.println("Subscribe succeeded");
      
      led.setLight(led.GREEN, true);
      delay(1000);
      led.setLight(led.OFF);
    }
    else
    {
      setSetupError("Could not subscribe to message");
    }
  }
  else
  {
    setSetupError("Modem init failed");
  }
}

void sendData(bool shock = false)
{
  led.setLight(led.YELLOW);

  debugSerial.println("Trying to get GPS fix (timeout 2 minutes)");
    
  uint32_t timeout = 120000;
  if (sodaq_gps.scan(false, timeout)) //wait for fix
  {      
    led.setLight(led.WHITE);
    GeoLocation geoLocation(sodaq_gps.getLat(), sodaq_gps.getLon(), 0);
    payload.reset();

    if (shock)
      payload.set("shock", true);
      
    payload.set("loc", geoLocation);
    payload.set("accel", accel.getX());
  }
  else
  {
    GeoLocation geoLocation(0, 0, 0);
    payload.reset();

    if (shock)
      payload.set("shock", true);
      
    payload.set("loc", geoLocation);
    payload.set("accel", accel.getX());
  }

  debugSerial.println("Sending payload");
  if (modem.send(payload))
  {
    led.setLight(led.GREEN);
    debugSerial.println("Sending succesful");
  }
  else
  {
    led.setLight(led.RED);
    debugSerial.println("Sending failed");
  }

  delay(1000);
  led.setLight(led.OFF);
}

void loop() {  
  payload.reset();
  modem.send(payload);
  delay(2000);
  
  if (acc_int_flag && lock)
  {
    accel.disableAccelerometer();
    
    acc_int_flag = false;
    alarm = true;
    
    debugSerial.println("Shock Detected");

    sendData(true);
  }

  //if there is an alarm, send location every 60 seconds
  if (alarm && millis() - previousMillis > 60000)
  {
    sendData();
    previousMillis = millis();
  }
}

void initAccelerometer()
{
  accel.rebootAccelerometer();
  delay(1000);
  
  pinMode(ACCEL_INT1, INPUT);
  attachInterrupt(ACCEL_INT1, interrupt_event, CHANGE);

  SYSCTRL->XOSC32K.bit.RUNSTDBY = 1; //Make sure interrupt is triggered when in deepsleep mode

  // Enable interrupts on the SAMD 
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_EIC) |
      GCLK_CLKCTRL_GEN_GCLK1 |
      GCLK_CLKCTRL_CLKEN;

 // Enable the Accelerometer
 accel.enableAccelerometer(
      Sodaq_LSM303AGR::LowPowerMode,
      Sodaq_LSM303AGR::HrNormalLowPower10Hz,
      Sodaq_LSM303AGR::XYZ,
      Sodaq_LSM303AGR::Scale8g,
      true);
      
  delay(100);
      
  // If XYZ goes below or above threshold the interrupt is triggered
  accel.enableInterrupt1(
      Sodaq_LSM303AGR::XHigh | Sodaq_LSM303AGR::XLow | Sodaq_LSM303AGR::YHigh | Sodaq_LSM303AGR::YLow | Sodaq_LSM303AGR::ZHigh | Sodaq_LSM303AGR::ZLow,
      acc * threshhold / 100.0,
      acd,
      Sodaq_LSM303AGR::MovementRecognition);
}

void initGPS()
{
  sodaq_gps.init(GPS_ENABLE);
}

void setSetupError(char* message)
{
  debugSerial.println(message);

  led.setLight(led.RED, true, 10);
  
  exit(0);
}

void interrupt_event()
{
  acc_int_flag = true;
  accel.disableAccelerometer(); 
}

void callback(const char* data)
{
  debugSerial.println(data);
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);

  bool locked = root["value"].as<bool>();
  if (locked)
  {
    debugSerial.println("Device locked");
    accel.enableAccelerometer();
    
    lock = true;

    //Send the incoming value to backend to be sure the lock is activated
    payload.reset();
    payload.set("lock", true);
    modem.send(payload);
  }
  else
  {
    debugSerial.println("Device Unlocked");
    accel.disableAccelerometer();
    
    lock = false;
    alarm = false; //device is unlocked, disable alarm
    acc_int_flag = false;

    //Send the incoming value to backend to be sure the lock is deactivated
    payload.reset();
    payload.set("lock", false);
    modem.send(payload);
  }
}
