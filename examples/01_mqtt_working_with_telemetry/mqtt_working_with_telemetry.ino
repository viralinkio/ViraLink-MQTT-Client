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

// store last uptime Milliseconds that we sent telemetry data
uint64_t lastSentTelemetryMs;

// store last uptime Milliseconds that we request attributes values
uint64_t lastRequestedAttributesMs;

bool on_message(const String &topic, DynamicJsonDocument json) {

    Serial.println("New Message: ");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Data [json]: ");
    Serial.println(json.as<String>());

    return true;
}

void sendTimeSeriesData() {
    Serial.println("Sending TimeSeries Data");
    DynamicJsonDocument data(100);
    data["temperature"] = random(0, 100);
    mqttController.sendTelemetry(data.as<String>());

}

void sendAttributesData() {
    DynamicJsonDocument data(100);
    data["OS_Version"] = random(0, 5);
    mqttController.sendAttributes(data.as<String>());
}

void requestAttributesValues() {
    Serial.println("Request Shared and Client Side Attributes");
    DynamicJsonDocument data(100);
    data["sharedKeys"] = "updateInterval,OS_VERSION";
    data["clientKeys"] = "uptime";
    mqttController.requestAttributesJson(data.as<String>(), [](String topic, DynamicJsonDocument json) -> bool {
        Serial.println(json.as<String>());

        // if you return true means the callbacks handled successfully
        // if you return false the message will come through default callback again (on_message function)
        return true;
    });
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
    // sendSystemAttributes send some basic info to the platform like upTime, Free Heap
    mqttController.sendSystemAttributes(true);
    mqttController.connect(client, "esp", VIRALINK_TOKEN, "", VIRALINK_MQTT_URL, VIRALINK_MQTT_PORT, on_message,
                           nullptr, []() {
                Serial.println("Connected To Platform");
            });

    // this function add message to queue. After the MQTT connection occur the message will send automatically
    sendAttributesData();
}

void loop() {
    mqttController.loop();

    // send telemetry data each 5 second
    if ((Uptime.getMilliseconds() - lastSentTelemetryMs) > 5000) {
        lastSentTelemetryMs = Uptime.getMilliseconds();
        sendTimeSeriesData();
    }

    // request Attributes Value each 10 second
    if ((Uptime.getMilliseconds() - lastRequestedAttributesMs) > 10000) {
        lastRequestedAttributesMs = Uptime.getMilliseconds();
        requestAttributesValues();
    }

}