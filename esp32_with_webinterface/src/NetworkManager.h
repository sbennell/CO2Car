#pragma once

#include <WiFi.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <time.h>
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
    static const char* AP_PASSWORD;
    static const unsigned long CHECK_INTERVAL = 5000;  // Check connection every 5 seconds
    static const unsigned long CONNECT_TIMEOUT = 30000;  // 30 seconds timeout for connection
    
    void startStation();
    void startAP();
    void checkConnection();
    String generateAPName();
    void onWiFiEvent(WiFiEvent_t event);
    
    Configuration& config;
    bool apMode;
    bool connected;
    bool connecting;
    unsigned long lastCheck;
    unsigned long connectStartTime;
    DNSServer dnsServer;
    WiFiEventId_t eventId;
};
