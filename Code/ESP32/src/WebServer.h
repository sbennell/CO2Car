#pragma once

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

// Function pointer type for command handler
typedef void (*CommandHandler)(const char* command);

class WebServer {
public:
    WebServer();
    void begin();
    void handleWebSocketMessage(AsyncWebSocketClient *client, const char *data);
    void notifyStatus(const char* status);
    void notifySensorStates(bool sensor1, bool sensor2);
    void notifyTimes(float lane1, float lane2);
    void notifyRaceComplete(float lane1, float lane2);
    void setCommandHandler(CommandHandler handler);
    
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    std::vector<AsyncWebSocketClient*> clients;
    CommandHandler commandHandler;
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
    void setupRoutes();
    void broadcastJson(const JsonDocument& doc);
};
