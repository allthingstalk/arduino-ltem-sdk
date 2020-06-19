# AllThingsTalk Arduino LTE-M SDK

<img align="right" width="250" height="92" src="extras/lte-m-logo.png">

The LTE-M SDK for Arduino provides an easy to use set of functions that support LTE-M LPWAN networks for the AllThingsTalk IoT Cloud.  
LTE-M RDK (Rapid Development Kit) files are also contained in this SDK [here](/examples/Rapid%20Development%20Kit).

Here’s a simplest **complete** Arduino sketch that connects to LTE-M Network and sends `Hello World!` to AllThingsTalk Platform:

```cpp
#include <AllThingsTalk_LTEM.h>
APICredentials credentials("api.allthingstalk.io", "Your-Device-Token", "Your-Device-ID");
AllThingsTalk_LTEM att(Serial1, credentials, "Your-APN");
CborPayload payload;
void setup() { att.init(); payload.set("StringSensorAsset", "Hello World!"); att.send(payload); }
void loop() { }
```

That’s how easy it is!  
If you're having issues, [check the troubleshooting section](#troubleshooting-and-notes).

> [AllThingsTalk](https://www.allthingstalk.com) is an accessible IoT Platform for rapid development.  
In the blink of an eye, you'll be able to extract, visualize and use the collected data.  
[Get started and connect up to 10 devices free-of-charge](https://www.allthingstalk.com/maker)


# Table of Contents
<!--ts-->  
* [Installation](#installation)  
  * [Supported Hardware](#supported-hardware)
    * [Modules](#modules)
    * [Development Boards](#development-boards)
* [Connecting](#connecting)
  * [Defining Credentials](#defining-credentials)
    * [Separating Credentials (keys.h)](#separating-credentials)
  * [Maintaining Connection](#maintaining-connection)
  * [Connecting and Disconnecting](#connecting-and-disconnecting)
* [Sending Data](#sending-data)
  * [JSON](#json)
  * [CBOR](#cbor)
* [Receiving Data](#receiving-data)
  * [Actuation Callbacks](#actuation-callbacks)
* [Getting Modem Information](#getting-modem-information)
  * [Getting Firmware Version](#getting-firmware-version)
  * [Getting Firmware Revision](#getting-firmware-revision)
  * [Getting IMEI](#getting-imei)
  * [Getting ICCID](#getting-iccid)
  * [Getting IMSI](#getting-imsi)
* [Debug](#debug)
  * [Enable Debug Output](#enable-debug-output)
  * [Enable Verbose Debug Output](#enable-verbose-debug-output)
  * [Enable AT Command Debug Output](#enable-at-command-debug-output)
* [Troubleshooting and Notes](#troubleshooting-and-notes)
<!--te-->

# Installation

## Supported Hardware

This SDK has been tested and works on the hardware listed below:

### Modules
- [SARA-R410M](https://www.u-blox.com/en/product/sara-r4-series)

### Development Boards
- [Sodaq SARA AFF](https://support.sodaq.com/Boards/Sara_AFF/)
- [Sodaq SARA SFF](https://support.sodaq.com/Boards/Sara_SFF/)

## Installation
This SDK is available on Arduino Library Manager. To install it:

- In Arduino IDE, go to *Tools* > *Manage Libraries*
- Search for and download "**AllThingsTalk LTE-M SDK**" by AllThingsTalk
- Done! The SDK (as well as RDK) examples are downloaded and available as well in *Arduino IDE* > *File* > *Examples* > *AllThingsTalk LTE-M SDK*


# Connecting

The library takes care about initialization and maintaining the network connection and connection towards AllThingsTalk.

## Defining Credentials

At the beginning of your sketch (before `setup()`), make sure to include this library and define your credentials as shown:

```cpp
#include <AllThingsTalk_LTEM.h>
APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID); // Class used to define credentials for connecting to AllThingsTalk
AllThingsTalk_LTEM att(Serial1, credentials, APN);
```

- `SPACE_ENDPOINT` is the [AllThingsTalk space](https://www.allthingstalk.com/spaces) you're going to connect to. If using AllThingsTalk Maker, simply enter `"api.allthingstalk.io"`
- `DEVICE_TOKEN` is your AllThingsTalk Device Token
- `DEVICE_ID` is your AllThingsTalk Device ID
- `Serial1` is the name of the Serial port used for the LTE-M Modem
- `credentials` is the credentials object created above
- `APN` is the APN of your LTE-M Service provider

> To get your **Device ID** and **Device Token**, go to your [AllThingsTalk Maker](https://maker.allthingstalk.com) devices, choose your device and click “*Settings*”, then choose “*Authentication*” and copy "Device ID" and “Device Tokens” values.

> From here and on, this part of the code won’t be shown in the examples below as it’s assumed you already have it.

### Separating Credentials

In case you share your Arduino sketch often (either via GitHub or other means), you should consider creating a "**keys.h**" file that contains your credentials.  
This way, when you share your sketch, your credentials would remain private because you'd only share the Arduino sketch. Another advantage is that whoever downloads your sketch could have their own **keys.h** file, thus the sketch would immediately work on their computer.

First, create a new file in the same directory as your Arduino sketch, name it **keys.h** and copy/paste the following into the file:
```cpp
#ifndef KEYS_H
#define KEYS_H

char* DEVICE_ID      = "Your-Device-ID";
char* DEVICE_TOKEN   = "Your-Device-Token";
char* APN            = "Your-LTE-M-APN";
char* SPACE_ENDPOINT = "Your-Space-Endpoint";

#endif
```

Then, make sure to change the top of your sketch to include **keys.h** and use the variables defined in it:

```cpp
#include <AllThingsTalk_LTEM.h>
#include "keys.h" // Include our newly created file
APICredentials credentials(SPACE_ENDPOINT, DEVICE_TOKEN, DEVICE_ID);
AllThingsTalk_LTEM att(Serial1, credentials, APN);
```
So, now you've included the "**keys.h**" file and changed the credentials to variables that will be loaded from that file. Done.

> If you're going to put your sketch to GitHub, make sure to [gitignore](https://help.github.com/en/articles/ignoring-files) the **keys.h** file

## Maintaining Connection

All you have to do to connect and to maintain a connection is add `init()` method to your `setup()` function and `loop()` method to your `loop()` function:

```cpp
void setup() {
  att.init();  // Initializes AllThingsTalk
}
void loop() {
  att.loop();  // Keeps LTE-M and AllThingsTalk connection alive (if active)
}
```

This will take care of connecting to LTE-M Network and AllThingsTalk.  

## Connecting and Disconnecting

Connection is automatically established once `init()` is executed.  
However, if you wish to disconnect and/or connect from either LTE-M or AllThingsTalk during device operation, you can do that by using these methods **anywhere** in your sketch:

| **Method**                 | **Operation**                                                                 |
| -------------------------- | ----------------------------------------------------------------------------- |
| connect();                 | Connects to **both** LTE-M Network and AllThingsTalk                                   |
| disconnect();              | Disconnects **both** LTE-M Network and AllThingsTalk                                   |

Example:

```cpp
void setup() {
  att.init();       // Initializes AllThingsTalk
}
void loop() {
  att.loop();       // Keeps LTE-M Network and AllThingsTalk connection alive (if active)
  att.disconnect(); // Disconnects from both LTE-M and AllThingsTalk
  delay(5000);         // Wait 5 seconds
  att.connect();    // Connects to both LTE-M and AllThingsTalk
}
```

# Sending Data

You can send data to AllThingsTalk in 3 different ways using the library.  

## JSON

*JavaScript Object Notation*  
[Read about JSON in our Documentation](https://docs.allthingstalk.com/developers/data-formats/#json)  
JSON is a lightweight data-interchange format which is easy for humans to read and write and easy for machines to parse. It’s widely adopted on the web.

> This is the quickest and simplest way of sending data, but uses a tiny bit more bandwidth and battery.

> When using JSON to send data, the message is sent immediately upon execution.  

Use the following method to send a JSON message:

```cpp
att.send("asset_name", value);
```

- `asset_name` is the name of asset on your AllThingsTalk Maker.  
  This argument is of type `char*`, in case you’re defining it as a variable.
- `value` is the data that’ll be sent to the specified asset. It can be of any type.
- `att.send()` returns boolean **true** or **false** depending on if the message went through or not.


## CBOR

*Concise Binary Object Representation*  
[Read more about CBOR in our Documentation](https://docs.allthingstalk.com/developers/data-formats/#cbor)  
CBOR is a data format whose design goals include the possibility of extremely small code size, fairly small message size, and extensibility without the need for version negotiation.  

> This method uses less data. Use it if you’re working with limited data or bandwidth.  

> As opposed to JSON data sending, with CBOR, you can build a payload with multiple messages before sending them.

You’ll need to create a `CborPayload` object before being able to send data using CBOR.  
The beginning of your sketch should therefore contain `CborPayload payload`.

```cpp
#include <AllThingsTalk_LTEM.h>
CborPayload payload;
```

Now you can send a message to your AllThingsTalk Maker whenever you want using:

```cpp
payload.reset();
payload.set("asset_name", value);
payload.set("asset_2_name", value2);
att.send(payload);
```

- `payload.reset()` clears the message queue, so you’re sure what you’re about to send is the only thing that’s going to be sent.  
- `payload.set("asset_name", value)` adds a message to queue. 
	You can add as many messages (payloads) as you like before actually sending them to AllThingsTalk.
    -  `asset_name` is the name of asset on your AllThingsTalk Maker.  
       This argument is of type `char*`, in case you’re defining it as a variable.
    -  `value` is the data you want to send. It can be of any type.
- `att.send(payload)` Sends the payload and returns boolean **true** or **false** depending on if the message went through or not.
    
# Receiving data

## Actuation Callbacks

Actuation Callbacks call your functions once a message arrives from your AllThingsTalk Maker to your device on a specified asset.  
For each *Actuator* asset you have on your AllThingsTalk Maker device, you can add an actuation callback in your `setup()` function (or anywhere else if you wish) by adding `setActuationCallback("Your-Actuator-Asset-Name", YourFunction)`  
To receive data, simply create functions that utilize your desired type of data.  

- Your function argument can be of any type (SDK automatically adjusts).  
  Just make sure to match your function argument type with your *Actuator* asset type on AllThingsTalk Maker. 
- You're able to call `setActuationCallback("asset", YourFunction)` anywhere in your sketch to add a new Actuation Callback during runtime.  
- Returns boolean **true** if it was successful and **false** if it failed.
- You can define up to 32 Actuation Callbacks.

**Example:**

```cpp
...
void setup() {
  SerialUSB.begin(115200);        // Starts serial port on buad rate 115200
  att.debugPort(SerialUSB);       // Tells the library to use the defined serial port
  pinMode(LED_PIN, OUTPUT);    // Initializes pin LED_PIN as output
  att.setActuationCallback("your-asset-1", myActuation1);
  att.setActuationCallback("your-asset-2", myActuation2);
  att.init();               // Initializes AllThingsTalk and network Connection
}

void myActuation1(String data) { // Called when a message from "your-asset-1" is received
  SerialUSB.print("You've received a message from AllThingsTalk: ");
  SerialUSB.println(data);        // Prints the received String data
}

void myActuation2(bool data) {   // Called when a message from "your-asset-2" is received
  if (data) {
    digitalWrite(LED_PIN, HIGH); // Turns on LED
  } else {
    digitalWrite(LED_PIN, LOW);  // Turns off LED
}

void loop() {
  att.loop();  // Keeps AllThingsTalk and network Connection alive
}
```

This means that each time a message arrives from your *Actuator* asset `your-asset-1` from AllThingsTalk Maker, your function `myActuation1` will be called and the message (actual data) will be forwarded to it as an argument.  
In this case, if your device receives a string value `Hello there!` on asset `your-asset-1`, the received message will be printed via Serial and if it receives value `true` on asset `your-asset-2`, the LED will be turned on. (You would change LED_PIN to a real pin on your board).


# Getting Modem Information

Information about the LTE-M Modem and the SIM Card is automatically shown at boot when a network connection is established (if you have [Debug](#debug) enabled).
However, you also have a number of methods at your disposal to get specific information from your Modem, as shown below.

> These methods will only return information if the LTE-M Network is connected due to the limitations of U-Blox LTE-M Modems.

## Getting Firmware Version

To get the Firmware Version of your LTE-M Modem, simply run the following line, which will print the output directly to Serial Monitor if you have [Debug](#debug) enabled.

```cpp
att.getFirmwareVersion();
```

This method also returns the output as `char*`, so you can save the output in a variable in case you don't have Debug enabled.

## Getting Firmware Revision

To get the Firmware Revision of your LTE-M Modem, simply run the following line, which will print the output directly to Serial Monitor if you have [Debug](#debug) enabled.

```cpp
att.getFirmwareRevision();
```

This method also returns the output as `char*`, so you can save the output in a variable in case you don't have Debug enabled.

## Getting IMEI

To get the IMEI of your LTE-M Modem, simply run the following line, which will print the output directly to Serial Monitor if you have [Debug](#debug) enabled.

```cpp
att.getIMEI();
```

This method also returns the output as `char*`, so you can save the output in a variable in case you don't have Debug enabled.

## Gettting ICCID

To get your SIM's [ICCID](https://en.wikipedia.org/wiki/SIM_card#ICCID) (*Integrated Circuit Card Identifier*), simply run the following line, which will print the output directly to Serial Monitor if you have [Debug](#debug) enabled.

```cpp
att.getICCID();
```

This method also returns the output as `char*`, so you can save the output in a variable in case you don't have Debug enabled.

## Getting IMSI

To get your [IMSI](https://en.wikipedia.org/wiki/International_mobile_subscriber_identity) (*International mobile subscriber identity*), simply run the following line, which will print the output directly to Serial Monitor if you have [Debug](#debug) enabled.

```cpp
att.getIMSI();
```

This method also returns the output as `char*`, so you can save the output in a variable in case you don't have Debug enabled.


# Debug

The library outputs useful information such as your network connection details, AllThingsTalk connection details, connection status details and errors, asset creation results, messages going in/out, raw messages, AT commands being sent out to/from the modem and much, much more.

> When choosing a baud rate, if your board supports it, you should go for 115200 or higher, because higher speed baud rates mean less time wasted (by the CPU) outputting messages, therefore not halting your code.

## Enable Debug Output

Output of this information is not enabled by default. To enable it, you need to pass the name of your Serial interface to `debugPort()`  
For example, if you have `SerialUSB.begin(115200)` in your `setup()` function, you'd add `debugPort(SerialUSB)`:

```cpp
void setup() {
  SerialUSB.begin(115200);   // Initialize SerialUSB to 115200 baud rate
  att.debugPort(SerialUSB);  // Now the library knows where to output debug information
  att.init();
}
```

That’s it! You should now see debug information from the library along with your serial output (if you have any).

> When enabling Debug Output, make sure to define it before anything else from this library, so you can see all output from the library.

## Enable Verbose Debug Output

> Enabling Verbose Debug Output can help you significantly when troubleshooting your code.
 
If you wish to see more information, you can use Verbose Debug Output which outputs way more information in addition that shown by Normal Debug Output.

Enable Verbose Debug Output by adding argument `true` to the existing `debugPort(SerialUSB)` method.

Example:

```cpp
void setup() {
  SerialUSB.begin(115200); // Initialize SerialUSB to 115200 baud rate
  att.debugPort(SerialUSB, true); // Verbose Debug Output is now enabled
  att.init();
}
```

## Enable AT Command Debug Output

> Note that it is **required** to have Verbose Debug output enabled to be able to use this feature.

If you wish to see raw AT commands going in and out of the LTE-M modem on you board, you can enable that by adding argument `true` to the existing `debugPort(SerialUSB, true)` method.  
  
Example:
```cpp
void setup() {
  SerialUSB.begin(115200); // Initialize SerialUSB to 115200 baud rate
  att.debugPort(SerialUSB, true, true); // Verbose and AT Command Debug Output is now enabled
  att.init();
}
```

# Troubleshooting and Notes

- To make sure everything is working as intented, make sure you're **at least** these versions of the following software:

    | Name | Version | Used for | Type | Description |
    |--|--|--|--|--|
    | [Arduino IDE](https://www.arduino.cc/en/Main/Software) | 1.8.10 | All | Desktop Software | Main development environment. |
    | [SODAQ SAMD Boards](https://support.sodaq.com/getting_started/#stable) | 1.6.20 | SODAQ Sara | Arduino Board [Core](https://www.arduino.cc/en/Guide/Cores) | Enables Arduino IDE to work with the SODAQ Sara Boards. |

- This SDK relies several different libraries to make it work:

    | Name | Description |
    |--|--|
    | [ArduinoJson](https://arduinojson.org/) (INCLUDED) | Parsing and building JSON payloads to send/receive from AllThingsTalk. |
    | [Sodaq_R4X](https://github.com/SodaqMoja/Sodaq_r4x) (INCLUDED) | Handles low-level communication with LTE-M Modem |
    | [Sodaq_MQTT](https://github.com/SodaqMoja/Sodaq_mqtt) (INCLUDED) | Handles MQTT communication |
    |  [Sodaq_R4X_MQTT](https://github.com/SodaqMoja/Sodaq_r4x_mqtt) (INCLUDED) | "Translator" library between Sodaq_R4X and Sodaq_MQTT |
    |  [Sodaq_WDT](https://github.com/SodaqMoja/Sodaq_wdt) (INCLUDED) | Sodaq's Watchdog library |

- Sending `geoLocation` datatype is supported with [CBOR](#cbor) sending but **not** with [JSON](#json) Sending at the moment.