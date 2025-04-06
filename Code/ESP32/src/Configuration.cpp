#include "Configuration.h"

const char* Configuration::CONFIG_FILE = "/config.json";

Configuration::Configuration() :
    wifiSSID("Network"),
    wifiPassword("Had2much!"),
    sensorThreshold(150),
    relayActivationTime(250),
    tieThreshold(0.002)
{}

void Configuration::begin() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println("📄 Creating new configuration file...");
        saveToFile();
    } else {
        loadFromFile();
    }
}

void Configuration::setWiFiCredentials(const String& ssid, const String& password) {
    wifiSSID = ssid;
    wifiPassword = password;
    save();
}

void Configuration::setSensorThreshold(int threshold) {
    sensorThreshold = threshold;
    save();
}

void Configuration::setRelayActivationTime(int ms) {
    relayActivationTime = ms;
    save();
}

void Configuration::setTieThreshold(float seconds) {
    tieThreshold = seconds;
    save();
}



void Configuration::save() {
    saveToFile();
}

void Configuration::loadFromFile() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("❌ Failed to open config file for reading");
        return;
    }
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("❌ Failed to parse config file: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Load WiFi settings
    wifiSSID = doc["wifi"]["ssid"] | wifiSSID;
    wifiPassword = doc["wifi"]["password"] | wifiPassword;
    
    // Load sensor settings
    sensorThreshold = doc["sensor"]["threshold"] | sensorThreshold;
    
    // Load race timing parameters
    relayActivationTime = doc["timing"]["relay_ms"] | relayActivationTime;
    tieThreshold = doc["timing"]["tie_threshold"] | tieThreshold;

    
    Serial.println("✅ Configuration loaded");
}

void Configuration::saveToFile() {
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("❌ Failed to open config file for writing");
        return;
    }
    
    StaticJsonDocument<512> doc;
    
    // Save WiFi settings
    doc["wifi"]["ssid"] = wifiSSID;
    doc["wifi"]["password"] = wifiPassword;
    
    // Save sensor settings
    doc["sensor"]["threshold"] = sensorThreshold;
    
    // Save race timing parameters
    doc["timing"]["relay_ms"] = relayActivationTime;
    doc["timing"]["tie_threshold"] = tieThreshold;

    
    if (serializeJson(doc, file) == 0) {
        Serial.println("❌ Failed to write config file");
    } else {
        Serial.println("✅ Configuration saved");
    }
    
    file.close();
}
