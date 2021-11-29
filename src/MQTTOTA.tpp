#ifndef VIRALINK_MQTTOTA_TPP
#define VIRALINK_MQTTOTA_TPP

#include <Ticker.h>

#define FW_CHECKSUM_ATTR "fw_checksum"
#define FW_CHECKSUM_ALG_ATTR "fw_checksum_algorithm"
#define FW_SIZE_ATTR "fw_size"
#define FW_TITLE_ATTR "fw_title"
#define FW_VERSION_ATTR "fw_version"

#define FW_STATE_ATTR "fw_state"
#define FW_ERROR_ATTR "fw_error"

#define FW_REQUEST_TOPIC "v2/fw/request/"
#define FW_RESPONSE_TOPIC "v2/fw/response/"

void OTAResetESP() {
    ESP.restart();
}

class MQTTOTA {
public:
    bool begin(String current_fw_title, String current_fw_version);

    MQTTOTA(MQTTController *mqttController, uint16_t chunkSize);

private:
    String current_fw_title, current_fw_version;
    MQTTController *mqttController;
    Ticker restartTicker;
    uint16_t totalChunks, chunkSize, currentChunk, requestId;

    bool handleMessage(String topic, DynamicJsonDocument json);

    bool handleMessageRaw(String topic, uint8_t *payload, unsigned int length);

    void requestChunkPart(int chunkPart);
};

bool MQTTOTA::handleMessage(String topic, DynamicJsonDocument json) {
    if (json.containsKey("shared"))
        json = json.getMember("shared");

    if (!json.containsKey(FW_CHECKSUM_ATTR) || !json.containsKey(FW_CHECKSUM_ALG_ATTR) ||
        !json.containsKey(FW_SIZE_ATTR) || !json.containsKey(FW_TITLE_ATTR) || !json.containsKey(FW_VERSION_ATTR))
        return false;

    String targetTitle = json[FW_TITLE_ATTR].as<String>();
    String targetVersion = json[FW_VERSION_ATTR].as<String>();

    if (targetTitle.equals(this->current_fw_title) && targetVersion.equals(this->current_fw_version)) {
        printDBGln("Firmware is Up-to-date");
        return true;
    }

    if (json[FW_CHECKSUM_ATTR].as<String>().equals(ESP.getSketchMD5())) {
        printDBGln("Current MD5 is equal with New Firmware MD5.");
        DynamicJsonDocument status(100);
        status[FW_STATE_ATTR] = "FAILED";
        status[FW_ERROR_ATTR] = "Current MD5 is equal with New Firmware MD5.";
        mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, status.as<String>());
        return true;
    }

    // starting OTA Update
    printDBGln(
            String("New Firmware Available .... Start Update from [" + current_fw_title + ":" + current_fw_version +
                   "] To [" + targetTitle + ":" + targetVersion + "]"));

    if (!json[FW_CHECKSUM_ALG_ATTR].as<String>().equals("MD5")) {
        printDBGln("Unsupported checksum Algorithm");
        DynamicJsonDocument status(100);
        status[FW_STATE_ATTR] = "FAILED";
        status[FW_ERROR_ATTR] = "Unsupported checksum Algorithm";
        mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, status.as<String>());
        return true;
    }

    OTAUpdate.onStart([=]() {
        printDBGln("OTA started");
        DynamicJsonDocument status(100);
        status[FW_STATE_ATTR] = "UPDATING";
        mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, status.as<String>());

        mqttController->setTimeout(30000);
        if (!mqttController->getMqttClient()->setBufferSize(chunkSize + 50)) {
            mqttController->resetTimeout();
            mqttController->resetBufferSize();
            printDBGln("NOT ENOUGH RAM!");
            status[FW_STATE_ATTR] = "FAILED";
            status[FW_ERROR_ATTR] = "NOT ENOUGH RAM!";
            mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, status.as<String>());
            return;
        }

        requestId = random(1, 1000);
        totalChunks = (json[FW_SIZE_ATTR].as<String>().toInt() / chunkSize);
        if (json[FW_SIZE_ATTR].as<String>().toInt() % chunkSize == 0) totalChunks--;
        requestChunkPart(0);
    });

    OTAUpdate.onEnd([=](bool result) {
        mqttController->resetTimeout();
        mqttController->resetBufferSize();
        if (result) {
            printDBGln("OTA Ended Successfully! ");
            DynamicJsonDocument status(200);
            status[FW_TITLE_ATTR] = targetTitle;
            status[FW_VERSION_ATTR] = targetVersion;
            status[FW_STATE_ATTR] = "UPDATED";
            mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, status.as<String>());
            restartTicker.once(3, OTAResetESP);
        }
    });

    OTAUpdate.onProgress([=](int current, int total) {

    });

    OTAUpdate.onError([=](int err) {
        mqttController->resetTimeout();
        mqttController->resetBufferSize();
        printDBGln(String("OTA ERROR [" + String(err) + "]: " + OTAUpdate.getLastErrorString()));
        DynamicJsonDocument status(100);
        status[FW_STATE_ATTR] = "FAILED";
        status[FW_ERROR_ATTR] = String("OTA ERROR [" + String(err) + "]: " + OTAUpdate.getLastErrorString());
        mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, status.as<String>());
    });

    OTAUpdate.rebootOnUpdate(false);
    if (!OTAUpdate.startUpdate(json[FW_SIZE_ATTR], json[FW_CHECKSUM_ATTR])) {
        printDBGln("Can Not start OTA");
        printDBGln(OTAUpdate.getLastErrorString());
        DynamicJsonDocument status(100);
        status[FW_STATE_ATTR] = "FAILED";
        mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, status.as<String>());
    }

    return true;
}

