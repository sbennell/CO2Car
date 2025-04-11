#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class WifiManager {
public:
    static WifiManager& getInstance() {
        static WifiManager instance;
        return instance;
    }

    bool begin(const char* ssid = nullptr, const char* password = nullptr);
    void handleClient();
    void broadcastRaceState(const char* state);
    void broadcastRaceTime(uint32_t elapsedTime, uint16_t lane1Time, uint16_t lane2Time);
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    
private:
    WifiManager() {}
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    
    void setupWebServer();
    void setupWebSocket();
    void handleWebSocketMessage(AsyncWebSocketClient* client, const char* data);
    
    static void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                               AwsEventType type, void* arg, uint8_t* data, size_t len);
};

#endif // WIFI_MANAGER_H
