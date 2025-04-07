#pragma once

#include <WiFi.h>
#include <DNSServer.h>
#include "Configuration.h"

class NetworkManager {
public:
    NetworkManager(Configuration& cfg);
    void begin();
    void update();
    bool isConnected() const;
    bool isAPMode() const;
    String getIP() const;
    String getSSID() const;
    int getRSSI() const;
    void reconnect();  // Force reconnect with current config

private:
    void startAP();
    void startStation();
    void checkConnection();
    String generateAPName();

    Configuration& config;
    DNSServer dnsServer;
    bool apMode;
    bool connected;
    unsigned long lastCheck;
    const unsigned long CHECK_INTERVAL = 5000;  // Check every 5 seconds
    const char* AP_PASSWORD = "co2racer";       // Simple password for AP mode
};