bool MQTTOTA::handleMessageRaw(String topic, uint8_t *payload, unsigned int length) {
    if (topic.indexOf(FW_RESPONSE_TOPIC) != 0) return false;

    int id = topic.substring(15, topic.indexOf("/", 15)).toInt();
    if (requestId != id)
        return true;

    int chunkPart = topic.substring(topic.lastIndexOf("/") + 1).toInt();
    if (OTAUpdate.isUpdating() && chunkPart == currentChunk) {
        printDBGln(String("Writing Chuck part: " + String(currentChunk)));
        printDBG("OTA progress: ");
        printDBGln(String(String((((float) currentChunk) / ((float) totalChunks)) * 100) + "%"));

        if (!OTAUpdate.writeUpdateChunk(payload, length))
            return true;

        if (currentChunk == totalChunks) {
            OTAUpdate.endUpdate();
        } else {
            currentChunk++;
            requestChunkPart(currentChunk);
        }
    }
    return true;
}

bool MQTTOTA::begin(String currentFirmwareTitle, String currentFirmwareVersion) {
    requestId = 0;

    this->current_fw_title = currentFirmwareTitle;
    this->current_fw_version = currentFirmwareVersion;

    DynamicJsonDocument data(200);
    data["current_fw_title"] = current_fw_title;
    data["current_fw_version"] = current_fw_version;
    data.shrinkToFit();
    mqttController->addToPublishQueue(V1_TELEMETRY_TOPIC, data.as<String>());

    DynamicJsonDocument requestData(300);
    requestData["sharedKeys"] = String(
            String(FW_CHECKSUM_ATTR) + "," + FW_CHECKSUM_ALG_ATTR + "," + FW_SIZE_ATTR + "," + FW_TITLE_ATTR + "," +
            FW_VERSION_ATTR);
    requestData.shrinkToFit();

    MQTTController::MqttCallbackJsonPayload callbackJson = [this](String topic, DynamicJsonDocument json) -> bool {
        return handleMessage(topic, json);
    };
    mqttController->requestAttributesJson(requestData.as<String>(), callbackJson);
    mqttController->registerCallbackJsonPayload(callbackJson);
    mqttController->registerCallbackRawPayload([this](String topic, uint8_t *payload, unsigned int length) -> bool {
        return handleMessageRaw(topic, payload, length);
    });
    return false;
}

void MQTTOTA::requestChunkPart(int chunkPart) {
    mqttController->addToPublishQueue(
            String(FW_REQUEST_TOPIC + String(requestId) + "/chunk/" + String(chunkPart)),
            String(chunkSize));
}

MQTTOTA::MQTTOTA(MQTTController *mqttController, uint16_t chunkSize) : mqttController(mqttController),
                                                                       chunkSize(chunkSize) {}

#endif
