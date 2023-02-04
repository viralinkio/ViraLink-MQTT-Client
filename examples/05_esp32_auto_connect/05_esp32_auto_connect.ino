#include "Arduino.h"

#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#include "viralink.h"

#include "NetworkController.h"

#if defined(ESP32)
#include "WiFi.h"
#elif defined(ESP8266)
#include "ESP8266WiFi.h"
#endif

#include <ETH.h>

WiFiClient ethClient;

#define TINY_GSM_MODEM_BG96     //GSM MODEL Depends on Each Device
#define SerialAT Serial2          //if you enable GSM feature the SerialAT should be defined
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient tinyGsmClient(modem);

//NetworkInterface(name, id, priority = 1, timeout = 30000)
NetworkInterface lanInterface("lan8720", 3, 1);
NetworkInterface wifiInterface("wifi", 1, 2, 10000);
NetworkInterface modemInterface("modem", 2, 3);

#define WIFI_SSID "xxxxxxx"
#define WIFI_PASSWORD "xxxxxxx"
#define SIM_APN "apn"

NetworkInterfacesController networkController;

void setup() {

    SerialMon.begin(115200);
    Serial.println();

    wifiInterface.setConnectInterface([]() -> bool {
        Serial.print("Connecting To WiFi ");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        return true;
    });
    wifiInterface.setConnectionCheckInterfaceInterface([]() -> bool {
        return WiFi.status() == WL_CONNECTED;
    });
    wifiInterface.OnConnectingEvent([]() {
        Serial.print(".");
    }, 500);
    wifiInterface.OnConnectedEvent([]() {
        Serial.println(String("Connected to WIFI with IP: ") + WiFi.localIP().toString());
    });

    lanInterface.setConnectInterface([]() -> bool {
        Serial.print("Connecting To LAN8720 ");
        return ETH.begin(1);
    });
    lanInterface.setConnectionCheckInterfaceInterface([]() -> bool {
        return ETH.linkUp() && !ETH.localIP().toString().equals("0.0.0.0");
    });
    lanInterface.OnConnectingEvent([]() {
        Serial.print(".");
    }, 500);
    lanInterface.OnConnectedEvent([]() {
        Serial.println(String("Connected to LAN with IP: ") + ETH.localIP().toString());
    });

    // no need to timeout because the function only Trying to connect only ones;
    modemInterface.setTimeoutMs(0);
    modemInterface.setConnectInterface([]() -> bool {
        SerialAT.begin(9600);
        Serial.println("Initializing modem...");
        if (!modem.restart())
            return false;

        String name = modem.getModemName();
        Serial.println("Modem Name: " + name);

        String modemInfo = modem.getModemInfo();
        Serial.println("Modem Info: " + modemInfo);

        if (!modem.isNetworkConnected()) {
            Serial.println("Waiting for network...");
            if (!modem.waitForNetwork(120000L))
                return false;
        }

        if (modem.isNetworkConnected()) { Serial.println("Network connected"); }

        Serial.println("Connecting to apn");
        if (!modem.gprsConnect(SIM_APN, "", ""))
            return false;

        Serial.println("Connected to GPRS");

        return true;
    });
    modemInterface.setConnectionCheckInterfaceInterface([]() -> bool {
        return modem.isGprsConnected();
    });
    modemInterface.OnConnectedEvent([]() {
        Serial.println(String("Connected to GSM with IP: ") + modem.localIP().toString());
    });

    networkController.addNetworkInterface(&wifiInterface);
    networkController.addNetworkInterface(&lanInterface);
    networkController.addNetworkInterface(&modemInterface);

    networkController.setAutoReconnect(true);
    networkController.autoConnectToNetwork();

}

void loop() {
    networkController.loop();
}