#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "ESPAsyncWebServer.h"

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

private:
    static constexpr const char* LOG_TAG = "CONFIG_MGR";
    ConfigManager() {} // Private constructor
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

public:
    struct NetworkConfig {
        String wifiMode;     // "station" or "ap"
        String ssid;
        String password;
        String apSsid;
        String apPassword;
    };

    struct SensorConfig {
        uint16_t detectionThreshold;  // in mm
        uint8_t samplingRate;         // in Hz
    };

    struct RaceConfig {
        uint16_t relayDuration;       // in ms
        uint8_t tieThreshold;         // in ms
    };

    struct SystemConfig {
        bool ntpEnabled;
        String timezone;
        String logLevel;
    };

    bool begin();
    void setupWebHandlers(AsyncWebServer& server);
    
    // Configuration getters
    NetworkConfig getNetworkConfig();
    SensorConfig getSensorConfig();
    RaceConfig getRaceConfig();
    SystemConfig getSystemConfig();
    
    // Configuration setters
    bool updateNetworkConfig(const NetworkConfig& config);
    bool updateSensorConfig(const SensorConfig& config);
    bool updateRaceConfig(const RaceConfig& config);
    bool updateSystemConfig(const SystemConfig& config);
    
    // WebSocket handlers
    void handleWebSocketMessage(AsyncWebSocket* ws, AsyncWebSocketClient* client, 
                              const char* message);
    void broadcastConfig(AsyncWebSocket* ws);

private:
    static const char* CONFIG_FILE;
    NetworkConfig networkConfig;
    SensorConfig sensorConfig;
    RaceConfig raceConfig;
    SystemConfig systemConfig;
    
    bool loadConfig();
    bool saveConfig();
    void setDefaults();
    
    // JSON helpers
    void serializeConfig(JsonDocument& doc);
    bool deserializeConfig(JsonDocument& doc);
    void sendConfigResponse(AsyncWebSocketClient* client, const String& section);
    void sendErrorResponse(AsyncWebSocketClient* client, const String& message);
    void sendSuccessResponse(AsyncWebSocketClient* client, const String& message);
};

// ConfigManager is now a singleton, use ConfigManager::getInstance()

#endif // CONFIG_MANAGER_H
