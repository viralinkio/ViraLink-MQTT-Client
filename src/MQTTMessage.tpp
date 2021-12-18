#ifndef VIRALINK_MQTT_Message_TPP
#define VIRALINK_MQTT_Message_TPP

#define V1_Attributes_TOPIC "v1/devices/me/attributes"
#define V1_Attributes_GATEWAY_TOPIC "v1/gateway/attributes"

#define V1_TELEMETRY_TOPIC "v1/devices/me/telemetry"
#define V1_TELEMETRY_GATEWAY_TOPIC "v1/gateway/telemetry"

class MQTTMessage {
public:
    MQTTMessage(const String &topic, const String &payload);

    MQTTMessage();

    const String &getTopic() const;

    const String &getPayload() const {
        return payload;
    }

    unsigned short getRetry() const;

    void incrementRetry();

private:
    unsigned short retry;
    String topic, payload;
};

MQTTMessage::MQTTMessage() {
    topic = "";
    payload = "";
}

MQTTMessage::MQTTMessage(const String &messageTopic, const String &messagePayload) {
    this->topic = messageTopic;
    this->payload = messagePayload;
    retry = 0;
}

const String &MQTTMessage::getTopic() const {
    return topic;
}

unsigned short MQTTMessage::getRetry() const {
    return retry;
}

void MQTTMessage::incrementRetry() {
    retry++;
}

#endif