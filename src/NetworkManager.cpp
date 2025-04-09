#include "NetworkManager.h"
#include <esp_wifi.h>

const char* NetworkManager::AP_PASSWORD = "co2racer";

NetworkManager::NetworkManager(Configuration& cfg) 
    : config(cfg), apMode(false), connected(false), connecting(false),
      lastCheck(0), connectStartTime(0), eventId(0) {
    // Force configuration load on construction
    config.begin();
}

void NetworkManager::begin() {
    // Initialize WiFi with clean state
    WiFi.persistent(true);  // Enable settings storage in flash
    WiFi.disconnect(true, true);  // Full disconnect
    WiFi.mode(WIFI_OFF);    // Turn off WiFi
    yield();
    delay(500);
    yield();
    
    // Reset WiFi subsystem
    esp_wifi_stop();
    delay(500);
    esp_wifi_deinit();
    delay(500);
    esp_wifi_init(NULL);
    delay(500);
    esp_wifi_start();
    delay(500);
    yield();
    
    // Ensure configuration is loaded and get credentials
    String ssid = config.getWiFiSSID();
    String password = config.getWiFiPassword();
    
    Serial.print("\n📱 WiFi SSID from config: ");
    Serial.println(ssid);
    
    if (ssid.length() > 0) {  // Only try to connect if we have an SSID
        // Register event handler first
        if (eventId) {
            WiFi.removeEvent(eventId);
        }
        eventId = WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
            this->onWiFiEvent(event);
        });
        
        // Try station mode
        WiFi.mode(WIFI_STA);  // Set mode before anything else
        yield();
        delay(100);
        yield();
        
        // Configure WiFi for stability
        esp_wifi_set_ps(WIFI_PS_NONE);  // Disable power saving
        WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Max power
        WiFi.setAutoReconnect(true);  // Enable auto-reconnect
        
        // Start connection
        Serial.print("\n📱 Connecting to WiFi: ");
        Serial.println(ssid);
        
        WiFi.begin(ssid.c_str(), password.c_str());
        connecting = true;
        connectStartTime = millis();
        lastCheck = millis();
        
        // Let the event handler manage the connection
        Serial.println("\nℹ Waiting for connection...");
    } else {
        // No credentials, go straight to AP mode
        startAP();
    }
}

void NetworkManager::startStation() {
    if (apMode) {
        // Stop AP mode and DNS server
        WiFi.softAPdisconnect(true);
        dnsServer.stop();
        apMode = false;
        delay(100);  // Wait for AP to stop
    }
    
    // Remove any existing event handler
    if (eventId) {
        WiFi.removeEvent(eventId);
    }
    
    // Get WiFi credentials once
    String ssid = config.getWiFiSSID();
    String password = config.getWiFiPassword();
    
    if (ssid.length() == 0) {  // Only check for empty SSID
        Serial.println("\n⚠ No WiFi credentials configured");
        Serial.println("📱 Please configure WiFi through the web interface");
        connecting = false;
        connected = false;
        startAP();  // Start AP mode immediately
        return;
    }
    
    // Configure WiFi station
    wifi_config_t conf;
    memset(&conf, 0, sizeof(conf));
    memcpy(conf.sta.ssid, ssid.c_str(), ssid.length());
    memcpy(conf.sta.password, password.c_str(), password.length());
    conf.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    esp_wifi_stop();
    delay(100);
    esp_wifi_set_config(WIFI_IF_STA, &conf);
    esp_wifi_start();
    delay(100);
    
    // Register WiFi event handler
    eventId = WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        this->onWiFiEvent(event);
    });

    Serial.print("\n📱 Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);  // Save WiFi settings to flash
    WiFi.setAutoReconnect(true);  // Enable auto-reconnect
    WiFi.begin(ssid.c_str(), password.c_str());
    
    connecting = true;
    connectStartTime = millis();
}

