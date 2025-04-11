#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Forward declarations
class ConfigManager;

class WifiManager {
public:
    static WifiManager& getInstance() {
        static WifiManager instance;
        return instance;
    }
    
    bool begin(const char* ssid, const char* password, bool apMode = false);
    void broadcastStatus();
    void broadcastRaceState(const char* state);
    void broadcastRaceTime(uint32_t elapsedTime, uint16_t lane1Time, uint16_t lane2Time);
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    
private:
    static constexpr const char* LOG_TAG = "WIFI_MANAGER";
    static constexpr uint32_t WIFI_CONNECT_TIMEOUT = 10000;  // 10 seconds
    
    AsyncWebServer* server;
    AsyncWebSocket* ws;

    WifiManager() : 
        server(new AsyncWebServer(80)), 
        ws(new AsyncWebSocket("/ws")) {}
    
    ~WifiManager() {
        if (ws) {
            ws->closeAll();
            delete ws;
        }
        if (server) {
            delete server;
        }
    }
    
    // Prevent copying
    WifiManager(const WifiManager&) = delete;
    WifiManager& operator=(const WifiManager&) = delete;
    
    void setupWebServer();
    void setupWebSocket();
    void handleWebSocketMessage(AsyncWebSocketClient* client, const char* data);
    
    static void onWebSocketEvent(AsyncWebSocket* server,
                               AsyncWebSocketClient* client,
                               AwsEventType type,
                               void* arg,
                               uint8_t* data,
                               size_t len);
};

#endif // WIFI_MANAGER_H
