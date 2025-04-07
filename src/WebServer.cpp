#include "WebServer.h"
#include "Version.h"

WebServer::WebServer(TimeManager& tm, Configuration& cfg, NetworkManager& nm) 
    : server(80), ws("/ws"), commandHandler(nullptr), 
      timeManager(tm), raceHistory(tm), config(cfg), networkManager(nm) {}

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

    // Handle configuration page
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("📄 Serving config.html");
        if (LittleFS.exists("/config.html")) {
            Serial.println("✅ Found config.html");
            request->send(LittleFS, "/config.html", "text/html");
        } else {
            Serial.println("❌ config.html not found");
            request->send(404, "text/plain", "config.html not found");
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

void WebServer::sendVersionInfo(AsyncWebSocketClient *client) {
    StaticJsonDocument<200> doc;
    doc["type"] = "version";
    doc["version"] = VERSION_STRING;
    doc["buildDate"] = BUILD_DATE;
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

void WebServer::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                               AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            Serial.printf("🔗 WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            
            // Add client to our list
            clients.push_back(client);
            
            // Send initial configuration
            sendVersionInfo(client);
            sendRaceHistory(client);
            sendNetworkInfo(client);
            break;
        }
            
        case WS_EVT_DISCONNECT: {
            Serial.printf("🔕 WebSocket client #%u disconnected\n", client->id());
            
            // Remove client from our list
            clients.erase(std::remove_if(clients.begin(), clients.end(),
                [client](AsyncWebSocketClient* c) { return c->id() == client->id(); }),
                clients.end());
            break;
        }
            
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                // Ensure null termination
                char *message = new char[len + 1];
                memcpy(message, data, len);
                message[len] = '\0';
                
                // Process the message
                handleWebSocketMessage(client, message);
                
                // Clean up
                delete[] message;
            }
            break;
        }
            
        case WS_EVT_ERROR:
            Serial.printf("❌ WebSocket client #%u error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
            break;
    }
}

void WebServer::handleWebSocketMessage(AsyncWebSocketClient *client, const char *data) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, data);
    
    if (error) {
        Serial.print("❌ Error parsing WebSocket message: ");
        Serial.println(error.c_str());
        Serial.print("Message was: ");
        Serial.println(data);
        return;
    }
    
    const char* command = doc["command"];
    if (!command) {
        Serial.println("❌ No command in message");
        return;
    }

    if (strcmp(command, "get_network_status") == 0) {
        sendNetworkInfo(client);
    }
    else if (strcmp(command, "get_config") == 0) {
        StaticJsonDocument<512> configDoc;
        configDoc["type"] = "config";
        JsonObject wifi = configDoc.createNestedObject("wifi");
        wifi["ssid"] = config.getWiFiSSID();
        wifi["password"] = config.getWiFiPassword();
        
        JsonObject sensor = configDoc.createNestedObject("sensor");
        sensor["threshold"] = config.getSensorThreshold();
        
        JsonObject timing = configDoc.createNestedObject("timing");
        timing["relay_ms"] = config.getRelayActivationTime();
        timing["tie_threshold"] = config.getTieThreshold();
        
        String output;
        serializeJson(configDoc, output);
        client->text(output);
    }
    else if (strcmp(command, "set_config") == 0) {
        const char* section = doc["section"];
        if (!section) {
            Serial.println("❌ No section in set_config message");
            return;
        }
        JsonObject data = doc["data"];
        if (data.isNull()) {
            Serial.println("❌ No data in set_config message");
            return;
        }
        
        if (strcmp(section, "wifi") == 0) {
            config.setWiFiCredentials(data["ssid"], data["password"]);
            networkManager.reconnect();
            
            // Send success response
            StaticJsonDocument<64> response;
            response["type"] = "config_saved";
            String output;
            serializeJson(response, output);
            client->text(output);
        }
        else if (strcmp(section, "sensor") == 0) {
            config.setSensorThreshold(data["threshold"]);
        }
        else if (strcmp(section, "timing") == 0) {
            config.setRelayActivationTime(data["relay_ms"]);
            config.setTieThreshold(data["tie_threshold"]);
        }
        
        // Send success response
        StaticJsonDocument<64> response;
        response["type"] = "config_saved";
        String output;
        serializeJson(response, output);
        client->text(output);
    }
    else if (strcmp(command, "get_history") == 0) {
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
    else if (strcmp(command, "load") == 0 || strcmp(command, "start") == 0) {
        commandHandler(command);
    }
    else {
        Serial.print("❌ Unknown command: ");
        Serial.println(command);
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
    // Consider times within 0.002 seconds (2ms) as a tie
    float timeDiff = abs(lane1 - lane2);
    if (timeDiff <= 0.002) {
        // For ties, use the average of both times
        float avgTime = (lane1 + lane2) / 2;
        lane1 = lane2 = avgTime;
    }
    
    raceHistory.addRace(lane1, lane2);
    
    StaticJsonDocument<200> doc;
    doc["type"] = "race_complete";
    doc["lane1"] = lane1;
    doc["lane2"] = lane2;
    doc["winner"] = (timeDiff <= 0.002) ? 0 : (lane1 < lane2 ? 1 : 2);
    
    broadcastJson(doc);
}

void WebServer::broadcastJson(const JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    Serial.print("📣 Broadcasting: ");
    Serial.println(output);
    
    // Clean up disconnected clients first
    clients.erase(std::remove_if(clients.begin(), clients.end(),
        [](AsyncWebSocketClient* c) { return c->status() != WS_CONNECTED; }),
        clients.end());
    
    // Send to all connected clients
    for (auto client : clients) {
        client->text(output);
    }
}

void WebServer::setCommandHandler(CommandHandler handler) {
    commandHandler = handler;
}

void WebServer::sendNetworkInfo(AsyncWebSocketClient *client) {
    StaticJsonDocument<200> doc;
    doc["type"] = "network_status";
    doc["mode"] = networkManager.isAPMode() ? "AP" : "Station";
    doc["connected"] = networkManager.isConnected();
    doc["ssid"] = networkManager.getSSID();
    doc["ip"] = networkManager.getIP();
    doc["rssi"] = networkManager.getRSSI();
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

void WebServer::notifyNetworkStatus() {
    StaticJsonDocument<200> doc;
    doc["type"] = "network_status";
    doc["mode"] = networkManager.isAPMode() ? "AP" : "Station";
    doc["connected"] = networkManager.isConnected();
    doc["ssid"] = networkManager.getSSID();
    doc["ip"] = networkManager.getIP();
    doc["rssi"] = networkManager.getRSSI();
    
    broadcastJson(doc);
}
