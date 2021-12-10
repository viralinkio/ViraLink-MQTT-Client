#ifndef VIRALINK_NETWORK_CONTROLLER_TPP
#define VIRALINK_NETWORK_CONTROLLER_TPP

#include "PrintDBG.tpp"
#include "Uptime.h"
#include "map"
#include "NetworkInterface.h"
#include "vector"

typedef bool (*OnConnectionEvent)(NetworkInterface *networkInterface);

bool compareInterval(const NetworkInterface &i1, const NetworkInterface &i2) {
    return (i1.getPriority() < i2.getPriority());
}

class NetworkInterfacesController {

private:
    std::vector<NetworkInterface> networkInterfaces;
    short currentTryingInterfaceIndex = -1;
    bool autoConnect, autoReconnect;
    uint32_t autoReconnectCheckPeriod;
    uint64_t lastCheckedConnectionMS;

public:
    void addNetworkInterface(const NetworkInterface &networkInterface);

    bool connectToNetwork(NetworkInterface *targetNetwork);

    bool connectToNetwork(uint8_t id);

    void loop();

    void autoConnectToNetwork();

    short findNetworkInterfaceIndexById(uint8_t id);

    short findNetworkInterfaceIndexByReference(NetworkInterface *networkInterface);

    void setAutoReconnect(bool autoReconnectValue, uint32_t connectionCheckPeriodValue = 3000);

    NetworkInterface *getCurrentNetworkInterface();
};

void NetworkInterfacesController::addNetworkInterface(const NetworkInterface &networkInterface) {
    networkInterfaces.push_back(networkInterface);
}

bool NetworkInterfacesController::connectToNetwork(uint8_t id) {
    short index = findNetworkInterfaceIndexById(id);
    if (index != -1) return connectToNetwork(&networkInterfaces[findNetworkInterfaceIndexById(id)]);
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
    if (!networkInterfaces[currentTryingInterfaceIndex].connect()) return false;
    return true;
}

void NetworkInterfacesController::loop() {
    if (networkInterfaces.empty() || currentTryingInterfaceIndex < 0 ||
        currentTryingInterfaceIndex >= networkInterfaces.size())
        return;

    NetworkInterface *networkInterface = &networkInterfaces[currentTryingInterfaceIndex];
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

    if (autoConnect && (currentTryingInterfaceIndex < networkInterfaces.size() - 1)) {
        currentTryingInterfaceIndex++;

        printDBGln("[Auto Connect Mode]: change to next network interface: " +
                   networkInterfaces[currentTryingInterfaceIndex].getName());
        networkInterfaces[currentTryingInterfaceIndex].connect();
        return;
    }

    if (autoReconnect) {
        if (autoConnect) {
            currentTryingInterfaceIndex = 0;
            printDBGln("[Auto Connect Mode + AutoReconnect]: Start again from first priority. connecting to: " +
                       networkInterfaces[currentTryingInterfaceIndex].getName());
            networkInterfaces[currentTryingInterfaceIndex].connect();
            return;
        }

        printDBGln("[AutoReconnect]: Retry Connecting to: " +
                   networkInterfaces[currentTryingInterfaceIndex].getName());
        networkInterfaces[currentTryingInterfaceIndex].connect();
    }


}

void NetworkInterfacesController::autoConnectToNetwork() {

    if (networkInterfaces.empty()) {
        printDBGln("Network Interfaces are empty.");
        return;
    }
    sort(networkInterfaces.begin(), networkInterfaces.end(), compareInterval);
    autoConnect = true;
    currentTryingInterfaceIndex = 0;
    networkInterfaces[currentTryingInterfaceIndex].connect();
}

short NetworkInterfacesController::findNetworkInterfaceIndexById(uint8_t id) {
    std::vector<NetworkInterface>::iterator it;
    short i = 0;
    for (it = networkInterfaces.begin(); it != networkInterfaces.end(); it++, i++) {
        if (it->getId() == id)
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
    if (!networkInterfaces.empty() && currentTryingInterfaceIndex >= 0 &&
        currentTryingInterfaceIndex < networkInterfaces.size())
        return &networkInterfaces[currentTryingInterfaceIndex];

    return nullptr;
}

#endif