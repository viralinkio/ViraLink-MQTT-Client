#ifndef VIRALINK_MQTT_CONTROLLER_TPP
#define VIRALINK_MQTT_CONTROLLER_TPP

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "MQTTMessage.tpp"

#include "map"

#include <utility>
#include "viralink.h"

class MQTTController {
public:

    typedef bool (*DefaultMqttCallbackJsonPayload)(const String &topic, JsonDocument json);

    typedef bool (*DefaultMqttCallbackRawPayload)(const String &topic, uint8_t *payload, unsigned int length);

    typedef std::function<bool(String topic, JsonDocument json)> MqttCallbackJsonPayload;

    typedef std::function<bool(const String &topic, uint8_t *payload, unsigned int length)> MqttCallbackRawPayload;

    typedef std::function<void(MQTTMessage mqttMessage)> SentMQTTMessageCallback;

    typedef void (*ConnectionEvent)();

    MQTTController(int queueSize = 512);

    void updateSendSystemAttributesInterval(float seconds);

    bool isConnected();

    void connect(Client &client, String id, String username, String pass, String url, uint16_t port,
                 DefaultMqttCallbackJsonPayload callback, DefaultMqttCallbackRawPayload callbackRaw = nullptr,
                 ConnectionEvent connectionEvent = nullptr);

    void init();

    void loop();

    bool setBufferSize(uint16_t chunkSize);

    bool addToPublishQueue(const String &topic, const String &payload);

    bool requestAttributesJson(const String &keysJson, const MqttCallbackJsonPayload &callback = nullptr);

    bool sendAttributes(JsonDocument json, const String &deviceName = "");

    bool sendAttributes(const String &json);

    bool sendTelemetry(JsonDocument json, const String &deviceName = "");

    bool sendTelemetry(const String &json);

    bool sendClaimRequest(const String &key, uint32_t duration_ms, const String &deviceName = "");

    bool sendGatewayConnectEvent(const String &deviceName);

    bool sendGatewayDisConnectEvent(const String &deviceName);

    bool subscribeToGatewayEvent();

    bool unsubscribeToGatewayEvent();

    void registerCallbackRawPayload(const MqttCallbackRawPayload &callback);

    void registerCallbackJsonPayload(const MqttCallbackJsonPayload &callback);

    void onSentMQTTMessageCallback(const SentMQTTMessageCallback &callback);

    void setTimeout(int timeout_ms);

    void resetTimeout();

    void resetBufferSize();

    void sendSystemAttributes(bool value);

    uint16_t getQueueSize();

private:
    Queue<MQTTMessage> *queue;
#ifdef INC_FREERTOS_H
    SemaphoreHandle_t semaQueue;
#endif
    PubSubClient mqttClient;
    float updateInterval = 10;
    uint64_t lastSendAttributes;
    uint64_t connectionRecheckTimeout;
    bool isSendAttributes = false;
    String id, username, pass, url;
    uint16_t port;
    ConnectionEvent connectedEvent;
    DefaultMqttCallbackJsonPayload defaultCallback;
    DefaultMqttCallbackRawPayload defaultCallbackRaw;
    std::vector <MqttCallbackRawPayload> registeredCallbacksRaw;
    std::vector <MqttCallbackJsonPayload> registeredCallbacksJson;
    std::map<unsigned int, MqttCallbackJsonPayload> requestsCallbacksJson;
    SentMQTTMessageCallback sentMqttMessageCallback;
    uint16_t defaultTimeout, defaultBufferSize, jsonSerializeBuffer;
    uint32_t timeout, attrRequestId;

    String getChipInfo();

    void sendAttributesFunc();

    void on_message(const char *topic, uint8_t *payload, unsigned int length);

};

void MQTTController::registerCallbackRawPayload(const MqttCallbackRawPayload &callback) {
    registeredCallbacksRaw.push_back(callback);
}

void MQTTController::registerCallbackJsonPayload(const MQTTController::MqttCallbackJsonPayload &callback) {
    registeredCallbacksJson.push_back(callback);
}

void MQTTController::on_message(const char *tp, uint8_t *payload, unsigned int length) {
    String topic = String(tp);

    printDBG("On message: ");
    printDBGln(topic);
    printDBG("Length: ");
    printDBGln(String(length));


    for (MqttCallbackRawPayload callback: registeredCallbacksRaw)
        if (callback(tp, payload, length))
            return;

    char json[length + 1];
    strncpy(json, (char *) payload, length);
    json[length] = '\0';

    printDBG("Topic: ");
    printDBGln(tp);
    printDBG("Message: ");
    printDBGln(json);

    // Decode JSON request
    JsonDocument data;
    auto error = deserializeJson(data, (char *) json);
    if (error) {
        printDBG("deserializeJson() failed with code ");
        printDBGln(error.c_str());
        if (defaultCallbackRaw != nullptr) defaultCallbackRaw(topic, payload, length);
        return;
    }

    if (topic.indexOf("v1/devices/me/attributes/response/") == 0) {
        unsigned int topicId = topic.substring(34).toInt();
        auto it = requestsCallbacksJson.begin();
        for (int i = 0; i < requestsCallbacksJson.size(); i++) {
            if (it->first == topicId) {
                if (it->second(topic, data)) {
                    requestsCallbacksJson.erase(topicId);
                    return;
                } else break;
            }
            it++;
        }
    }

    for (MqttCallbackJsonPayload callback: registeredCallbacksJson)
        if (callback(topic, data))
            return;

    if (defaultCallback != nullptr) defaultCallback(topic, data);
    if (defaultCallbackRaw != nullptr) defaultCallbackRaw(tp, payload, length);
}

