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
 * Unless requiled.RED by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * WHAT DOES THIS SKETCH DO:
 * -------------------------
 * Puts the device in deep sleep to conserve energy
 * Generates an interrupt when a shock has been detected and sends over a trigger state, timestamp and GPS location of the event.
 * 
 * you can set the G-force and sensitivity for the shock detection:
 * accPercentage: value in %, 100 = 8G
 * accThreshold: value between xx and yy ; The lower the value, the faster an interrupt will be generated
 * 
 * Explanation of the lights:
 * --------------------------
 * Blue: initialization with att and making connection with backend
 * Yellow: searching for GPS signal
 * White: sending data
 * Green: success
 * Red: error
 * 
 * Dependencies:
 * --------------------------
 * This sketch uses external libraries that were added alongside the sketch itself for convenience.
 * - "Time" by Michael Margolis, maintained by Paul Stoffregen
 * - "Sodaq_UBlox_GPS" by Keestux and SODAQ, maintained by Kees Bakker
 * - "Sodaq_LSM303AGR" by Alex Tsamakos and SODAQ, maintained by Kees Bakker
 */

#include <AllThingsTalk_LTEM.h>
#include <LED.h>
#include <Wire.h>
#include "src/TimeLib.h"
#include "src/Sodaq_UBlox_GPS.h"
#include "src/Sodaq_LSM303AGR.h"
#include "keys.h"

#define debugSerial SerialUSB
#define modemSerial Serial1

struct unix {
  long get(int y, int m = 0, int d = 0, int h = 0, int i = 0, int s = 0) {
    setTime(h, i, s, d, m, y);
    adjustTime(-10800); // +3
    return now();
  }
} unix;

APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
AllThingsTalk_LTEM att(modemSerial, credentials, APN);
CborPayload payload;
Sodaq_LSM303AGR accel;
LED led;

unsigned long previousMillis;
volatile bool magInterruptFlag  = true;
volatile bool suddenMovement    = false;
double        accPercentage     = 80.0;   // Acceleration% (100% = 8g)
double        accThreshold      = 1.0;    // The lower the value, the quicker an interrupt will be generated
int           accDuration       = 0;      // Acceleration Duration
const int     GPS_TIMEOUT       = 15000;
const int     GPS_RETRY_COUNT   = 1;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial && millis() < 10000) {}
  att.debugPort(debugSerial);

  debugSerial.println("EXPLANATION OF LIGHT SEQUENCE");
  debugSerial.println("-----------------------------");
  debugSerial.println("BLUE: Trying to initialise AllThingsTalk");
  debugSerial.println("--> GREEN: Initialization succeeded");
  debugSerial.println("--> RED: Initialization failed");
  debugSerial.println();
  debugSerial.println("WHEN SHOCK DETECTED");
  debugSerial.println("-------------------");
  debugSerial.println("YELLOW: Trying to get GPS fix");
  debugSerial.println("--> GREEN: GPS Fix Found");
  debugSerial.println("--> RED: No GPS Fix");
  debugSerial.println();
  debugSerial.println("WHITE: Trying to send data to AllThingsTalk");
  debugSerial.println("--> GREEN: Sending successful");
  debugSerial.println("--> RED: Sending failed");
  debugSerial.println();
  
  Wire.begin();
  delay(1000);

  initAssets();
  initiateAccelerometer();  
}

void initAssets() {
  wakeUp();
  payload.reset();
  payload.set("ShockDetected", false);
  sendPayload(payload);
}

void initiateAccelerometer() {
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
      accPercentage * accThreshold / 100.0,
      accDuration,
      Sodaq_LSM303AGR::MovementRecognition);
}

void wakeUp() {
  USB->DEVICE.CTRLA.reg |= USB_CTRLA_ENABLE;
  USBDevice.attach();
  
  pinMode(SARA_ENABLE, OUTPUT);
  digitalWrite(SARA_ENABLE, HIGH);

  pinMode(SARA_TX_ENABLE, OUTPUT);
  digitalWrite(SARA_TX_ENABLE, HIGH);

  sodaq_gps.init(GPS_ENABLE);

  led.setLight(led.BLUE);
  if (att.init()) { // Initialize AllThingsTalk LTE-M SDK
     led.setLight(led.GREEN, true);     
  } else {
    setSetupError("AllThingsTalk LTE-M Initialization Failed");
  }
}

void interrupt_event() {
  suddenMovement = true;
}

void setSetupError(char* message) {
  debugSerial.println(message);
  led.setLight(led.RED, true, 10);
  exit(0);
}

void loop() {  
  att.loop(); // Keep the network and connection towards AllThingsTalk alive
  //If shock detected, send gps coördinates and timestamp to backend
  if(suddenMovement) {
    accel.disableAccelerometer();
    suddenMovement = false;
    int count = 0;
    debugSerial.println("***************************** SHOCK DETECTED ****************************");
    debugSerial.print("Searching gps signal");
    led.setLight(led.YELLOW);

    while (!sodaq_gps.scan(false, GPS_TIMEOUT) && count < GPS_RETRY_COUNT) { //wait for fix
      debugSerial.print(".");
      delay(1000);
      count++;
    }
    debugSerial.println();
    led.setLight(led.OFF);

    if (count < GPS_RETRY_COUNT) {     
      GeoLocation geoLocation(sodaq_gps.getLat(), sodaq_gps.getLon(), 0);
      payload.reset();
      payload.set("loc", geoLocation);
    } else {
      GeoLocation geoLocation(0, 0);
      payload.reset();
      payload.set("loc", geoLocation);
    }

    payload.set("shock", true);
    sendPayload(payload);
    accel.enableAccelerometer();
  }
}

void sendPayload(CborPayload payload) {
  led.setLight(led.WHITE);
  if (att.send(payload)) {
    led.setLight(led.GREEN, true);
  } else {
    led.setLight(led.RED, true);
  }
  delay(1000);
  led.setLight(led.OFF);
}