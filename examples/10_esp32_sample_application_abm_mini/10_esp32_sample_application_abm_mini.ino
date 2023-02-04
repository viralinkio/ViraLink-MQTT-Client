#include "Arduino.h"

#define FIRMWARE_TITLE "abm_mini_v5.1"
#define FIRMWARE_VERSION "1"

#define AP_WIFI_SSID "ABM_MINI_CONFIG" //wifi ssid for ap mode in config web server
#define AP_WIFI_PASS "1234567890" //wifi pass for ap mode in config web server
#define AP_WIFI_ADDRESS IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)

#define TINY_GSM_RX_BUFFER 512
#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined
#define TINY_GSM_MODEM_SIM800     //GSM MODEL Depends on Each Device
#define SerialAT Serial2          //if you enable GSM feature the SerialAT should be defined
//#define TINY_GSM_DEBUG SerialMon

#define GSM_ENABLE_PIN 26
#define RESET_KEY_PIN 18
#define ACK_LED_PIN 19
#define BUZZER_PIN 23
#define RELAY1_PIN 32
#define RELAY2_PIN 33
#define RELAY3_PIN 25
#define IN1_PIN 34
#define IN2_PIN 39
#define IN3_PIN 36
#define RF_PIN 5
#define SENSOR_PIN 27

#include "viralink.h"
#include <WebServer.h>
#include "WiFi.h"
#include "NetworkController.h"
#include <TinyGsmClient.h>
#include "Preferences.h"
#include "Ticker.h"
#include <esp_task_wdt.h>
#include <Button.h>

Ticker restartTicker;

//NetworkInterface(name, id, priority = 1, timeout = 30000)
NetworkInterface wifiInterface("wifi", 1, 2, 10000);
NetworkInterface modemInterface("modem", 2, 3);
NetworkInterfacesController networkController;

Preferences preferences;
Button resetButton(RESET_KEY_PIN, INPUT_PULLDOWN);
MQTTController mqttController(1024);
MQTTOTA ota(&mqttController, 10240);

WebServer server;
bool onWebserver = false;
WiFiClient wifiClient;
TinyGsm modem(SerialAT);
TinyGsmClient tinyGsmClient(modem);

const char myHtml[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>

<h2>Device Config Page<h2>
<h3> Enter Your Config And Press Submit Button</h3>

<form action="/config">
  <br>
  WiFi SSID:<br>
  <input type="text" name="wSSID" required>
  <br>
  WiFi Password:<br>
  <input type="text" name="wPASS" required>
  <br>
  GSM APN:<br>
  <input type="text" name="gAPN" required>
  <br>
  SIM PIn:<br>
  <input type="text" name="gPIN">
  <br>
  Device Token:<br>
  <input type="text" name="dTOKEN" required>
  <br>
  Admin Phone Number:<br>
  <input type="text" name="phone" required>
  <br>
  <br>
  Connection Types:<br>
  <input type="checkbox" id="wifi" name="wifi" value="true">
  <label for="wifi"> WiFi Connection</label><br>
  <input type="checkbox" id="gsm" name="gsm" value="true">
  <label for="gsm"> GSM Connection</label><br><br>
  <br><br>
  <input type="submit" value="Submit">
</form>
</body></html>)=====";

void resetESP() {
    ESP.restart();
}

void mqttLoop(void *parameter) {
    while (true) {
        if (networkController.getCurrentNetworkInterface() != nullptr &&
            networkController.getCurrentNetworkInterface()->getId() != modemInterface.getId())
            mqttController.loop();
        delayMicroseconds(1);
    }
}

bool on_message(const String &topic, DynamicJsonDocument json) {

    Serial.println("Topic1: ");
    Serial.println(topic);
    Serial.println("Message1: ");
    Serial.println(json.as<String>());

    if (json.containsKey("method")) {

        String methodName = json["method"].as<String>();
        String params = json["params"].as<String>();

        if (methodName.equalsIgnoreCase("restart_device")) {
            float seconds = json["params"]["seconds"];
            if (seconds == 0) seconds = 1;
            Serial.println("Device Will Restart in " + String(seconds) + "Seconds");
            restartTicker.once(seconds, resetESP);
        }
    }

    return true;
}

void connectToNetwork() {
    if (preferences.getBool("wifi")) {
        Serial.println("Added WiFi Interface");
        networkController.addNetworkInterface(&wifiInterface);
    }

    if (preferences.getBool("gsm")) {
        Serial.println("Added GSM Interface");
        networkController.addNetworkInterface(&modemInterface);
    }

    networkController.setAutoReconnect(true, 10000);
    networkController.autoConnectToNetwork();
}

void connectToPlatform(Client &client) {

//    delete sslClient;
//    sslClient = new SSLClient(client, TAs, (size_t) TAs_NUM, A5);
    Serial.println("Trying to Connect Platform");
    mqttController.connect(client, "esp", preferences.getString("token"), "", "console.viralink.io", 1883,
                           on_message,
                           nullptr, []() {
                Serial.println("Connected To Platform");

                ota.checkForUpdate();

                DynamicJsonDocument data(100);
                data["Phone"] = preferences.getString("phone");
                mqttController.sendAttributes(data);
            });
}

