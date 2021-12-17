#ifndef VIRALINK_NETWORK_INTERFACE_TPP
#define VIRALINK_NETWORK_INTERFACE_TPP

typedef bool (*NetworkCommandInterface)();

typedef void (*ConnectionEvent)();

typedef Client *(*CreateClientInterface)();

class NetworkInterface {

private:
    String name;
    ConnectionEvent connectedEvent, timeoutEvent, connectingEvent;
    uint8_t id, priority;
    uint32_t timeout_ms = 30000;
    uint32_t connectionEventPeriod_ms = 50;
    NetworkCommandInterface connectionCheckInterface, connectInterface, disconnectInterface;
    CreateClientInterface createClientInterface;
    uint64_t startTryingTimeMS, lastConnectingEventMs;
    bool tryingToConnect, connected;

public:

    bool connect();

    bool lastConnectionStatus() const;

    bool checkConnection();

    bool disconnect();

    Client *createClient();

    NetworkInterface();

    NetworkInterface(const String &name, uint8_t id, uint8_t priority = 1, uint32_t timeoutMs = 30000);

    void OnConnectedEvent(ConnectionEvent onConnectionEvent) {
        connectedEvent = onConnectionEvent;
    }

    void OnTimeoutEvent(ConnectionEvent onTimeoutEvent) {
        timeoutEvent = onTimeoutEvent;
    }

    void OnConnectingEvent(ConnectionEvent onConnectingEvent, uint32_t eventCallPeriod_ms = 50) {
        connectionEventPeriod_ms = eventCallPeriod_ms;
        connectingEvent = onConnectingEvent;
    }

    void setConnectionCheckInterfaceInterface(NetworkCommandInterface networkCommandInterface) {
        NetworkInterface::connectionCheckInterface = networkCommandInterface;
    }

    void setConnectInterface(NetworkCommandInterface networkCommandInterface) {
        NetworkInterface::connectInterface = networkCommandInterface;
    }

    void setDisconnectInterface(NetworkCommandInterface networkCommandInterface) {
        NetworkInterface::disconnectInterface = networkCommandInterface;
    }

    void setCreateClientInterface(CreateClientInterface clientInterface) {
        NetworkInterface::createClientInterface = clientInterface;
    }

    void setTimeoutMs(uint32_t timeoutMs) {
        timeout_ms = timeoutMs;
    }

    void setPriority(uint8_t priority);

    const String &getName() const {
        return name;
    }

    uint8_t getId() const {
        return id;
    }

    uint8_t getPriority() const {
        return priority;
    }

    uint32_t getTimeoutMs() const {
        return timeout_ms;
    }

    void loop();

    bool isConnecting() const;

};

bool NetworkInterface::connect() {
    if (connectInterface == nullptr) {
        printDBG("Connect Interface SHOULD NOT be NULL");
        return false;
    }

    if (connectInterface()) {
        startTryingTimeMS = Uptime.getMilliseconds();
        tryingToConnect = true;
        return true;
    }
    return false;
}

bool NetworkInterface::isConnecting() const {
    return tryingToConnect;
}

bool NetworkInterface::lastConnectionStatus() const {
    return connected;
}

bool NetworkInterface::checkConnection() {
    connected = connectionCheckInterface != nullptr && connectionCheckInterface();
    return connected;
}

bool NetworkInterface::disconnect() {
    tryingToConnect = false;
    if (disconnectInterface != nullptr) return disconnectInterface();
    return true;
}

Client *NetworkInterface::createClient() {
    if (createClientInterface != nullptr) return createClientInterface();
    return nullptr;
}

NetworkInterface::NetworkInterface() {}

void NetworkInterface::loop() {

    if (!tryingToConnect) return;

    //todo make a little period in connection check
    if (checkConnection()) {
        tryingToConnect = false;
        if (connectedEvent != nullptr) connectedEvent();
        return;
    }

    if ((Uptime.getMilliseconds() - startTryingTimeMS) > timeout_ms) {
        tryingToConnect = false;
        if (timeoutEvent != nullptr) timeoutEvent();
        return;
    }

    if (connectingEvent != nullptr && ((Uptime.getMilliseconds() - lastConnectingEventMs) > connectionEventPeriod_ms)) {
        lastConnectingEventMs = Uptime.getMilliseconds();
        connectingEvent();
    }

}

NetworkInterface::NetworkInterface(const String &name, uint8_t id, uint8_t priority, uint32_t timeoutMs) :
        name(name), id(id), priority(priority), timeout_ms(timeoutMs) {}

void NetworkInterface::setPriority(uint8_t newPriority) {
    NetworkInterface::priority = newPriority;
}

#endif