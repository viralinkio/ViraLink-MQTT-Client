#ifndef VIRALINK_NETWORK_CONTROLLER_TPP
#define VIRALINK_NETWORK_CONTROLLER_TPP

#define MAX_INTERFACES_SIZE 5

#include "PrintDBG.tpp"
#include "Uptime.h"
#include "NetworkInterface.h"

typedef bool (*OnConnectionEvent)(NetworkInterface *networkInterface);

class NetworkInterfacesController {

private:
    NetworkInterface *networkInterfaces[MAX_INTERFACES_SIZE];
    unsigned short networkInterfacesCurrentSize = 0;
    short currentTryingInterfaceIndex = -1;
    bool autoConnect, autoReconnect;
    uint32_t autoReconnectCheckPeriod;
    uint64_t lastCheckedConnectionMS;

public:
    bool addNetworkInterface(NetworkInterface *networkInterface);

    bool connectToNetwork(NetworkInterface *targetNetwork);

    bool connectToNetwork(uint8_t id);

    void loop();

    void autoConnectToNetwork();

    short findNetworkInterfaceIndexById(uint8_t id);

    short findNetworkInterfaceIndexByReference(NetworkInterface *networkInterface);

    void setAutoReconnect(bool autoReconnectValue, uint32_t connectionCheckPeriodValue = 3000);

    NetworkInterface *getCurrentNetworkInterface();

    void sortInterfaces();
};

bool NetworkInterfacesController::addNetworkInterface(NetworkInterface *networkInterface) {
    if (networkInterfacesCurrentSize < MAX_INTERFACES_SIZE) {
        networkInterfaces[networkInterfacesCurrentSize] = networkInterface;
        networkInterfacesCurrentSize++;
        return true;
    }
    return false;
}

bool NetworkInterfacesController::connectToNetwork(uint8_t id) {
    short index = findNetworkInterfaceIndexById(id);
    if (index != -1) return connectToNetwork(networkInterfaces[findNetworkInterfaceIndexById(id)]);
    return false;
}

bool NetworkInterfacesController::connectToNetwork(NetworkInterface *targetNetwork) {

    if (targetNetwork == nullptr) {
        printDBG("Network Interface Can not be NULL");
        return false;
    }

    short index = findNetworkInterfaceIndexByReference(targetNetwork);
    if (index == -1) {
        printDBGln("Network Interface Not Found. Add it to NetworkController");
        return false;
    }

    currentTryingInterfaceIndex = index;
    autoConnect = false;
    if (!networkInterfaces[currentTryingInterfaceIndex]->connect()) return false;
    return true;
}

void NetworkInterfacesController::loop() {
    if (networkInterfacesCurrentSize == 0 || currentTryingInterfaceIndex < 0 ||
        currentTryingInterfaceIndex >= networkInterfacesCurrentSize)
        return;

    NetworkInterface *networkInterface = networkInterfaces[currentTryingInterfaceIndex];
    networkInterface->loop();

    if (networkInterface->isConnecting()) return;

    uint64_t now = Uptime.getMilliseconds();

    if (networkInterface->lastConnectionStatus()) {
        if (autoReconnect && (now - lastCheckedConnectionMS) > autoReconnectCheckPeriod) {
            lastCheckedConnectionMS = now;
            networkInterface->checkConnection();
        }
        return;
    }

    printDBGln("Could not Connect to " + networkInterface->getName());

    if (autoConnect && (currentTryingInterfaceIndex < networkInterfacesCurrentSize - 1)) {
        currentTryingInterfaceIndex++;

        printDBGln("[Auto Connect Mode]: change to next network interface: " +
                   networkInterfaces[currentTryingInterfaceIndex]->getName());
        networkInterfaces[currentTryingInterfaceIndex]->connect();
        return;
    }

    if (autoReconnect) {
        if (autoConnect) {
            currentTryingInterfaceIndex = 0;
            printDBGln("[Auto Connect Mode + AutoReconnect]: Start again from first priority. connecting to: " +
                       networkInterfaces[currentTryingInterfaceIndex]->getName());
            networkInterfaces[currentTryingInterfaceIndex]->connect();
            return;
        }

        printDBGln("[AutoReconnect]: Retry Connecting to: " +
                   networkInterfaces[currentTryingInterfaceIndex]->getName());
        networkInterfaces[currentTryingInterfaceIndex]->connect();
    }


}

void NetworkInterfacesController::autoConnectToNetwork() {

    if (networkInterfacesCurrentSize == 0) {
        printDBGln("Network Interfaces are empty.");
        return;
    }

    sortInterfaces();
    autoConnect = true;
    currentTryingInterfaceIndex = 0;
    networkInterfaces[currentTryingInterfaceIndex]->connect();
}

short NetworkInterfacesController::findNetworkInterfaceIndexById(uint8_t id) {
    for (int i = 0; i < networkInterfacesCurrentSize; i++) {
        if (networkInterfaces[i]->getId() == id)
            return i;
    }
    return -1;
}

short NetworkInterfacesController::findNetworkInterfaceIndexByReference(NetworkInterface *networkInterface) {
    if (networkInterface == nullptr)
        return -1;
    return findNetworkInterfaceIndexById(networkInterface->getId());
}

void NetworkInterfacesController::setAutoReconnect(bool autoReconnectValue, uint32_t connectionCheckPeriodValue) {
    NetworkInterfacesController::autoReconnect = autoReconnectValue;
    autoReconnectCheckPeriod = connectionCheckPeriodValue;
}

NetworkInterface *NetworkInterfacesController::getCurrentNetworkInterface() {
    if (!networkInterfacesCurrentSize == 0 && currentTryingInterfaceIndex >= 0 &&
        currentTryingInterfaceIndex < networkInterfacesCurrentSize)
        return networkInterfaces[currentTryingInterfaceIndex];

    return nullptr;
}

void NetworkInterfacesController::sortInterfaces() {
    for (size_t i = 0; i < networkInterfacesCurrentSize; ++i) {
        for (size_t j = i + 1; j < networkInterfacesCurrentSize; ++j) {
            if (networkInterfaces[i]->getPriority() > networkInterfaces[j]->getPriority()) {
                std::swap(networkInterfaces[i], networkInterfaces[j]);
            }
        }
    }
}

#endif