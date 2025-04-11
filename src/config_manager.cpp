#include "config_manager.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "esp_log.h"

static const char* LOG_TAG = "CONFIG_MGR";

const char* ConfigManager::CONFIG_FILE = "/config.json";

bool ConfigManager::begin() {
    if (!SPIFFS.begin(true)) {
        log_e("Failed to mount SPIFFS");
        return false;
    }
    
    if (!loadConfig()) {
        log_w("Failed to load config, using defaults");
        saveConfig(); // Create initial config file
    }
    
    return true;
}

void ConfigManager::setupWebHandlers(AsyncWebServer& server) {
    // Config page handler
    server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/config.html", "text/html");
    });

    // Config API endpoints
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        StaticJsonDocument<1024> doc;
        serializeConfig(doc);
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/config/network", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("mode", true) || !request->hasParam("ssid", true)) {
            request->send(400, "text/plain", "Missing required parameters");
            return;
        }

        NetworkConfig newConfig;
        newConfig.wifiMode = request->getParam("mode", true)->value();
        newConfig.ssid = request->getParam("ssid", true)->value();
        newConfig.password = request->hasParam("password", true) ? 
                            request->getParam("password", true)->value() : "";
        
        if (newConfig.wifiMode == "ap") {
            newConfig.apSsid = request->hasParam("ap_ssid", true) ? 
                               request->getParam("ap_ssid", true)->value() : "RaceTimer-AP";
            newConfig.apPassword = request->hasParam("ap_password", true) ? 
                                  request->getParam("ap_password", true)->value() : "racetimer123";
        }

        if (updateNetworkConfig(newConfig)) {
            request->send(200, "text/plain", "Network configuration updated");
        } else {
            request->send(500, "text/plain", "Failed to update network configuration");
        }
    });
}

void ConfigManager::setDefaults() {
    // Network defaults - start in AP mode for initial setup
    networkConfig.wifiMode = "ap";
    networkConfig.ssid = "";
    networkConfig.password = "";
    networkConfig.apSsid = "RaceTimer-AP";
    networkConfig.apPassword = "racetimer123";
    
    // Sensor defaults
    sensorConfig.detectionThreshold = 150;  // 150mm
    sensorConfig.samplingRate = 50;         // 50Hz
    
    // Race defaults
    raceConfig.relayDuration = 250;         // 250ms
    raceConfig.tieThreshold = 2;            // 2ms
    
    // System defaults
    systemConfig.ntpEnabled = true;
    systemConfig.timezone = "AEST-10";
    systemConfig.logLevel = "INFO";
    
    ESP_LOGI("CONFIG", "Setting default configuration:");
    ESP_LOGI("CONFIG", "  WiFi Mode: %s", networkConfig.wifiMode.c_str());
    ESP_LOGI("CONFIG", "  AP SSID: %s", networkConfig.apSsid.c_str());
    ESP_LOGI("CONFIG", "  AP Password: %s", networkConfig.apPassword.c_str());
}

bool ConfigManager::loadConfig() {
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        return false;
    }
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        log_e("Failed to parse config file: %s", error.c_str());
        return false;
    }
    
    return deserializeConfig(doc);
}

