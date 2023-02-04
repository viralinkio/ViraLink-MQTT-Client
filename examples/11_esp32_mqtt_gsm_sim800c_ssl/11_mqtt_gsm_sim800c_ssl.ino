#include <Arduino.h>

#define TINY_GSM_MODEM_SIM800     //GSM MODEL Depends on Each Device
#define SerialAT Serial2          //if you enable GSM feature the SerialAT should be defined
#define TINY_GSM_DEBUG SerialMon

#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define VIRALINK_TOKEN "xxxxxx"
#define VIRALINK_MQTT_URL "console.viralink.io"
#define VIRALINK_MQTT_PORT 8883

#define SIM_APN "apn"

#include <SSLClient.h>
#include "certificates.h" // This file must be auto generated using provided script

#include <TinyGsmClient.h>
#include "viralink.h"

TinyGsm modem(SerialAT);
TinyGsmClient tinyGsmClient(modem);
SSLClient sslClient(client, TAs, (size_t) TAs_NUM, A5);
MQTTController mqttController;

bool connectToNetwork() {

    Serial.println("Initializing modem...");
    if (!modem.restart())
        return false;

    String name = modem.getModemName();
    Serial.println("Modem Name: " + name);

    String modemInfo = modem.getModemInfo();
    Serial.println("Modem Info: " + modemInfo);

    if (!modem.isNetworkConnected()) {
        Serial.println("Waiting for network...");
        if (!modem.waitForNetwork(120000L))
            return false;
    }

    if (modem.isNetworkConnected()) { Serial.println("Network connected"); }

    Serial.println("Connecting to apn");
    if (!modem.gprsConnect(SIM_APN, "", ""))
        return false;

    Serial.println("Connected to GPRS");

    return true;
}

bool on_message(const String &topic, DynamicJsonDocument json) {

    Serial.println("New Message: ");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Data [json]: ");
    Serial.println(json.as<String>());

    return true;
}

void setup() {
    delay(5000);
    Serial.begin(115200);
    SerialAT.begin(115200);

    if (!connectToNetwork()) {
        Serial.println("Could Not Connect to GPRS");
        return;
    }

    mqttController.init();
    mqttController.connect(sslClient, "esp", VIRALINK_TOKEN, "", VIRALINK_MQTT_URL, VIRALINK_MQTT_PORT, on_message,
                           nullptr, []() {
                Serial.println("Connected To Platform");
            });
}

void loop() {
    mqttController.loop();
}