#pragma once

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "RaceHistory.h"
#include "Version.h"
#include "Configuration.h"

// Function pointer type for command handler
typedef void (*CommandHandler)(const char* command);

class WebServer {
public:
    WebServer(TimeManager& timeManager, Configuration& config);
    void begin();
    void handleWebSocketMessage(AsyncWebSocketClient *client, const char *data);
    void notifyStatus(const char* status);
    void notifySensorStates(bool sensor1, bool sensor2);
    void notifyTimes(float lane1, float lane2);
    void notifyRaceComplete(float lane1, float lane2);
    void sendVersionInfo(AsyncWebSocketClient *client);
    void setCommandHandler(CommandHandler handler);
    
private:
    AsyncWebServer server;
    TimeManager& timeManager;
    AsyncWebSocket ws;
    std::vector<AsyncWebSocketClient*> clients;
    CommandHandler commandHandler;
    RaceHistory raceHistory;  // Will be initialized in constructor
    Configuration& config;
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
    void setupRoutes();
    void broadcastJson(const JsonDocument& doc);
    void sendRaceHistory(AsyncWebSocketClient *client);
};
