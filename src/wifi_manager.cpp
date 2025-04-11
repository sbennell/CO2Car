#include "wifi_manager.h"
#include <esp_log.h>

#define LOG_TAG "WIFI_MANAGER"
#define WIFI_CONNECT_TIMEOUT 20000  // 20 seconds timeout for WiFi connection

bool WifiManager::begin(const char* ssid, const char* password) {
    if (!SPIFFS.begin(true)) {
        ESP_LOGE(LOG_TAG, "Failed to mount SPIFFS");
        return false;
    }

    // Start WiFi
    if (ssid && password) {
        WiFi.begin(ssid, password);
    } else {
        // Default to AP mode if no credentials
        WiFi.softAP("RaceTimer-AP", "racetimer123");
        ESP_LOGI(LOG_TAG, "AP Mode - SSID: RaceTimer-AP, Password: racetimer123");
        setupWebServer();
        return true;
    }

    // Wait for connection with timeout
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT) {
        delay(100);
    }

    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(LOG_TAG, "Failed to connect to WiFi");
        // Fall back to AP mode
        WiFi.disconnect();
        WiFi.softAP("RaceTimer-AP", "racetimer123");
        ESP_LOGI(LOG_TAG, "Fallback to AP Mode - SSID: RaceTimer-AP, Password: racetimer123");
    } else {
        ESP_LOGI(LOG_TAG, "Connected to WiFi");
        ESP_LOGI(LOG_TAG, "IP: %s", WiFi.localIP().toString().c_str());
    }

    setupWebServer();
    return true;
}

void WifiManager::setupWebServer() {
    // Setup WebSocket
    setupWebSocket();
    
    // Serve static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    
    // API endpoints
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["status"] = "ok";
        doc["version"] = "0.11.1";
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    ESP_LOGI(LOG_TAG, "Web server started");
}

void WifiManager::setupWebSocket() {
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
}

void WifiManager::handleWebSocketMessage(AsyncWebSocketClient* client, const char* data) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    
    if (error) {
        ESP_LOGE(LOG_TAG, "WebSocket JSON parse failed");
        return;
    }

    // Handle incoming WebSocket messages here
    // Example: {"command": "reset"}
    const char* command = doc["command"];
    if (command) {
        // Process commands
    }
}

void WifiManager::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                 AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            ESP_LOGI(LOG_TAG, "WebSocket client connected");
            break;
        case WS_EVT_DISCONNECT:
            ESP_LOGI(LOG_TAG, "WebSocket client disconnected");
            break;
        case WS_EVT_DATA:
            if (len) {
                char* message = new char[len + 1];
                memcpy(message, data, len);
                message[len] = '\0';
                WifiManager::getInstance().handleWebSocketMessage(client, message);
                delete[] message;
            }
            break;
        default:
            break;
    }
}

void WifiManager::broadcastRaceState(const char* state) {
    if (ws.count() == 0) return;
    
    StaticJsonDocument<200> doc;
    doc["type"] = "state";
    doc["data"] = state;
    
    String message;
    serializeJson(doc, message);
    ws.textAll(message);
}

void WifiManager::broadcastRaceTime(uint32_t elapsedTime, uint16_t lane1Time, uint16_t lane2Time) {
    if (ws.count() == 0) return;
    
    StaticJsonDocument<200> doc;
    doc["type"] = "time";
    doc["elapsed"] = elapsedTime;
    doc["lane1"] = lane1Time;
    doc["lane2"] = lane2Time;
    
    String message;
    serializeJson(doc, message);
    ws.textAll(message);
}
