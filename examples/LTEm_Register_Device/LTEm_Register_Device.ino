#include <APICredentials.h>
#include <CborPayload.h>
#include <LTEmModem.h>

#include "keys.h"

#define debugSerial SerialUSB
#define ltemSerial SerialSARA

APICredentials credentials(SPACE_ENDPOINT);
LTEmModem modem(ltemSerial, debugSerial, credentials, false);

CborPayload payload;

void setup() {
  debugSerial.begin(115200);
  while (!debugSerial  && millis() < 10000) {}

  if (modem.init()) {
    if (modem.registerDevice(modem.getIMEI(), PARTNER_ID)) {
      debugSerial.println("Registration of device successful");

      payload.set("temp", 22.3);
      if (modem.send(payload)) {
        debugSerial.println("Sending payload succeeded");
      }
      else {
        debugSerial.println("Sending payload failed");
      }
    }
    else {
      debugSerial.println(modem.getLastError());
    }
  }
  else {
    debugSerial.println(modem.getLastError());
  }
}

void loop() {
  
}
