# arduino-LTE-m-sdk

This is a SDK by AllThingsTalk that provides connectivity to their cloud through [LTE-m radios](https://en.wikipedia.org/wiki/LTE-M).

### Version v1.0

What's new?

* Support Binary payloads
* Support CBOR payloads
* Support Actuation

## Hardware

This SDK has been tested to work with the following hardware

Chip
- SARA-R410M (https://www.u-blox.com/en/product/sara-r4-series)

Board
- Sodaq AFF
- Sodaq SFF

## Installation

Download the library and import the .zip file directly using the Arduino IDE. (Sketch -> Include library -> Add .ZIP library)

## How to use

### Credentials
New classes are introduced, like the credentials classes.  

**API Credentials**
The API Credentials are used to connect to our cloud platform (https://maker.allthingstalk.com/)
This class exists of one constructor with following parameters: space, device token and device id.

*space*
This parameter is to connect to our platform via api, like 'api.allthingstalk.io'.

*device token*
Every device in your private ground has a device token and device id.
Your device token that you can find under settings -> authentication of your device

*device id*
You can also find this id where you can find your device token

Example:
```
APICredentials credentials(SPACE, DEVICE_TOKEN, DEVICE_ID);
```

### Modems

**LTE-m Modem**
The LTE-m class has one constructor with parameters like Hardware Serial, Software Serial, APICredentials (see _Credentials_), fulldebug and callback
Modem class is also used for sending the payload.

Example constructor:
```
void callback(const char* data);

LTEmModem modem(ltemSerial, debugSerial, credentials, true, callback);
```

### Payload

**CBOR Payload**
This class is used to build up your payload for sending to the platform.
For more information about CBOR see https://cbor.io/

```
CBORPayload payload;

payload.set("Battery", 39);
payload.set("DoorOpen", true);
payload.set("Temp", 24.5);
```

Like mentioned before, you use the modem class to send the payload
```
modem.send(payload);
```

*CBOR payload also supports tag 103 and 120*

Tag 103 is automatically used when you add a Geolocation object (example GPSCBOR)
```
GeoLocation geoLocation(51.4546534, 4.127432, 11.2);
    
payload.set("loc", geoLocation);

modem.send(payload);
```

Tag 120 is used when you set the timestamp (example iotDataPoint)
```
GeoLocation geoLocation(51.4546534, 4.127432, 11.2);
    
payload.set("bat", 98);
payload.set("title", "Hello Universe");
payload.set("loc", geoLocation);
payload.set("doorClosed", true);
payload.setTimestamp(138914018);

modem.send(payload);
```

**Binary Payload**
Like in CBOR, the set functionality is almost the same, except you have to translate the incoming data your self via ABCL.  Examples given.
```
BinaryPayload payload;

payload.set(17.56);
payload.set(true);
payload.set("hello");

modem.send(&payload);
```
OR

```
BinaryPayload payload01("Hello");
BinaryPayload payload02("from ");

modem.send(payload01);
delay(5000);
modem.send(payload02);

```

## Actuation
You can also have actuation support in your sketch, the only thing you have to do is add following lines of code:
```
void callback(const char* data);

APICredentials credentials(SPACE, DEVICE_TOKEN, DEVICE_ID);
LTEmModem modem(ltemSerial, debugSerial, credentials, true, callback);

void callback(const char* data)
{
}
```

In the callback method you receive the json string that is sent from the backend.

**Parameters**
data: json string 


# License
Apache 2.0

# Contributions
Pull requests and new issues are welcome.

## How to Install
Download this library as .zip.
In Arduino IDE goto Sketch ==> Include Library ==> Add .ZIP Library.

This will import the new sdk and examples.

> Note: to avoid conflicts, please remove or backup the old SDK