bool ConfigManager::saveConfig() {
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        return false;
    }
    
    StaticJsonDocument<1024> doc;
    serializeConfig(doc);
    
    if (serializeJson(doc, file) == 0) {
        log_e("Failed to write config file");
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

void ConfigManager::serializeConfig(JsonDocument& doc) {
    // Network config
    JsonObject network = doc.createNestedObject("network");
    network["mode"] = networkConfig.wifiMode;
    network["ssid"] = networkConfig.ssid;
    network["password"] = networkConfig.password;
    network["ap_ssid"] = networkConfig.apSsid;
    network["ap_password"] = networkConfig.apPassword;
    
    // Sensor config
    JsonObject sensor = doc.createNestedObject("sensor");
    sensor["threshold"] = sensorConfig.detectionThreshold;
    sensor["rate"] = sensorConfig.samplingRate;
    
    // Race config
    JsonObject race = doc.createNestedObject("race");
    race["relay_duration"] = raceConfig.relayDuration;
    race["tie_threshold"] = raceConfig.tieThreshold;
    
    // System config
    JsonObject system = doc.createNestedObject("system");
    system["ntp"] = systemConfig.ntpEnabled;
    system["timezone"] = systemConfig.timezone;
    system["log_level"] = systemConfig.logLevel;
}

bool ConfigManager::deserializeConfig(JsonDocument& doc) {
    // Network config
    if (doc.containsKey("network")) {
        JsonObject network = doc["network"];
        networkConfig.wifiMode = network["mode"] | "station";
        networkConfig.ssid = network["ssid"] | "";
        networkConfig.password = network["password"] | "";
        networkConfig.apSsid = network["ap_ssid"] | "RaceTimer-AP";
        networkConfig.apPassword = network["ap_password"] | "racetimer123";
    }
    
    // Sensor config
    if (doc.containsKey("sensor")) {
        JsonObject sensor = doc["sensor"];
        sensorConfig.detectionThreshold = sensor["threshold"] | 150;
        sensorConfig.samplingRate = sensor["rate"] | 50;
    }
    
    // Race config
    if (doc.containsKey("race")) {
        JsonObject race = doc["race"];
        raceConfig.relayDuration = race["relay_duration"] | 250;
        raceConfig.tieThreshold = race["tie_threshold"] | 2;
    }
    
    // System config
    if (doc.containsKey("system")) {
        JsonObject system = doc["system"];
        systemConfig.ntpEnabled = system["ntp"] | true;
        systemConfig.timezone = system["timezone"] | "AEST-10";
        systemConfig.logLevel = system["log_level"] | "INFO";
    }
    
    return true;
}

void ConfigManager::handleWebSocketMessage(AsyncWebSocket* ws, AsyncWebSocketClient* client, 
                                        const char* message) {
    ESP_LOGD(LOG_TAG, "Received config message: %s", message);
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        ESP_LOGE(LOG_TAG, "Config JSON parse failed: %s", error.c_str());
        sendErrorResponse(client, String("Invalid JSON: ") + error.c_str());
        return;
    }
    
    const char* type = doc["type"];
    if (!type) {
        sendErrorResponse(client, "Missing 'type' field");
        return;
    }
    
    // Handle get config request
    if (strcmp(type, "getConfig") == 0) {
        const char* section = doc["section"];
        if (!section) {
            // Send complete config if no section specified
            StaticJsonDocument<1024> response;
            response["type"] = "config";
            serializeConfig(response);
            String output;
            serializeJson(response, output);
            client->text(output);
            return;
        }
        sendConfigResponse(client, section);
        return;
    }
    
    // Handle set config request
    if (strcmp(type, "setConfig") == 0) {
        const char* section = doc["section"];
        if (!section) {
            sendErrorResponse(client, "Missing 'section' field");
            return;
        }
        
        JsonObject data = doc["data"];
        if (data.isNull()) {
            sendErrorResponse(client, "Missing 'data' field");
            return;
        }
        
        bool success = false;
        String successMsg;
        
        if (strcmp(section, "network") == 0) {
            NetworkConfig newConfig;
            newConfig.wifiMode = data["mode"].as<const char*>() ?: networkConfig.wifiMode;
            
            if (strcmp(newConfig.wifiMode.c_str(), "station") == 0) {
                if (!data["ssid"] || !data["password"]) {
                    sendErrorResponse(client, "Station mode requires SSID and password");
                    return;
                }
                newConfig.ssid = data["ssid"].as<const char*>();
                newConfig.password = data["password"].as<const char*>();
            } else if (strcmp(newConfig.wifiMode.c_str(), "ap") == 0) {
                newConfig.apSsid = data["apSsid"].as<const char*>() ?: "RaceTimer-AP";
                newConfig.apPassword = data["apPassword"].as<const char*>() ?: "racetimer123";
            } else {
                sendErrorResponse(client, "Invalid WiFi mode");
                return;
            }
            
            success = updateNetworkConfig(newConfig);
            successMsg = "Network settings updated";
            
        } else if (strcmp(section, "sensor") == 0) {
            SensorConfig newConfig;
            newConfig.detectionThreshold = data["threshold"] | sensorConfig.detectionThreshold;
            newConfig.samplingRate = data["rate"] | sensorConfig.samplingRate;
            success = updateSensorConfig(newConfig);
            successMsg = "Sensor settings updated";
            
        } else if (strcmp(section, "race") == 0) {
            RaceConfig newConfig;
            newConfig.relayDuration = data["relayDuration"] | raceConfig.relayDuration;
            newConfig.tieThreshold = data["tieThreshold"] | raceConfig.tieThreshold;
            success = updateRaceConfig(newConfig);
            successMsg = "Race settings updated";
            
        } else if (strcmp(section, "system") == 0) {
            SystemConfig newConfig;
            newConfig.ntpEnabled = data["ntp"] | systemConfig.ntpEnabled;
            newConfig.timezone = data["timezone"] | systemConfig.timezone;
            newConfig.logLevel = data["logLevel"] | systemConfig.logLevel;
            success = updateSystemConfig(newConfig);
            successMsg = "System settings updated";
        } else {
            sendErrorResponse(client, "Invalid configuration section");
            return;
        }
        
        if (success) {
            sendSuccessResponse(client, successMsg);
            broadcastConfig(ws);
        } else {
            sendErrorResponse(client, "Failed to update configuration");
        }
        return;
    }
    
    sendErrorResponse(client, "Invalid message type");
}

