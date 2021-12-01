#include <Arduino.h>

#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define WIFI_SSID "xxxxxxx"
#define WIFI_PASSWORD "xxxxxxx"

#define VIRALINK_TOKEN "xxxxxxx"
#define VIRALINK_MQTT_URL "console.viralink.io"
#define VIRALINK_MQTT_PORT 1883

#include "viralink.h"

#if defined(ESP32)
#include "WiFi.h"
#elif defined(ESP8266)
#include "ESP8266WiFi.h"
#endif

MQTTController mqttController;
WiFiClient client;

bool on_message(const String &topic, DynamicJsonDocument json) {

    if (json.containsKey("method")) {
        String methodName = json["method"];
        Serial.print("MethodName: ");
        Serial.println(methodName);

        if (json.containsKey("params")) {
            String params = json["params"];
            Serial.print("Params: ");
            Serial.println(params);
        }

        if (methodName.equals("setLEDStatus"))
            digitalWrite(LED_BUILTIN, json["params"]["enabled"]);

        if (methodName.equals("getLEDStatus")) {
            String responseTopic = String(topic);
            responseTopic.replace("request", "response");
            DynamicJsonDocument responsePayload(100);
            responsePayload[String(LED_BUILTIN)] = digitalRead(LED_BUILTIN) ? "ON" : "OFF";
            mqttController.addToPublishQueue(responseTopic, responsePayload.as<String>());
        }

    }
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
    mqttController.connect(client, "esp", VIRALINK_TOKEN, "", VIRALINK_MQTT_URL, VIRALINK_MQTT_PORT, on_message,
                           nullptr, []() {
                Serial.println("Connected To Platform");
            });

}

void loop() {
    mqttController.loop();
}