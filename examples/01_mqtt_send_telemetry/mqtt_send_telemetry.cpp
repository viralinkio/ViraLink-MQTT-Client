#include <Arduino.h>

#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define WIFI_SSID "xxxxxxx"
#define WIFI_PASSWORD "xxxxxxx"

#define VIRALINK_TOKEN "xxxxxxx"
#define VIRALINK_MQTT_URL "console.viralink.io"
#define VIRALINK_MQTT_PORT 1883

#include "viralink.h"

#ifdef ESP32
#include "WiFi.h"
#elifdef ESP8266
#include "ESP8266WiFi.h"
#endif

MQTTController mqttController;
WiFiClient client;

// store last uptime Milliseconds that we sent telemetry data
uint64_t lastSentTelemetryMs;

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
                sendAttributesData();
            });

}

void loop() {
    mqttController.loop();

    // send telemetry data each 5 second
    if ((Uptime.getMilliseconds() - lastSentTelemetryMs) < 5000) return;
    lastSentTelemetryMs = Uptime.getMilliseconds();

    sendTimeSeriesData();
}