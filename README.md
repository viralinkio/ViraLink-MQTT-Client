# ViraLink MQTT Client for Viralink IoT Cloud Platform

[![Actions Status: main](https://github.com/viralinkio/ViraLink-MQTT-Client/workflows/PlatformIO%20CI/badge.svg)](https://registry.platformio.org/libraries/viralinkio/ViraLink-MQTT-Client)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/viralinkio/library/ViraLink-MQTT-Client.svg)](https://registry.platformio.org/libraries/viralinkio/ViraLink-MQTT-Client)
[![arduino-library-badge](https://www.ardu-badge.com/badge/ViraLink-MQTT-Client.svg?)](https://www.ardu-badge.com/ViraLink-MQTT-Client)
[![Chat on Telegram](https://img.shields.io/badge/Chat%20on-Telegram-blue.svg)](https://t.me/+PUASABLY8Qg5Zjhk)

This library helps you to connect to viralink.io server with The most needed examples.

## Features
1. Connect to [console.viralink.ir](https://console.viralink.ir) or any Thingsboard server via MQTT Protocol.
2. Send Telemetry and Attributes to Server
3. OTA Update via MQTT. (Only supports esp8266 and esp32)
4. Implement the queue for queuing the mqtt messages when not connected to server to prevent data lost
5. Implement custom Uptime class instead of millis(). millis() only support 49 days but this implementation support upto 584m year! (uint64_t milliseconds).
6. Configure your network Interfaces and Auto Connect to Interfaces base on priorities, Also you can set Auto Reconnect that will try to reconnect when interface disconnected!
7. MQTTController.loop() can run on other cores (on freeRTOS systems like esp32). 

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
You can install this library via Platformio and/or Arduino IDE. 

### [Platformio](https://platformio.org/)
All dependencies will be installed automatically.

Just add [ViraLink-MQTT-Client](https://registry.platformio.org/libraries/viralinkio/ViraLink-MQTT-Client/installation) to your [platformio.ini](https://docs.platformio.org/en/latest/projectconf/index.html)

This is a example of paltformio.ini for ESP32 DevKit. 

NOTE: This is just an example. 
Update the version of the library to the latest !
```
[env:myenv]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
lib_deps =
     viralinkio/ViraLink-MQTT-Client @ ^1.0.2
```

### [Arduino IDE](https://www.arduino.cc/en/software)
All Required libraries can be installed directly from the Arduino Library manager.

1. From Tools => Manage Libraries...
2. Search for ViraLink-MQTT-Client
3. Select Install
4. Select Install All


## Run Examples
### [Platformio](https://platformio.org/)
If you are using platformio just rename one example .ino file to main.cpp file in your src folder.

Build the project:
```
pio run -e myenv
```
Upload (flash) to the board:
```
pio run -t upload -e myenv
```

### [Arduino IDE](https://www.arduino.cc/en/software)
0. Add [ESP8266](https://arduino-esp8266.readthedocs.io/en/latest/installing.html) / [ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) from your Board manager
1. Install the ViraLink-MQTT-Client from Tools => Manage Libraries...
2. Open One example from File => Examples => ViraLink-MQTT-Client
3. Select Target Board and Port from Tools menu
4. Build and Upload
