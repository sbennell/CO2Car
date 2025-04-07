#include "NetworkManager.h"
#include <esp_wifi.h>

const char* NetworkManager::AP_PASSWORD = "co2racer";

NetworkManager::NetworkManager(Configuration& cfg) 
    : config(cfg), apMode(false), connected(false), connecting(false),
      lastCheck(0), connectStartTime(0), eventId(0) {}

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

    // Remove any existing event handler
    if (eventId) {
        WiFi.removeEvent(eventId);
    }

    // Clean up existing connections
    WiFi.disconnect(true, true);  // Disconnect and erase stored credentials
    delay(100);  // Brief delay for cleanup
    
    // Register WiFi event handler
    eventId = WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        this->onWiFiEvent(event);
    });

    Serial.print("\n📱 Connecting to WiFi: ");
    Serial.println(config.getWiFiSSID());
    
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);  // Don't write WiFi settings to flash
    WiFi.setAutoReconnect(false);  // We'll handle reconnection
    WiFi.begin(config.getWiFiSSID().c_str(), config.getWiFiPassword().c_str());
    
    connecting = true;
    connectStartTime = millis();
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
    if (connecting && (millis() - connectStartTime >= CONNECT_TIMEOUT)) {
        Serial.println("\n❌ WiFi connection timeout");
        connecting = false;
        connected = false;
        
        // Clean up failed connection
        WiFi.disconnect(true);
        
        // If we're not in AP mode and connection failed, switch to AP
        if (!apMode) {
            startAP();
        }
    } else if (!apMode && !connecting && !connected && 
               (millis() - lastCheck >= CHECK_INTERVAL)) {
        // Try to reconnect periodically if we're not connected
        lastCheck = millis();
        startStation();
    }
    
    if (apMode) {
        dnsServer.processNextRequest();
    }

    // Give other tasks a chance to run
    yield();
}

void NetworkManager::checkConnection() {
    if (!apMode && !connecting && WiFi.status() != WL_CONNECTED) {
        connected = false;
        Serial.println("\n🔄 WiFi disconnected, attempting to reconnect...");
        startStation();
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

void NetworkManager::onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            connecting = false;
            connected = true;
            Serial.println("\n✅ Connected to WiFi");
            Serial.print("📍 IP: ");
            Serial.println(WiFi.localIP());
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            connected = false;
            if (!connecting) {
                // Only show message if we were previously connected
                Serial.println("\n🔄 WiFi connection lost");
            }
            break;
            
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
            
        case SYSTEM_EVENT_STA_STOP:
            Serial.println("WiFi client stopped");
            break;
            
        default:
            break;
    }
}

void NetworkManager::reconnect() {
    Serial.println("🔄 Reconnecting with new WiFi settings...");
    
    // Stop WiFi completely
    WiFi.mode(WIFI_OFF);
    delay(100);  // Brief delay for cleanup
    
    connected = false;
    connecting = false;
    
    // Start new connection attempt
    startStation();
}
