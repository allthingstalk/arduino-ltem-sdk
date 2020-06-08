# AllThingsTalk Arduino LTE-M SDK

<img align="right" width="250" height="92" src="extras/lte-m-logo.png">

The LTE-M SDK for Arduino provides an easy to use set of functions that support LTE-M LPWAN networks for the AllThingsTalk IoT Cloud.  
LTE-M RDK (Rapid Development Kit) files are also contained in this SDK [here](/examples/Rapid%20Development%20Kit).

## Supported Hardware

This SDK has been tested and works on the hardware listed below.

### Modules
- [SARA-R410M](https://www.u-blox.com/en/product/sara-r4-series)

### Development Boards
- [Sodaq AFF](https://support.sodaq.com/Boards/Sara_AFF/)
- [Sodaq SFF](https://support.sodaq.com/Boards/Sara_SFF/)

## Installation
This SDK is available on Arduino Library Manager. To install it:

- In Arduino IDE, go to *Tools* > *Manage Libraries*
- Search for and download "**AllThingsTalk LTE-M SDK**" by AllThingsTalk
- Done! The SDK (as well as RDK) examples are downloaded and available as well in *Arduino IDE* > *File* > *Examples* > *AllThingsTalk LTE-M SDK*


## How to use

### Credentials

**API Credentials**
The API Credentials are used to connect to our cloud platform (https://maker.allthingstalk.com/)
This class exists of one constructor with following parameters: space, device token and device id.

Space, Device Token and Device ID
* Space: Sets your space endpoint, like 'api.allthingstalk.io'.
* Device Token: device authentication & authorization bearer token. Your can find your Device Token in the AllThingsTalk Cloud under your device -> settings -> authentication.
* Device ID:  Unique ID for your device in the AllThingsTalk Cloud. Your can find your Device ID in the AllThingsTalk Cloud under your device -> settings -> authentication.

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

payload.reset();
payload.set(17.56);
payload.set(true);
payload.set("hello");

modem.send(payload);
```
OR

```
BinaryPayload payload01("Hello");
BinaryPayload payload02("from ");

modem.send(payload01);
delay(5000);
modem.send(payload02);

```

**Json Payload**
```
JsonPayload payload;

payload.reset();
payload.set("counter", "{\"value\": 1}");
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