void initInterfaces() {
    wifiInterface.setConnectInterface([]() -> bool {
        Serial.print("Connecting To WiFi ");
        WiFi.mode(WIFI_STA);
        WiFi.begin(preferences.getString("ssid").c_str(), preferences.getString("pass").c_str());
        return true;
    });
    wifiInterface.setConnectionCheckInterfaceInterface([]() -> bool {
        return WiFi.status() == WL_CONNECTED;
    });
    wifiInterface.OnConnectingEvent([]() {
        Serial.print(".");
    }, 500);
    wifiInterface.OnConnectedEvent([]() {
        Serial.println(String("Connected to WIFI with IP: ") + WiFi.localIP().toString());
        ota.startHandleOTAMessages();
        connectToPlatform(wifiClient);
        DynamicJsonDocument data(200);
        data["Connection Type"] = "WIFI";
        data["IP"] = WiFi.localIP().toString();
        data.shrinkToFit();
        mqttController.sendAttributes(data);
    });

    // no need to timeout because the function only Trying to connect only ones;
    modemInterface.setTimeoutMs(0);
    modemInterface.setConnectInterface([]() -> bool {

        digitalWrite(GSM_ENABLE_PIN, HIGH);
        delay(1000);
        digitalWrite(GSM_ENABLE_PIN, LOW);

        Serial.println("Initializing modem...");
        if (!modem.restart())
            return false;

        String name = modem.getModemName();
        Serial.println("Modem Name: " + name);

        String modemInfo = modem.getModemInfo();
        Serial.println("Modem Info: " + modemInfo);

        String simPin = preferences.getString("pin");
        if (!simPin.isEmpty() && modem.getSimStatus() != 3) { modem.simUnlock(simPin.c_str()); }

        if (!modem.isNetworkConnected()) {
            Serial.println("Waiting for network...");
            if (!modem.waitForNetwork(30000L))
                return false;
        }

        if (modem.isNetworkConnected()) { Serial.println("Network connected"); }
        Serial.println(modem.getOperator());

        Serial.println("Connecting to " + preferences.getString("apn"));
        if (!modem.gprsConnect(preferences.getString("apn").c_str(), "", "")) {
            Serial.println("failed");
            return false;
        }

        Serial.println("Connected to GPRS");

        return true;
    });
    modemInterface.setConnectionCheckInterfaceInterface([]() -> bool {
        return modem.isGprsConnected();
    });
    modemInterface.OnConnectedEvent([]() {
        Serial.println(String("Connected to GSM with IP: ") + modem.localIP().toString());
        ota.stopHandleOTAMessages();
        connectToPlatform(tinyGsmClient);

        DynamicJsonDocument data(400);
        data["Connection Type"] = "GSM";
        data["IP"] = modem.localIP().toString();
        data["Operator"] = modem.getOperator();
        data["IMEI"] = modem.getIMEI();
        data["Signal Quality"] = modem.getSignalQuality();
        data["ModemInfo"] = modem.getModemInfo();
        data.shrinkToFit();
        mqttController.sendAttributes(data);
    });
}

void setup() {

    SerialMon.begin(115200);
    Serial.println();

    Serial.println("Hello from [" + String(FIRMWARE_TITLE) + "]:[" + String(FIRMWARE_VERSION) + "]");

    resetButton.init();
    preferences.begin("abm_mini", false);

    // detect 3s long pressed on button to activate factory reset
    resetButton.onLongClick([]() {
        printDBGln("Factory Reset Function Called");
        preferences.clear();
        delay(1000); // this is necessary before restart otherwise it will run again after reboot
        ESP.restart();
    }, 3000);

    // open web config page to config device for first time or after the factory reset
    if (!preferences.isKey("configured")) {
        Serial.println("Config Page Started");

        WiFi.softAPConfig(AP_WIFI_ADDRESS);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASS, 1, 0, 10);

        server.on("/", [=]() {
            server.send(200, "text/html", myHtml); //Send web page
        });

        //form action is handled here
        server.on("/config", [=]() {
            preferences.clear();
            preferences.putString("configured", "true");
            preferences.putString("ssid", server.arg("wSSID"));
            preferences.putString("pass", server.arg("wPASS"));
            preferences.putString("apn", server.arg("gAPN"));
            preferences.putString("pin", server.arg("gPIN"));
            preferences.putString("token", server.arg("dTOKEN"));
            preferences.putString("phone", server.arg("phone"));
            preferences.putBool("wifi", !server.arg("wifi").isEmpty());
            preferences.putBool("gsm", !server.arg("gsm").isEmpty());
            preferences.end();
            restartTicker.once(2, resetESP);

            server.send(200, "text/html",
                        "Your Configuration Saved And Device Will Restart in 2 second"); //Send web page
        });
        server.begin();
        onWebserver = true;

        return;
    }

    SerialAT.begin(115200);

    mqttController.init();
    mqttController.sendSystemAttributes(true);
    ota.begin(FIRMWARE_TITLE, FIRMWARE_VERSION);

    delay(1000);
    disableCore0WDT();
    xTaskCreatePinnedToCore(
            mqttLoop, // Function to implement the task
            "MQTT_Loop", // Name of the task
            10000, // Stack size in words
            NULL,  // Task input parameter
            0, // Priority of the task
            NULL,  // Task handle.
            0); // Core where the task should run

    pinMode(GSM_ENABLE_PIN, OUTPUT);
    initInterfaces();
    connectToNetwork();
}

void loop() {
    if (networkController.getCurrentNetworkInterface() != nullptr &&
        networkController.getCurrentNetworkInterface()->getId() == modemInterface.getId() &&
        modemInterface.lastConnectionStatus())
        mqttController.loop();

    if (onWebserver) {
        server.handleClient();
        return;
    }

    resetButton.loop();
    networkController.loop();
}