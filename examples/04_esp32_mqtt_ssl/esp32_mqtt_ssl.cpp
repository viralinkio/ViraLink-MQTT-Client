#include <Arduino.h>

#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define WIFI_SSID "xxxxxxx"
#define WIFI_PASSWORD "xxxxxxx"

#define VIRALINK_TOKEN "xxxxxxx"
#define VIRALINK_MQTT_URL "console.viralink.io"
#define VIRALINK_MQTT_PORT 8883

#include "viralink.h"
// this library is avaible on  https://github.com/OPEnSLab-OSU/SSLClient
// tested version : openslab-osu/SSLClient @ ^1.6.11
#include <SSLClient.h>
#include "certificates.h" // This file must be auto generated

#ifdef ESP32

#include "WiFi.h"

#elif defined(ESP8266)

#include "ESP8266WiFi.h"

#endif

WiFiClient client;
// A5 is a random pin that used for generating real random data for encryption
SSLClient sslClient(client, TAs, (size_t) TAs_NUM, A5);
MQTTController mqttController;

bool on_message(const String &topic, DynamicJsonDocument json) {

    Serial.println("New Message: ");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Data [json]: ");
    Serial.println(json.as<String>());

    return true;
}

void setup() {

    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.println();
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);
    }

    Serial.println("Connected to WiFi!");
    Serial.println(WiFi.localIP());

    mqttController.init();
    mqttController.connect(&sslClient, "esp", VIRALINK_TOKEN, "", VIRALINK_MQTT_URL, VIRALINK_MQTT_PORT, on_message,
                           nullptr, []() {
                Serial.println("Connected To Platform");
            });

}

void loop() {
    mqttController.loop();
}