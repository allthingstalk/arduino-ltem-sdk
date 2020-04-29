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
 * Unless requiled.RED by applicable law or agreed to in writing, software
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
 * Puts the device in deep sleep to conserve energy
 * Generates an interrupt when a shock has been detected and sends over a trigger state, timestamp and GPS location of the event.
 * 
 * you can set the G-force and sensitivity for the shock detection:
 * acc: value in %, 100 = 8G
 * threshold: value between xx and yy ; The lower the value, the faster an interrupt will be generated
 * 
 * 
 * 
 * Explanation of the lights:
 * --------------------------
 * Blue: initialization with modem and making connection with backend
 * Yellow: searching for GPS signal
 * White: sending data
 * Green: success
 * Red: error
 */

 
#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>
#include <Device.h>
#include <LED.h>

#include <Wire.h>
#include <TimeLib.h>
#include <Sodaq_UBlox_GPS.h>
#include <Sodaq_LSM303AGR.h>

#include "keys.h"

#define debugSerial SerialUSB
#define ltemSerial Serial1

void callback(const char* data);

APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(ltemSerial, debugSerial, credentials, false, callback); // false: disables full debug mode

CborPayload payload;
LED led;

Sodaq_LSM303AGR accel;

unsigned long previousMillis;
volatile bool magInterruptFlag = true;
volatile bool acc_int_flag = false;

double acc = 80.0;   // Acceleration% (100% = 8g)
double threshhold = 1.0;  //The lower the value, the quicker an interrupt will be generated
int acd = 0;  // Acceleration Duration

const int GPS_TIMEOUT = 15000;
const int GPS_RETRY_COUNT = 1;

struct unix {
  long get(int y, int m = 0, int d = 0, int h = 0, int i = 0, int s = 0) {
    setTime(h, i, s, d, m, y);
    adjustTime(-10800); // +3
    return now();
  }
} unix;


void setup() {
  debugSerial.begin(115200);
  while (!debugSerial && millis() < 10000) {}

  debugSerial.println("EXPLANATION OF LIGHT SEQUENCE");
  debugSerial.println("-----------------------------");
  debugSerial.println("BLUE: trying to initialise modem");
  debugSerial.println("--> GREEN: init succeeded");
  debugSerial.println("--> RED: init failed");
  debugSerial.println();
  debugSerial.println("WHEN SHCOK DETECTED");
  debugSerial.println("-------------------");
  debugSerial.println("YELLOW: trying to get GPS fix");
  debugSerial.println("--> GREEN: GPS fix found");
  debugSerial.println("--> RED: No GPS fix");
  debugSerial.println();
  debugSerial.println("WHITE: trying to send data to AllThingsTalk platform");
  debugSerial.println("--> GREEN: sending successful");
  debugSerial.println("--> RED: sending failed");
  debugSerial.println();
  

  Wire.begin();
  delay(1000);

  initAssets();
  initiateAccelerometer();  
}

void initAssets()
{
  wakeUp();

  payload.reset();
  payload.set("ShockDetected", false);
  sendPayload(payload);
}

void initiateAccelerometer()
{
  accel.rebootAccelerometer();
  delay(1000);

  pinMode(ACCEL_INT1, INPUT);
  attachInterrupt(ACCEL_INT1, interrupt_event, CHANGE);

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
      
  // If XYZ goes below or above threshold the interrupt is triggeled.RED
  accel.enableInterrupt1(
      Sodaq_LSM303AGR::XHigh | Sodaq_LSM303AGR::XLow | Sodaq_LSM303AGR::YHigh | Sodaq_LSM303AGR::YLow | Sodaq_LSM303AGR::ZHigh | Sodaq_LSM303AGR::ZLow,
      acc * threshhold / 100.0,
      acd,
      Sodaq_LSM303AGR::MovementRecognition);
}

void wakeUp()
{
  USB->DEVICE.CTRLA.reg |= USB_CTRLA_ENABLE;
  USBDevice.attach();
  
  pinMode(SARA_ENABLE, OUTPUT);
  digitalWrite(SARA_ENABLE, HIGH);

  pinMode(SARA_TX_ENABLE, OUTPUT);
  digitalWrite(SARA_TX_ENABLE, HIGH);

  sodaq_gps.init(GPS_ENABLE);

  led.setLight(led.BLUE);
  if (modem.init(APN)) //wake up modem
  {
     led.setLight(led.GREEN, true);     
  }
  else
  {
    setSetupError("Modem init failed");
  }
}

void interrupt_event()
{
  acc_int_flag = true;
}

void setSetupError(char* message)
{
  debugSerial.println(message);
  led.setLight(led.RED, true, 10);
  exit(0);
}

void loop() {  
  //If shock detected, send gps coördinates and timestamp to backend
  if(acc_int_flag) 
  {
    accel.disableAccelerometer();
    acc_int_flag = false;

    int count = 0;

    debugSerial.println("***************************** SHOCK DETECTED ****************************");

    debugSerial.print("Searching gps signal");
    led.setLight(led.YELLOW);
 
    while (!sodaq_gps.scan(false, GPS_TIMEOUT) && count < GPS_RETRY_COUNT) //wait for fix
    {
      debugSerial.print(".");
      delay(1000);
      count++;
    }
    debugSerial.println();
    led.setLight(led.OFF);

    CborPayload pl;
    
    if (count < GPS_RETRY_COUNT) 
    {     
      GeoLocation geoLocation(sodaq_gps.getLat(), sodaq_gps.getLon(), 0);
      
      pl.reset();
      pl.set("shock", true);
      pl.set("loc", geoLocation);

      sendPayload(pl);
    }
    else
    {
      GeoLocation geoLocation(0, 0);
      
      pl.reset();
      pl.set("shock", true);
      pl.set("loc", geoLocation);
    }

    sendPayload(pl);
    
    accel.enableAccelerometer();
  }
}

void sendPayload(CborPayload payload)
{
  led.setLight(led.WHITE);
  if (modem.send(payload))
    led.setLight(led.GREEN, true);
  else
    led.setLight(led.RED, true);

  delay(1000);
  led.setLight(led.OFF);
}

void callback(const char* data)
{
  debugSerial.println("**************** IN CALLBACK ********************");
  
  debugSerial.println(data);
}
