#include <Arduino.h>

#define CURRENT_FIRMWARE_TITLE "OTA_EXAMPLE"
#define CURRENT_FIRMWARE_VERSION "1"

#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define WIFI_SSID "xxxxxxx"
#define WIFI_PASSWORD "xxxxxxx"

#define VIRALINK_TOKEN "xxxxxxx"
#define VIRALINK_MQTT_URL "console.viralink.io"
#define VIRALINK_MQTT_PORT 1883

#include "viralink.h"

#include <esp_task_wdt.h>

#if defined(ESP32)
#include "WiFi.h"
#elif defined(ESP8266)
#include "ESP8266WiFi.h"
#endif

WiFiClient client;
MQTTController mqttController;
// more chunkSize need more RAM so if you have not enough memory try to use smal chunkSize such as 2048 byte
MQTTOTA ota(&mqttController, 10240);

bool on_message(const String &topic, DynamicJsonDocument json) {

    Serial.println("New Message: ");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Data [json]: ");
    Serial.println(json.as<String>());

    return true;
}

void mqttLoop(void *parameter) {
    while (true){
        mqttController.loop();
        delayMicroseconds(1);
    }
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
    // ota.begin should called after mqttController.init()
    ota.begin(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);

    mqttController.connect(client, "esp", VIRALINK_TOKEN, "", VIRALINK_MQTT_URL, VIRALINK_MQTT_PORT, on_message,
                           nullptr, []() {
                ota.checkForUpdate();
                Serial.println("Connected To Platform");
            });

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
}

void loop() {

}