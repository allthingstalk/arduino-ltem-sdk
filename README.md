# lte-m sdk
## What's new
The design of the sdk is completely changed to make it more simple and more logic.  
So what's new, well almost everything, but not to panic, we will deliver examples to make the transition as easy as possible

### Credentials
New classes are introduced, like the credentials classes.  

**API Credentials**
The API Credential class exists of one constructor with following parameters: space, device token and device id

*space*
This parameter is to set-up your space, like 'api.allthingstalk.io'.

*device token*
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
This class is almost the same as the previous version, except you have now one function to update assets.
```
CBORPayload payload;

payload.map(3);
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
    
payload.map(1);
payload.set("loc", geoLocation);

modem.send(payload);
```

Tag 120 is used when you set the timestamp (example iotDataPoint)
```
GeoLocation geoLocation(51.4546534, 4.127432, 11.2);
    
payload.map(4);
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


## How to Install
Download this library as .zip.
In Arduino IDE goto Sketch ==> Include Library ==> Add .ZIP Library.

This will import the new sdk and examples.

> Note: to avoid conflicts, please remove or backup the old SDK
