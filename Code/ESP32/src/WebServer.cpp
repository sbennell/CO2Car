#include "WebServer.h"

WebServer::WebServer() : server(80), ws("/ws"), commandHandler(nullptr) {}

void WebServer::begin() {
    if (!LittleFS.begin()) {
        Serial.println("❌ Error mounting LittleFS, trying to format...");
        if (!LittleFS.format()) {
            Serial.println("❌ Error formatting LittleFS");
            return;
        }
        if (!LittleFS.begin()) {
            Serial.println("❌ Error mounting LittleFS after format");
            return;
        }
        Serial.println("✅ LittleFS formatted and mounted successfully");
    }

    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    
    server.addHandler(&ws);
    setupRoutes();
    server.begin();
    Serial.println("✅ Web server started");
}

void WebServer::setupRoutes() {
    // Serve static files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}

void WebServer::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                               AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("📱 WebSocket client #%u connected\n", client->id());
            clients.push_back(client);
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("📱 WebSocket client #%u disconnected\n", client->id());
            clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
            break;
            
        case WS_EVT_DATA:
            if (len) {
                data[len] = 0;
                handleWebSocketMessage(client, (char*)data);
            }
            break;
            
        default:
            break;
    }
}

void WebServer::handleWebSocketMessage(AsyncWebSocketClient *client, const char *data) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    
    if (error) {
        Serial.println("❌ Error parsing WebSocket message");
        return;
    }
    
    const char* command = doc["command"];
    if (command && commandHandler) {
        commandHandler(command);
    }
}

void WebServer::notifyStatus(const char* status) {
    StaticJsonDocument<200> doc;
    doc["type"] = "status";
    doc["status"] = status;
    broadcastJson(doc);
}

void WebServer::notifySensorStates(bool sensor1, bool sensor2) {
    StaticJsonDocument<200> doc;
    doc["type"] = "sensors";
    doc["sensor1"] = sensor1;
    doc["sensor2"] = sensor2;
    broadcastJson(doc);
}

void WebServer::notifyTimes(float lane1, float lane2) {
    StaticJsonDocument<200> doc;
    doc["type"] = "times";
    doc["lane1"] = lane1;
    doc["lane2"] = lane2;
    broadcastJson(doc);
}

void WebServer::notifyRaceComplete(float lane1, float lane2) {
    StaticJsonDocument<200> doc;
    doc["type"] = "race_complete";
    doc["lane1"] = lane1;
    doc["lane2"] = lane2;
    doc["winner"] = (lane1 < lane2) ? 1 : 2;
    broadcastJson(doc);
}

void WebServer::broadcastJson(const JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    
    for (auto client : clients) {
        if (client->status() == WS_CONNECTED) {
            client->text(output);
        }
    }
}

void WebServer::setCommandHandler(CommandHandler handler) {
    commandHandler = handler;
}
