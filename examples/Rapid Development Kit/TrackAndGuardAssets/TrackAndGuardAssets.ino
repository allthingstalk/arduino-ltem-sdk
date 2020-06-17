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
 * This sketch transforms your device to a lock for your car, motor, bicycle or whatever else crosses your mind!
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
volatile bool suddenMovement = false;
double        accPercentage  = 80.0;    // Acceleration % (100% = 8g)
int           accDuration    = 0;       // Acceleration Duration
double        accThreshhold  = 1.0;     // Sensitivity of the accelerometer. The lower the value, the quicker an interrupt will be generated.
int           gpsTimeout     = 120;     // Number of seconds before GPS fix times out
int           alarmSendInterval = 60;   // Seconds
bool          lock           = false;
bool          alarm          = false;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial && millis() < 10000) {}
  att.debugPort(debugSerial);
  led.setLight(led.MAGENTA);
  att.setActuationCallback("lock", lockCallback);
  led.setLight(led.GREEN, true);
  delay(1000);
  led.setLight(led.OFF);
  initGPS();
  initAccelerometer();
  accel.disableAccelerometer();
  led.setLight(led.BLUE);
  if (att.init()) {
    led.setLight(led.GREEN, true);
  } else {
    setSetupError("AllThingsTalk LTE-M Initialization Failed");
  }
}

void sendData(bool shock = false) {
  led.setLight(led.YELLOW);  // Yellow - Searching for GPS
  debugSerial.println("Trying to get GPS fix (timeout 2 minutes)");
  if (sodaq_gps.scan(false, gpsTimeout * 1000)) { // Wait for GPS fix
    led.setLight(led.WHITE);
    GeoLocation geoLocation(sodaq_gps.getLat(), sodaq_gps.getLon(), 0); // Saves the current GPS coordinates
    payload.reset();
    if (shock) payload.set("shock", true);
    payload.set("loc", geoLocation);
    payload.set("accel", accel.getX());
  } else {
    GeoLocation geoLocation(0, 0, 0); // Reset the GPS coordinates
    payload.reset();
    if (shock) payload.set("shock", true);
    payload.set("loc", geoLocation);
    payload.set("accel", accel.getX());
  }
  if (att.send(payload)) { // Send the payload
    led.setLight(led.GREEN);
  } else {
    led.setLight(led.RED);
  }
  delay(1000);
  led.setLight(led.OFF);
}

void initAccelerometer() {
  accel.rebootAccelerometer();
  delay(1000);
  pinMode(ACCEL_INT1, INPUT);
  attachInterrupt(ACCEL_INT1, interrupt_event, CHANGE);
  SYSCTRL->XOSC32K.bit.RUNSTDBY = 1; // Make sure interrupt is triggered when in deepsleep mode
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
      accPercentage * accThreshhold / 100.0,
      accDuration,
      Sodaq_LSM303AGR::MovementRecognition);
}

void initGPS() {
  sodaq_gps.init(GPS_ENABLE);
}

void setSetupError(char* message) {
  debugSerial.println(message);
  led.setLight(led.RED, true, 10);
  exit(0);
}

void interrupt_event() {
  suddenMovement = true;
  accel.disableAccelerometer();
}

void lockCallback(bool locked) {
  if (locked) {
    debugSerial.println("Device Locked");
    accel.enableAccelerometer();
    lock = true;
    payload.reset();
    payload.set("lock", true); // Send the incoming value to backend to be sure the lock is activated
    att.send(payload);
  } else {
    debugSerial.println("Device Unlocked");
    accel.disableAccelerometer();
    lock = false;
    alarm = false; // Device is unlocked, disable alarm
    suddenMovement = false;
    payload.reset();
    payload.set("lock", false); //Send the incoming value to backend to be sure the lock is deactivated
    att.send(payload);
  }
}

void loop() {
  // Keep the modem and the connection towards AllThingsTalk alive
  att.loop(); 

  // If there's a shock and your device is locked, gather and send data to AllThingsTalk
  if (suddenMovement && lock) {
    accel.disableAccelerometer();
    suddenMovement = false;
    alarm = true;
    debugSerial.println("Shock Detected");
    sendData(true);
  }
  
  // Send location every [alarmSendInterval] if alarm is enabled
  if (alarm && millis() - previousMillis > alarmSendInterval*1000) {
    sendData();
    previousMillis = millis();
  }
}