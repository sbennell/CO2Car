#include "wifi_manager.h"
#include <esp_log.h>

#define LOG_TAG "WIFI_MANAGER"
#define WIFI_CONNECT_TIMEOUT 20000  // 20 seconds timeout for WiFi connection

bool WifiManager::begin(const char* ssid, const char* password, bool apMode) {
    if (!SPIFFS.begin(true)) {
        ESP_LOGE(LOG_TAG, "Failed to mount SPIFFS");
        return false;
    }

    // Disconnect and clean up existing WiFi connections
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    delay(100);

    // Start WiFi
    if (apMode) {
        ESP_LOGI(LOG_TAG, "Starting AP Mode...");
        WiFi.mode(WIFI_AP);
        delay(100);
        
        if (!WiFi.softAP(ssid, password)) {
            ESP_LOGE(LOG_TAG, "Failed to start AP mode");
            return false;
        }
        ESP_LOGI(LOG_TAG, "AP Mode - SSID: %s", ssid);
        ESP_LOGI(LOG_TAG, "AP IP: %s", WiFi.softAPIP().toString().c_str());
    } else {
        ESP_LOGI(LOG_TAG, "Starting Station Mode...");
        WiFi.mode(WIFI_STA);
        delay(100);
        
        ESP_LOGI(LOG_TAG, "Connecting to %s...", ssid);
        WiFi.begin(ssid, password);
        
        unsigned long startAttemptTime = millis();
        
        while (WiFi.status() != WL_CONNECTED && 
               millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            ESP_LOGD(LOG_TAG, "Connection status: %d", WiFi.status());
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGE(LOG_TAG, "Failed to connect to WiFi. Status: %d", WiFi.status());
            switch (WiFi.status()) {
                case WL_NO_SSID_AVAIL:
                    ESP_LOGE(LOG_TAG, "SSID not found");
                    break;
                case WL_CONNECT_FAILED:
                    ESP_LOGE(LOG_TAG, "Incorrect password");
                    break;
                case WL_IDLE_STATUS:
                    ESP_LOGE(LOG_TAG, "Idle - changing state");
                    break;
                default:
                    ESP_LOGE(LOG_TAG, "Unknown error");
            }
            return false;
        }
        
        ESP_LOGI(LOG_TAG, "Station Mode - Connected to: %s", ssid);
        ESP_LOGI(LOG_TAG, "IP: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(LOG_TAG, "Signal Strength (RSSI): %d dBm", WiFi.RSSI());
    }
    
    if (!server || !ws) {
        ESP_LOGE(LOG_TAG, "Server or WebSocket not initialized");
        return false;
    }
    
    setupWebServer();
    return true;
}

void WifiManager::setupWebServer() {
    if (!server || !ws) {
        ESP_LOGE(LOG_TAG, "Server or WebSocket not initialized");
        return;
    }
    
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });
    
    server->serveStatic("/", SPIFFS, "/");
    
    setupWebSocket();
    
    server->on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["status"] = "ok";
        doc["version"] = "0.11.1";
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server->begin();
    ESP_LOGI(LOG_TAG, "Web server started");
}

void WifiManager::setupWebSocket() {
    if (!server || !ws) {
        ESP_LOGE(LOG_TAG, "Server or WebSocket not initialized");
        return;
    }
    
    ws->onEvent(onWebSocketEvent);
    server->addHandler(ws);
}

void WifiManager::handleWebSocketMessage(AsyncWebSocketClient* client, const char* data) {
    if (!client || !data) {
        ESP_LOGE(LOG_TAG, "Invalid client or data");
        return;
    }
    ESP_LOGD(LOG_TAG, "Received WebSocket message: %s", data);
    
    // Skip empty messages or heartbeats
    if (!data || strlen(data) == 0 || strcmp(data, "ping") == 0) {
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, data);
    
    if (error) {
        ESP_LOGE(LOG_TAG, "WebSocket JSON parse failed: %s", error.c_str());
        return;
    }

    // All messages must have a type field
    const char* type = doc["type"];
    if (!type) {
        ESP_LOGE(LOG_TAG, "WebSocket message missing 'type' field");
        return;
    }

    if (strcmp(type, "request") == 0) {
        const char* requestType = doc["data"];
        if (!requestType) {
            ESP_LOGE(LOG_TAG, "Request missing 'data' field");
            return;
        }
        
        if (strcmp(requestType, "system_status") == 0) {
            broadcastStatus();
        }
    } else if (strcmp(type, "config") == 0) {
        const char* section = doc["section"];
        if (!section) {
            ESP_LOGE(LOG_TAG, "Config message missing 'section' field");
            return;
        }
        JsonObject data = doc["data"];
        if (data.isNull()) {
            ESP_LOGE(LOG_TAG, "Config message missing 'data' field");
            return;
        }
        
        if (strcmp(section, "network") == 0) {
            String wifiMode = data["wifiMode"] | "ap";
            String ssid = data["ssid"] | "";
            String password = data["password"] | "";
            String apSsid = data["apSsid"] | "RaceTimer-AP";
            String apPassword = data["apPassword"] | "racetimer123";
            
            ESP_LOGI(LOG_TAG, "Updating network config - Mode: %s", wifiMode.c_str());
            
            // Apply new WiFi settings
            bool apMode = (wifiMode == "ap");
            const char* useSSID = apMode ? apSsid.c_str() : ssid.c_str();
            const char* usePassword = apMode ? apPassword.c_str() : password.c_str();
            
            ESP_LOGI(LOG_TAG, "Applying new WiFi settings - Mode: %s, SSID: %s", 
                     apMode ? "AP" : "Station", useSSID);
            
            if (!begin(useSSID, usePassword, apMode)) {
                ESP_LOGE(LOG_TAG, "Failed to apply new WiFi settings");
                return;
            }
            
            // Send success response
            StaticJsonDocument<200> response;
            response["type"] = "success";
            response["message"] = "Network settings updated";
            String output;
            serializeJson(response, output);
            client->text(output);
            
            // Broadcast new status to all clients
            broadcastStatus();
        }
    }
}