void NetworkManager::startAP() {
    Serial.println("\n📡 Starting Access Point mode...");
    
    // Clean stop of any existing mode
    WiFi.disconnect(true, true);  // Disconnect and clear settings
    WiFi.softAPdisconnect(true);
    delay(500);  // Longer delay for cleanup
    
    // Reset WiFi
    esp_wifi_stop();
    delay(500);
    esp_wifi_deinit();
    delay(500);
    esp_wifi_init(NULL);
    delay(500);
    esp_wifi_start();
    delay(500);
    
    // Start AP mode
    WiFi.mode(WIFI_AP);
    delay(500);
    
    // Generate AP name
    String apName = generateAPName();
    
    // Configure AP
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
        yield();  // Feed watchdog
        
        if (!apMode) {
            if (connecting && millis() - connectStartTime >= CONNECT_TIMEOUT) {
                Serial.println("\n❌ WiFi connection timeout");
                connecting = false;
                startAP();
            } else if (WiFi.status() == WL_CONNECTED) {
                // Check if we have a valid IP
                if (WiFi.localIP()[0] == 0) {
                    Serial.println("\n⚠ Invalid IP detected, reconnecting...");
                    connecting = true;
                    WiFi.disconnect(false);
                    yield();
                    WiFi.reconnect();
                }
            } else if (!connecting) {
                // Not connected and not trying to connect
                Serial.println("\n🔄 Connection lost, attempting to reconnect...");
                connecting = true;
                WiFi.reconnect();
            }
        }
        yield();  // Feed watchdog again
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
            if (WiFi.localIP()[0] != 0) {  // Check for valid IP
                Serial.println("\n✅ Connected to WiFi");
                Serial.print("📍 IP: ");
                Serial.println(WiFi.localIP());
                connected = true;
                connecting = false;
                connectStartTime = 0;  // Reset timeout counter
                
                // Configure time
                configTime(0, 0, "pool.ntp.org", "time.nist.gov");
                Serial.println("🕒 Synchronizing NTP time");
                
                // Wait for time to be set
                time_t now = time(nullptr);
                int retries = 0;
                while(now < 24*3600 && retries < 10) {
                    delay(500);
                    yield();
                    now = time(nullptr);
                    retries++;
                }
                
                if (now > 24*3600) {
                    Serial.println("✅ NTP time synchronized");
                } else {
                    Serial.println("⚠ NTP sync failed, continuing anyway");
                }
            } else {
                // Invalid IP, trigger reconnect
                Serial.println("\n⚠ Got invalid IP, retrying...");
                WiFi.disconnect(false);
                yield();
                WiFi.reconnect();
            }
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            connected = false;
            
            // Only try to reconnect if we're not in AP mode and not already connecting
            if (!apMode && !connecting) {
                // If we've been trying too long, switch to AP
                if (connectStartTime > 0 && millis() - connectStartTime > CONNECT_TIMEOUT) {
                    Serial.println("\n❌ WiFi connection timeout");
                    startAP();
                } else {
                    Serial.println("\n📱 Attempting to reconnect...");
                    connecting = true;
                    yield();
                    WiFi.disconnect(false);  // Disconnect but keep settings
                    yield();
                    delay(100);
                    yield();
                    WiFi.reconnect();
                }
            }
            break;
            
        case SYSTEM_EVENT_STA_START:
            WiFi.setAutoReconnect(true);
            break;
            
        case SYSTEM_EVENT_STA_STOP:
            connecting = false;  // Reset connecting state
            break;
            
        default:
            break;
    }
}

void NetworkManager::reconnect() {
    // Load current credentials
    String ssid = config.getWiFiSSID();
    
    Serial.println("\n🔄 Reconnecting with new WiFi settings...");
    Serial.print("📱 Using SSID: ");
    Serial.println(ssid);
    
    // Stop all WiFi activity
    WiFi.disconnect(true, true);  // Disconnect and clear settings
    WiFi.softAPdisconnect(true); // Stop AP if running
    WiFi.mode(WIFI_OFF);
    delay(500);  // Longer delay to ensure cleanup
    
    connected = false;
    connecting = false;
    apMode = false;
    
    // Save current config to flash
    WiFi.persistent(true);
    
    // Remove existing event handler
    if (eventId) {
        WiFi.removeEvent(eventId);
        eventId = 0;
    }
    
    // Only try to connect if we have valid credentials
    if (ssid.length() > 0) {
        // Start new connection attempt
        begin();  // Use full initialization sequence
    } else {
        Serial.println("\n⚠ No WiFi credentials configured");
        Serial.println("📱 Please configure WiFi through the web interface");
        startAP();
    }
}
