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

    Serial.println("New Message: ");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Data [json]: ");
    Serial.println(json.as<String>());

    return true;
}

void setup() {

    Serial.begin(115200);
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
    mqttController.connect(&client, "esp", VIRALINK_TOKEN, "", VIRALINK_MQTT_URL, VIRALINK_MQTT_PORT, on_message,
                           nullptr, []() {
                Serial.println("Connected To Platform");
            });

    char *letters = "abcdefghijklmnopqrstuvwxyz0123456789!";
    String key = "";
    for (int i = 0; i < 10; i++) {
        uint8_t randomValue = random(0, 36);
        key.concat(letters[randomValue]);
    }

    // this function add claim message to queue. After the MQTT connection occur the message will send automatically
    if (mqttController.sendClaimRequest(key, 6000))
        Serial.println("Claiming Request Key is Sent. Key: " + key);
}

void loop() {
    mqttController.loop();

}