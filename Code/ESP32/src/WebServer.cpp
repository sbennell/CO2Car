#include "WebServer.h"

WebServer::WebServer() : server(80), ws("/ws"), commandHandler(nullptr) {}

void WebServer::begin() {
    if (!LittleFS.begin(false)) {  // First try without formatting
        Serial.println("❌ Error mounting LittleFS, trying to format...");
        if (!LittleFS.format()) {
            Serial.println("❌ Error formatting LittleFS");
            return;
        }
        if (!LittleFS.begin(false)) {
            Serial.println("❌ Error mounting LittleFS after format");
            return;
        }
        Serial.println("✅ LittleFS formatted and mounted successfully");
    } else {
        Serial.println("✅ LittleFS mounted successfully");
    }

    // Create data directory if it doesn't exist
    if (!LittleFS.mkdir("/data")) {
        Serial.println("⚠ Warning: Failed to create /data directory (may already exist)");
    }
    
    raceHistory.begin();

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
    // Handle root path
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("📄 Serving index.html");
        if (LittleFS.exists("/index.html")) {
            Serial.println("✅ Found index.html");
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            Serial.println("❌ index.html not found");
            request->send(404, "text/plain", "index.html not found");
        }
    });

    // Handle favicon.ico
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204);
    });

    // Serve static files
    server.serveStatic("/", LittleFS, "/");

    // Handle 404s
    server.onNotFound([](AsyncWebServerRequest *request) {
        Serial.print("❌ 404 Not Found: ");
        Serial.println(request->url());
        String message = "File Not Found\n\n";
        message += "URI: " + request->url() + "\n";
        request->send(404, "text/plain", message);
    });
}

void WebServer::sendRaceHistory(AsyncWebSocketClient *client) {
    Serial.println("📄 Sending race history to client...");
    StaticJsonDocument<4096> historyDoc;
    StaticJsonDocument<4096> racesDoc;
    raceHistory.getHistory(racesDoc);
    
    historyDoc["type"] = "race_history";
    historyDoc["races"] = racesDoc.as<JsonArray>();
    
    String output;
    serializeJson(historyDoc, output);
    
    Serial.print("✅ Race history JSON: ");
    Serial.println(output);
    
    client->text(output);
}

void WebServer::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                               AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            Serial.printf("📱 WebSocket client #%u connected\n", client->id());
            clients.push_back(client);
            sendRaceHistory(client);
            break;
        }
            
        case WS_EVT_DISCONNECT: {
            Serial.printf("📱 WebSocket client #%u disconnected\n", client->id());
            clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
            break;
        }
            
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0;
                String message = (char*)data;
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, message);
                if (error) {
                    Serial.print("❌ deserializeJson() failed: ");
                    Serial.println(error.c_str());
                    return;
                }

                const char* command = doc["command"];
                if (strcmp(command, "test") == 0) {
                    // Send test response
                    DynamicJsonDocument response(1024);
                    response["type"] = "test_response";
                    response["message"] = "WebSocket is working!";
                    String jsonResponse;
                    serializeJson(response, jsonResponse);
                    client->text(jsonResponse);
                } else if (commandHandler) {
                    commandHandler(command);
                }
            }
            break;
        }
            
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
    if (strcmp(command, "get_history") == 0) {
        StaticJsonDocument<4096> historyDoc;
        StaticJsonDocument<4096> racesDoc;
        raceHistory.getHistory(racesDoc);
        historyDoc["type"] = "race_history";
        historyDoc["races"] = racesDoc.as<JsonArray>();
        String output;
        serializeJson(historyDoc, output);
        client->text(output);
    }
    else if (strcmp(command, "clear_history") == 0) {
        raceHistory.clear();
        StaticJsonDocument<64> response;
        response["type"] = "history_cleared";
        String output;
        serializeJson(response, output);
        client->text(output);
    }
    else if (command && commandHandler) {
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
    raceHistory.addRace(lane1, lane2);
    
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