void WifiManager::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                 AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            ESP_LOGI(LOG_TAG, "WebSocket client #%u connected from %s", 
                     client->id(), client->remoteIP().toString().c_str());
            // Send initial status
            StaticJsonDocument<200> status;
            status["type"] = "status";
            status["connected"] = WiFi.status() == WL_CONNECTED;
            status["mode"] = WiFi.getMode() == WIFI_MODE_AP ? "ap" : "station";
            if (WiFi.getMode() == WIFI_MODE_AP) {
                status["ip"] = WiFi.softAPIP().toString();
                status["ssid"] = WiFi.softAPSSID();
                status["stations"] = WiFi.softAPgetStationNum();
            } else if (WiFi.status() == WL_CONNECTED) {
                status["ip"] = WiFi.localIP().toString();
                status["ssid"] = WiFi.SSID();
                status["rssi"] = WiFi.RSSI();
            }
            String statusMsg;
            serializeJson(status, statusMsg);
            client->text(statusMsg);
            break;
        }
            
        case WS_EVT_DISCONNECT:
            ESP_LOGI(LOG_TAG, "WebSocket client #%u disconnected", client->id());
            break;
            
        case WS_EVT_DATA: {
            if (len) {
                AwsFrameInfo* info = (AwsFrameInfo*)arg;
                if (info->final && info->index == 0 && info->len == len) {
                    // Complete message received
                    char* message = new char[len + 1];
                    memcpy(message, data, len);
                    message[len] = '\0';
                    WifiManager::getInstance().handleWebSocketMessage(client, message);
                    delete[] message;
                }
            }
            break;
        }
            
        case WS_EVT_ERROR:
            ESP_LOGE(LOG_TAG, "WebSocket client #%u error %u", client->id(), *((uint16_t*)arg));
            break;
            
        default:
            break;
    }
}

void WifiManager::broadcastStatus() {
    if (!ws) {
        ESP_LOGE(LOG_TAG, "WebSocket not initialized");
        return;
    }
    
    StaticJsonDocument<200> doc;
    doc["type"] = "status";
    doc["connected"] = WiFi.status() == WL_CONNECTED;
    doc["mode"] = WiFi.getMode() == WIFI_MODE_AP ? "ap" : "station";
    
    if (WiFi.getMode() == WIFI_MODE_AP) {
        doc["ip"] = WiFi.softAPIP().toString();
        doc["ssid"] = WiFi.softAPSSID();
        doc["stations"] = WiFi.softAPgetStationNum();
    } else if (WiFi.status() == WL_CONNECTED) {
        doc["ip"] = WiFi.localIP().toString();
        doc["ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();
    }
    
    String output;
    serializeJson(doc, output);
    ws->textAll(output);
    
    ESP_LOGD(LOG_TAG, "Broadcast status update");
}

void WifiManager::broadcastRaceState(const char* state) {
    if (!ws) {
        ESP_LOGE(LOG_TAG, "WebSocket not initialized");
        return;
    }
    
    StaticJsonDocument<200> doc;
    doc["type"] = "race_state";
    doc["state"] = state;
    
    String output;
    serializeJson(doc, output);
    ws->textAll(output);
    
    ESP_LOGD(LOG_TAG, "Broadcast race state: %s", state);
}

void WifiManager::broadcastRaceTime(uint32_t elapsedTime, uint16_t lane1Time, uint16_t lane2Time) {
    if (!ws) {
        ESP_LOGE(LOG_TAG, "WebSocket not initialized");
        return;
    }
    
    StaticJsonDocument<200> doc;
    doc["type"] = "race_time";
    doc["elapsed"] = elapsedTime;
    doc["lane1"] = lane1Time;
    doc["lane2"] = lane2Time;
    
    String output;
    serializeJson(doc, output);
    ws->textAll(output);
    
    ESP_LOGD(LOG_TAG, "Broadcast race time - Elapsed: %u, Lane1: %u, Lane2: %u", 
             elapsedTime, lane1Time, lane2Time);
}
