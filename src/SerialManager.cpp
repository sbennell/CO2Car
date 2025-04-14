#include "SerialManager.h"
#include "Version.h"

SerialManager::SerialManager(TimeManager& timeManager, Configuration& config)
    : _timeManager(timeManager), _config(config) {
}

void SerialManager::begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    Serial.println("\n--- CO₂ Car Race Timer Serial Interface ---");
    Serial.println("Version: " + String(VERSION_STRING));
    Serial.println("Ready to communicate with Race Management System");
}

void SerialManager::processCommands() {
    if (Serial.available()) {
        String jsonString = Serial.readStringUntil('\n');
        
        // Parse JSON command
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, jsonString);
        
        if (error) {
            Serial.print("Error parsing JSON: ");
            Serial.println(error.c_str());
            return;
        }
        
        // Process command
        const char* cmd = doc["cmd"];
        
        if (cmd) {
            if (strcmp(cmd, "status") == 0) {
                sendStatus();
            }
            else if (strcmp(cmd, "start_race") == 0) {
                // Extract heat_id if provided
                unsigned long heatId = 0;
                if (doc.containsKey("heat_id")) {
                    heatId = doc["heat_id"];
                }
                
                // Trigger race start (this will be implemented in main.cpp)
                extern void triggerRaceStart(unsigned long heatId);
                triggerRaceStart(heatId);
            }
            else if (strcmp(cmd, "reset_timer") == 0) {
                // Reset the race timer (this will be implemented in main.cpp)
                extern void resetRaceTimer();
                resetRaceTimer();
            }
            else if (strcmp(cmd, "calibrate") == 0) {
                // Calibrate sensors (this will be implemented in main.cpp)
                extern void calibrateSensors();
                calibrateSensors();
            }
            else {
                Serial.print("Unknown command: ");
                Serial.println(cmd);
            }
        }
    }
    
    // Send status periodically
    if (millis() - _lastStatusTime > STATUS_INTERVAL) {
        sendStatus();
        _lastStatusTime = millis();
    }
}

void SerialManager::sendStatus() {
    StaticJsonDocument<512> doc;
    
    // Set message type
    doc["type"] = "status";
    doc["timestamp"] = _timeManager.getEpochTime();
    
    // Add status information
    createStatusJson(doc);
    
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println(jsonString);
    
    // Debug output to internal serial monitor only when using USB
    #ifdef DEBUG
    Serial.print("Status update sent: ");
    Serial.println(jsonString.length());
    #endif
}

void SerialManager::sendRaceStart(unsigned long heatId) {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "race_start";
    doc["heat_id"] = heatId;
    doc["timestamp"] = _timeManager.getEpochTime();
    
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println(jsonString);
}

void SerialManager::sendRaceFinish(unsigned long heatId, unsigned long car1Time, unsigned long car2Time, const char* winner) {
    StaticJsonDocument<512> doc;
    
    doc["type"] = "race_finish";
    doc["heat_id"] = heatId;
    doc["timestamp"] = _timeManager.getEpochTime();
    
    JsonObject results = doc.createNestedObject("results");
    
    JsonObject lane1 = results.createNestedObject("lane1");
    lane1["time"] = car1Time;
    lane1["finished"] = (car1Time > 0);
    
    JsonObject lane2 = results.createNestedObject("lane2");
    lane2["time"] = car2Time;
    lane2["finished"] = (car2Time > 0);
    
    if (winner && strlen(winner) > 0) {
        results["winner"] = winner;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println(jsonString);
}

void SerialManager::sendSensorReadings(int lane1Distance, bool lane1Detected, int lane2Distance, bool lane2Detected) {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "sensor_reading";
    doc["timestamp"] = _timeManager.getEpochTime();
    
    JsonObject lane1 = doc.createNestedObject("lane1");
    lane1["distance"] = lane1Distance;
    lane1["detected"] = lane1Detected;
    
    JsonObject lane2 = doc.createNestedObject("lane2");
    lane2["distance"] = lane2Distance;
    lane2["detected"] = lane2Detected;
    
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println(jsonString);
}

void SerialManager::sendError(const char* message) {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "error";
    doc["timestamp"] = _timeManager.getEpochTime();
    doc["message"] = message;
    
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println(jsonString);
}

void SerialManager::createStatusJson(JsonDocument& doc) {
    // Basic info
    doc["version"] = VERSION_STRING;
    doc["uptime"] = millis() / 1000; // Uptime in seconds
    
    // WiFi status
    JsonObject wifi = doc.createNestedObject("wifi");
    extern bool wifiConnected;
    extern String wifiSSID;
    extern int wifiRSSI;
    
    wifi["connected"] = wifiConnected;
    if (wifiConnected) {
        wifi["ssid"] = wifiSSID;
        wifi["rssi"] = wifiRSSI;
    }
    
    // Sensor status
    JsonObject sensors = doc.createNestedObject("sensors");
    extern bool sensor1Ok;
    extern bool sensor2Ok;
    
    sensors["ok"] = (sensor1Ok && sensor2Ok);
    sensors["sensor1_ok"] = sensor1Ok;
    sensors["sensor2_ok"] = sensor2Ok;
    
    // Relay status
    JsonObject relay = doc.createNestedObject("relay");
    extern bool relayOk;
    
    relay["ok"] = relayOk;
    
    // Race status
    JsonObject race = doc.createNestedObject("race");
    extern bool raceStarted;
    extern bool carsLoaded;
    extern bool car1Finished;
    extern bool car2Finished;
    
    if (!carsLoaded) {
        race["status"] = "waiting";
    } else if (carsLoaded && !raceStarted) {
        race["status"] = "ready";
    } else if (raceStarted && (!car1Finished || !car2Finished)) {
        race["status"] = "racing";
    } else {
        race["status"] = "finished";
    }
}