void ConfigManager::broadcastConfig(AsyncWebSocket* ws) {
    StaticJsonDocument<1024> doc;
    serializeConfig(doc);
    
    String output;
    doc["type"] = "config";
    serializeJson(doc, output);
    
    ws->textAll(output);
}

void ConfigManager::sendConfigResponse(AsyncWebSocketClient* client, const String& section) {
    StaticJsonDocument<512> doc;
    doc["type"] = "config";
    doc["section"] = section;
    
    if (section == "network") {
        JsonObject data = doc.createNestedObject("data");
        data["wifiMode"] = networkConfig.wifiMode;
        data["ssid"] = networkConfig.ssid;
        data["password"] = networkConfig.password;
        data["apSsid"] = networkConfig.apSsid;
        data["apPassword"] = networkConfig.apPassword;
    }
    // Add other sections as needed
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

void ConfigManager::sendErrorResponse(AsyncWebSocketClient* client, const String& message) {
    StaticJsonDocument<128> doc;
    doc["type"] = "error";
    doc["message"] = message;
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

void ConfigManager::sendSuccessResponse(AsyncWebSocketClient* client, const String& message) {
    StaticJsonDocument<128> doc;
    doc["type"] = "success";
    doc["message"] = message;
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

// Configuration getters
ConfigManager::NetworkConfig ConfigManager::getNetworkConfig() {
    return networkConfig;
}

ConfigManager::SensorConfig ConfigManager::getSensorConfig() {
    return sensorConfig;
}

ConfigManager::RaceConfig ConfigManager::getRaceConfig() {
    return raceConfig;
}

ConfigManager::SystemConfig ConfigManager::getSystemConfig() {
    return systemConfig;
}

// Configuration setters
bool ConfigManager::updateNetworkConfig(const NetworkConfig& config) {
    networkConfig = config;
    if (!saveConfig()) {
        ESP_LOGE(LOG_TAG, "Failed to save network configuration");
        return false;
    }
    ESP_LOGI(LOG_TAG, "Network configuration saved successfully");
    return true;
}

bool ConfigManager::updateSensorConfig(const SensorConfig& config) {
    sensorConfig = config;
    return saveConfig();
}

bool ConfigManager::updateRaceConfig(const RaceConfig& config) {
    raceConfig = config;
    return saveConfig();
}

bool ConfigManager::updateSystemConfig(const SystemConfig& config) {
    systemConfig = config;
    return saveConfig();
}
