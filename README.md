# Arduino-SDK for viralink.io

![Actions Status: main](https://github.com/viralinkio/ViraLink-MQTT-Client/workflows/PlatformIO%20CI/badge.svg)

This library helps you to connect viralink.io server with The most used examples and needed.

## Features
1. Connect to [viralink.io](https://viralink.io) or any Thingsboard server via MQTT Protocol.
2. Send Telemetry and Attributes to Server
3. OTA Update via MQTT. (Only supports esp8266 and esp32)
4. Implement the queue for queuing the mqtt messages when not connected to server to prevent data lost
5. Implement custom Uptime class instead of millis(). millis() only support 49 days but this implementation support upto 584m year! (uint64_t milliseconds).
6. Configure your network Interfaces and Auto Connect to Interfaces base on priorities, Also you can set Auto Reconnect that will try to reconnect when interface disconnected!
7. MQTTController.loop() can run other cores (on freeRTOS systems like esp32). 

## Supported Hardware
  * ESP8266
  * ESP32 

## Requirements

This library and its examples are dependent on the following libraries
 - [MQTT PubSub Client](https://github.com/knolleary/pubsubclient) — for interacting with MQTT.
 - [ArduinoJSON](https://github.com/bblanchon/ArduinoJson) — for dealing with JSON files.
 - [TinyGSM](https://github.com/vshymanskyy/TinyGSM) — for connecting via GSM network. Only Used in examples and it is Optional.
 - [SSLClient](https://github.com/OPEnSLab-OSU/SSLClient) — for connecting to MQTT client via ssl. Only used in examples and it is Optional. Not Working ON ESP8266!.

## Installation
You can install via Platformio and Arduino IDE. 

### [Platformio](https://platformio.org/)
If you are using platformio all dependencies are installed automatically. Just add [viralink-arduino-sdk](https://platformio.org/lib/show/12005/Viralink_Arduino_SDK/installation) to your [platformio.ini](https://docs.platformio.org/en/latest/projectconf/index.html)
The example for paltformio.ini for esp32 board. 
NOTE: This is example. Update the version of sdk library
```
[env:myenv]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
lib_deps =
     viralink/Viralink_Arduino_SDK @ ^0.0.47
```

### [Arduino IDE](https://www.arduino.cc/en/software)
All Requirement libraries can be installed directly from the Arduino Library manager.
Install [Requirements](#Requirements) via Arduino Library manager.

## Run Examples
### [Platformio](https://platformio.org/)
If you are using platformio just add .cpp file to your src folder.
Build project via command:
```
pio run -e myenv
```
Upload (flash) the board via command
```
pio run -t upload -e myenv
```

### [Arduino IDE](https://www.arduino.cc/en/software)
1. First Install All Requirement Libraries
2. Install the Viralink-Arduino-SDK from LibraryManager
3. Open One example
4. Select Target Board and Port
5. Build and Upload from Arduino IDE
