#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>

#include "keys.h"

#define debugSerial SerialUSB
#define ltemSerial SerialSARA

APICredentials credentials(SPACE_ENDPOINT);
LTEmModem modem(ltemSerial, debugSerial, credentials, true);

CborPayload payload;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial  && millis() < 10000) {}

  if (modem.init()) {
    char* imei = modem.getIMEI();
    debugSerial.println(imei);
    
    if (modem.registerDevice(imei)) {
      debugSerial.println("Registration of device successful");
    }
    else {
      debugSerial.println(modem.getLastError());
    }
  }
  
}

void loop() {
  
}
