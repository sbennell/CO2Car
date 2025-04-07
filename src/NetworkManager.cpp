#include "NetworkManager.h"
#include <esp_wifi.h>

NetworkManager::NetworkManager(Configuration& cfg) 
    : config(cfg), apMode(false), connected(false), lastCheck(0) {}

void NetworkManager::begin() {
    // First try station mode
    startStation();
    
    // If connection fails, start AP mode
    if (!isConnected()) {
        startAP();
    }
}

void NetworkManager::startStation() {
    if (apMode) {
        // Stop AP mode and DNS server
        WiFi.softAPdisconnect(true);
        dnsServer.stop();
        apMode = false;
    }

    Serial.print("\n📱 Connecting to WiFi: ");
    Serial.println(config.getWiFiSSID());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.getWiFiSSID().c_str(), config.getWiFiPassword().c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        Serial.println("\n✅ Connected to WiFi");
        Serial.print("📍 IP: ");
        Serial.println(WiFi.localIP());
    } else {
        connected = false;
        Serial.println("\n❌ WiFi connection failed");
        WiFi.disconnect();
    }
}

void NetworkManager::startAP() {
    Serial.println("\n📡 Starting Access Point mode...");
    
    WiFi.mode(WIFI_AP);
    String apName = generateAPName();
    
    if (WiFi.softAP(apName.c_str(), AP_PASSWORD)) {
        apMode = true;
        connected = true;
        Serial.println("✅ AP started successfully");
        Serial.print("📍 AP IP: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("📶 SSID: ");
        Serial.println(apName);
        Serial.print("🔑 Password: ");
        Serial.println(AP_PASSWORD);
        
        // Start DNS server to redirect all requests to our web server
        dnsServer.start(53, "*", WiFi.softAPIP());
    } else {
        apMode = false;
        connected = false;
        Serial.println("❌ Failed to start AP");
    }
}

void NetworkManager::update() {
    if (millis() - lastCheck >= CHECK_INTERVAL) {
        lastCheck = millis();
        checkConnection();
    }
    
    if (apMode) {
        dnsServer.processNextRequest();
    }
}

void NetworkManager::checkConnection() {
    if (!apMode && WiFi.status() != WL_CONNECTED) {
        connected = false;
        Serial.println("\n🔄 WiFi disconnected, attempting to reconnect...");
        startStation();
        
        // If station mode fails, switch to AP mode
        if (!isConnected()) {
            startAP();
        }
    }
}

String NetworkManager::generateAPName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[32];
    snprintf(apName, sizeof(apName), "CO2RaceTimer-%02X%02X", mac[4], mac[5]);
    return String(apName);
}

bool NetworkManager::isConnected() const {
    return connected;
}

bool NetworkManager::isAPMode() const {
    return apMode;
}

String NetworkManager::getIP() const {
    return apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

String NetworkManager::getSSID() const {
    return apMode ? WiFi.softAPSSID() : WiFi.SSID();
}

int NetworkManager::getRSSI() const {
    return apMode ? 0 : WiFi.RSSI();
}

void NetworkManager::reconnect() {
    Serial.println("🔄 Reconnecting with new WiFi settings...");
    WiFi.disconnect(true);
    delay(1000);  // Give time for disconnection
    startStation();
    
    // If station mode fails, switch to AP mode
    if (!isConnected()) {
        startAP();
    }
}