bool MQTTController::isConnected() {
    return mqttClient.connected();
}

void
MQTTController::connect(Client &client, String Id, String username_in, String password, String url_in, uint16_t port_in,
                        MQTTController::DefaultMqttCallbackJsonPayload callback,
                        MQTTController::DefaultMqttCallbackRawPayload callbackRaw,
                        MQTTController::ConnectionEvent connectionEvent) {

    this->id = Id;
    this->username = username_in;
    this->pass = password;
    this->url = url_in;
    this->port = port_in;

    connectedEvent = connectionEvent;
    this->defaultCallback = callback;
    this->defaultCallbackRaw = callbackRaw;

    this->mqttClient.setClient(client);
    mqttClient.setBufferSize(2048);
    mqttClient.setServer(url.c_str(), port);
    mqttClient.setCallback([&](const char *tp, uint8_t *payload, unsigned int length) {
        on_message(tp, payload, length);
    });

}


void MQTTController::init() {
#ifdef INC_FREERTOS_H
    semaQueue = xSemaphoreCreateBinary();
    if (semaQueue == NULL) {
        printDBGln("Could Not create Semaphore Queue");
        return;
    }
    xSemaphoreGive(semaQueue);
#endif
}

void MQTTController::loop() {
    if (url.isEmpty() || username.isEmpty()) return;

    uint64_t millis = Uptime.getMilliseconds();
    mqttClient.loop();

#ifdef INC_FREERTOS_H
    if (xSemaphoreTake(semaQueue, portMAX_DELAY)) {
#endif
    if (queue != nullptr && !queue->isEmpty() && isConnected()) {
        MQTTMessage message = queue->peek();
        if (mqttClient.publish(message.getTopic().c_str(), message.getPayload().c_str())) {
            if (sentMqttMessageCallback != nullptr) sentMqttMessageCallback(message);
            queue->removeLastPeek();
        } else {
            message.incrementRetry();
            if (message.getRetry() >= 5) {
                printDBGln("Could Not Publish");
                printDBGln(message.getTopic());
                printDBGln(message.getPayload());
                queue->removeLastPeek();
            }
        }
    }
#ifdef INC_FREERTOS_H
    xSemaphoreGive(semaQueue);
}
#endif

    if (isSendAttributes && ((millis - lastSendAttributes) > ((uint64_t)(updateInterval * 1000)))) {
        lastSendAttributes = millis;
        sendAttributesFunc();
    }

    if ((millis - connectionRecheckTimeout) <= timeout) return;
    connectionRecheckTimeout = millis;

    if (!isConnected()) {
        //todo: fixbug: not reConnect to cloud after invalid token
        printDBG(String("Connecting to MQTT server... "));
        if (mqttClient.connect(id.c_str(), username.c_str(), pass.c_str())) {
            printDBGln("[Connected]");
            if (isSendAttributes)
                addToPublishQueue(V1_Attributes_TOPIC, getChipInfo());

            mqttClient.subscribe("v1/devices/me/rpc/request/+");
            mqttClient.subscribe("v1/devices/me/attributes/response/+");
            mqttClient.subscribe(V1_Attributes_TOPIC);
            mqttClient.subscribe("v2/fw/response/+");

            if (connectedEvent != nullptr) connectedEvent();

        } else {
            printDBG("[FAILED] [ rc = ");
            printDBG(String(mqttClient.state()));
            printDBGln(String(" : retrying in " + String(timeout / 1000) + " seconds]"));
        }
    }
}

