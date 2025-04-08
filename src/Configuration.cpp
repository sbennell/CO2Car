#include "Configuration.h"

const char* Configuration::CONFIG_FILE = "/config.json";

Configuration::Configuration() :
    wifiSSID(""),
    wifiPassword(""),
    sensorThreshold(150),
    relayActivationTime(250),
    tieThreshold(0.002)
{}

void Configuration::begin() {
    // Don't try to access files if LittleFS isn't mounted
    if (!LittleFS.begin(false)) {
        Serial.println("❌ LittleFS not mounted, using default configuration");
        return;
    }
    
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println("📄 Creating new configuration file...");
        saveToFile();
    } else {
        loadFromFile();
        // Verify loaded configuration
        Serial.print("📱 Verifying WiFi SSID: ");
        Serial.println(wifiSSID);
    }
    
    // Save configuration to ensure format is current
    saveToFile();
}

void Configuration::setWiFiCredentials(const String& ssid, const String& password) {
    wifiSSID = ssid;
    wifiPassword = password;
    saveToFile();  // Call saveToFile directly to ensure immediate persistence
    Serial.print("✅ WiFi credentials saved - SSID: ");
    Serial.println(wifiSSID);
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
        saveToFile();  // Create default config file
        return;
    }
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("❌ Failed to parse config file: ");
        Serial.println(error.c_str());
        saveToFile();  // Create new config file with current settings
        return;
    }
    
    // Load WiFi settings if they exist
    if (doc["wifi"].containsKey("ssid")) {
        wifiSSID = doc["wifi"]["ssid"].as<String>();
    }
    if (doc["wifi"].containsKey("password")) {
        wifiPassword = doc["wifi"]["password"].as<String>();
    }
    
    // Load sensor settings
    sensorThreshold = doc["sensor"]["threshold"] | sensorThreshold;
    
    // Load race timing parameters
    relayActivationTime = doc["timing"]["relay_ms"] | relayActivationTime;
    tieThreshold = doc["timing"]["tie_threshold"] | tieThreshold;

    Serial.println("✅ Configuration loaded");
    
    // Log loaded WiFi settings
    Serial.print("📱 Loaded WiFi SSID: ");
    Serial.println(wifiSSID);
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
