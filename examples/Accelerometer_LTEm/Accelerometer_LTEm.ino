#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>
#include <Device.h>

#include <Wire.h>
#include <TimeLib.h>
#include <Sodaq_UBlox_GPS.h>
#include <Sodaq_LSM303AGR.h>

#include "keys.h"

#define debugSerial SerialUSB
#define ltemSerial Serial1
#define powerPin SARA_ENABLE
#define enablePin -1

void callback(const char* data);

APICredentials credentials(SPACE, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(ltemSerial, debugSerial, credentials, true, callback);

CborPayload payload;

Sodaq_LSM303AGR accel;

unsigned long previousMillis;
volatile bool magInterruptFlag = true;
volatile bool acc_int_flag = false;

double acc = 80.0;   // Acceleration% (100% = 8g)
double threshhold = 1.0;  //The lower the value, the quicker an interrupt will be generated
int acd = 0;  // Acceleration Duration

const int GPS_RETRY_COUNT = 5;

struct unix {
  long get(int y, int m = 0, int d = 0, int h = 0, int i = 0, int s = 0) {
    setTime(h, i, s, d, m, y);
    adjustTime(-10800); // +3
    return now();
  }
} unix;

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
  //ltemSerial.println("+UFACTORY");
  //delay(2000);

  Wire.begin();
  delay(1000);

  initiateAccelerometer();  
  deepSleep();
}

void initiateAccelerometer()
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

void deepSleep()
{
  accel.disableMagnetometer();

  pinMode(MAG_INT, OUTPUT);
  pinMode(GPS_ENABLE, OUTPUT);
  pinMode(SARA_ENABLE, OUTPUT);

  digitalWrite(MAG_INT, LOW); // we need to make this low otherwise this pin on the LSM303AGR starts leaking current
  digitalWrite(GPS_ENABLE, LOW); // low=poweredoff, high=poweredon
  digitalWrite(SARA_ENABLE, LOW); // low=poweredoff, high=poweredon

  debugSerial.flush();
  debugSerial.end();
  USBDevice.detach();
  USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;

  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  __WFI();
}

void wakeUp()
{
  USB->DEVICE.CTRLA.reg |= USB_CTRLA_ENABLE;
  USBDevice.attach();
  
  pinMode(SARA_ENABLE, OUTPUT);
  digitalWrite(SARA_ENABLE, HIGH);

  pinMode(SARA_TX_ENABLE, OUTPUT);
  digitalWrite(SARA_TX_ENABLE, HIGH);
  
  initiateLights();

  sodaq_gps.init(GPS_ENABLE);

  setLight(blue);
  if (modem.init(APN)) //wake up modem
  {
     setLight(green);     
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

void interrupt_event()
{
  acc_int_flag = true;
}

void setSetupError(char* message)
{
  debugSerial.println(message);
  setLight(red);
  exit(0);
}

void loop() {  
  //If shock detected, send gps coördinates and timestamp to backend
  if(acc_int_flag) 
  {
    int count = 1;
    
    accel.disableAccelerometer();
    acc_int_flag = false;

    Wire.begin();
    wakeUp();
      
    payload.reset();
    payload.set("ShockDetected", true);
    modem.send(payload);
    
    digitalWrite(LED_BLUE, LOW);
    
    debugSerial.println("***************************** SHOCK DETECTED ****************************");

    debugSerial.print("Searching gps signal");
    setLight(yellow);
    uint32_t timeout = 5000;
    while (!sodaq_gps.scan(false, timeout) && count < GPS_RETRY_COUNT) //wait for fix
    {
      debugSerial.print(".");
      delay(1000);
      count++;
    }
    debugSerial.println();
    setLight(off);
    
    if (count < GPS_RETRY_COUNT) 
    {      
      uint8_t year = sodaq_gps.getYear();
      uint8_t month = sodaq_gps.getMonth();
      uint8_t day = sodaq_gps.getDay();
      uint8_t hour = sodaq_gps.getHour() + 3;
      uint8_t minute = sodaq_gps.getMinute();
      uint8_t second = sodaq_gps.getSecond();

      setLight(blue);
      GeoLocation geoLocation(sodaq_gps.getLat(), sodaq_gps.getLon(), 0);
      payload.reset();
      payload.set("ShockDetected", true);
      payload.set("Shock", geoLocation);
      payload.setTimestamp(unix.get(year, month, day, hour, minute, second));

      modem.send(payload);

      setLight(off);
      delay(1000);
    }
    
    accel.enableAccelerometer();

    deepSleep();
  }
}

void callback(const char* data)
{
  debugSerial.println("**************** IN CALLBACK ********************");
  
  debugSerial.println(data);
}