#ifdef ESP32
#ifdef __cplusplus
extern "C" {
#endif

uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

uint8_t temprature_sens_read();

#endif

String MQTTController::getChipInfo() {
    JsonDocument data;
    data[String("Cpu FreqMHZ")] = ESP.getCpuFreqMHz();
    data[String("Viralink SDK Version")] = SDK_VERSION;
#ifdef ESP32
    data[String("Chip Type")] = "ESP32";
#endif

#ifdef ESP8266
    data[String("Chip Type")] = "ESP8266";
    data[String("Chip ID")] = ESP.getChipId();
#endif

    return data.as<String>();;
}

void MQTTController::sendAttributesFunc() {
    if (!isConnected())
        return;

    JsonDocument data;
    data[String("upTime")] = Uptime.getSeconds();
    data[String("ESP Free Heap")] = ESP.getFreeHeap();

#ifdef ESP32
    data[String("ESP32 temperature")] = (temprature_sens_read() - 32) / 1.8;
#endif


    addToPublishQueue(V1_Attributes_TOPIC, data.as<String>());
}

void MQTTController::updateSendSystemAttributesInterval(float seconds) {
    updateInterval = seconds;
}

bool MQTTController::setBufferSize(uint16_t chunkSize) {
    return mqttClient.setBufferSize(chunkSize);
}

bool MQTTController::addToPublishQueue(const String &topic, const String &payload) {

    bool result = false;
#ifdef INC_FREERTOS_H
    if (xSemaphoreTake(semaQueue, portMAX_DELAY)) {
#endif
    MQTTMessage message(topic, payload);
    if (queue != nullptr && queue->push(message)) result = true;
    else result = false;
#ifdef INC_FREERTOS_H
    xSemaphoreGive(semaQueue);
}
#endif
    return result;
}

MQTTController::MQTTController(int queueSize) {
    delete queue;
    queue = new Queue<MQTTMessage>(queueSize);
    defaultTimeout = 3000;
    defaultBufferSize = 5120;
    timeout = defaultTimeout;
    attrRequestId = 0;
    jsonSerializeBuffer = 1024;
}

bool
MQTTController::requestAttributesJson(const String &keysJson, const MQTTController::MqttCallbackJsonPayload &callback) {
    attrRequestId++;
    if (callback != nullptr)
        requestsCallbacksJson[attrRequestId] = callback;

    return addToPublishQueue(String("v1/devices/me/attributes/request/" + String(attrRequestId)),
                             keysJson);
}

bool MQTTController::sendAttributes(JsonDocument json, const String &deviceName) {
    if (!deviceName.isEmpty()) {
        JsonDocument newData;
        newData[deviceName] = json;
        newData.shrinkToFit();
        return addToPublishQueue(V1_Attributes_GATEWAY_TOPIC, newData.as<String>());
    }
    json.shrinkToFit();
    return addToPublishQueue(V1_Attributes_TOPIC, json.as<String>());
}

bool MQTTController::sendTelemetry(JsonDocument json, const String &deviceName) {
    if (!deviceName.isEmpty()) {
        JsonDocument newData;
        newData[deviceName].add(json);
        newData.shrinkToFit();
        return addToPublishQueue(V1_TELEMETRY_GATEWAY_TOPIC, newData.as<String>());
    }
    json.shrinkToFit();
    return addToPublishQueue(V1_TELEMETRY_TOPIC, json.as<String>());
}

bool MQTTController::sendAttributes(const String &json) {
    return addToPublishQueue(V1_Attributes_TOPIC, json);
}

bool MQTTController::sendTelemetry(const String &json) {
    return addToPublishQueue(V1_TELEMETRY_TOPIC, json);
}

bool MQTTController::sendClaimRequest(const String &key, uint32_t duration_ms, const String &deviceName) {
    JsonDocument data;
    data["secretKey"] = key;
    data["durationMs"] = duration_ms;
    data.shrinkToFit();

    if (!deviceName.isEmpty()) {
        JsonDocument deviceData;
        deviceData[deviceName] = data.as<String>();
        deviceData.shrinkToFit();
        return addToPublishQueue("v1/gateway/claim", deviceData.as<String>());
    }

    return addToPublishQueue("v1/devices/me/claim", data.as<String>());
}

void MQTTController::setTimeout(int timeout_ms) {
    this->timeout = timeout_ms;
}

void MQTTController::resetTimeout() {
    timeout = defaultTimeout;
}

void MQTTController::resetBufferSize() {
    mqttClient.setBufferSize(defaultBufferSize);
}

void MQTTController::sendSystemAttributes(bool value) {
    MQTTController::isSendAttributes = value;
}

uint16_t MQTTController::getQueueSize() {
    if (queue != nullptr) return queue->getCounts();
    return 0;
}

bool MQTTController::sendGatewayConnectEvent(const String &deviceName) {
    JsonDocument data;
    data["device"] = deviceName;
    data.shrinkToFit();
    return addToPublishQueue("v1/gateway/connect", deviceName);
}

bool MQTTController::sendGatewayDisConnectEvent(const String &deviceName) {
    JsonDocument data;
    data["device"] = deviceName;
    data.shrinkToFit();
    return addToPublishQueue("v1/gateway/disconnect", deviceName);
}

bool MQTTController::subscribeToGatewayEvent() {
    if (mqttClient.subscribe(V1_Attributes_GATEWAY_TOPIC))
        return mqttClient.subscribe("v1/gateway/rpc");
    return false;
}

bool MQTTController::unsubscribeToGatewayEvent() {
    if (mqttClient.unsubscribe(V1_Attributes_GATEWAY_TOPIC))
        return mqttClient.unsubscribe("v1/gateway/rpc");
    return false;
}

void MQTTController::onSentMQTTMessageCallback(const MQTTController::SentMQTTMessageCallback &callback) {
    sentMqttMessageCallback = callback;
}

#endif