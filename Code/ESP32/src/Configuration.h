#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

class Configuration {
public:
    Configuration();
    void begin();
    void save();
    
    // WiFi settings
    String getWiFiSSID() const { return wifiSSID; }
    String getWiFiPassword() const { return wifiPassword; }
    void setWiFiCredentials(const String& ssid, const String& password);
    
    // Sensor settings
    int getSensorThreshold() const { return sensorThreshold; }
    void setSensorThreshold(int threshold);
    
    // Race timing parameters
    int getRelayActivationTime() const { return relayActivationTime; }
    void setRelayActivationTime(int ms);
    float getTieThreshold() const { return tieThreshold; }
    void setTieThreshold(float seconds);
    

    
private:
    static const char* CONFIG_FILE;
    void loadFromFile();
    void saveToFile();
    
    // WiFi settings
    String wifiSSID;
    String wifiPassword;
    
    // Sensor settings
    int sensorThreshold;         // Distance threshold in mm
    
    // Race timing parameters
    int relayActivationTime;     // Time in ms to activate relay
    float tieThreshold;          // Time difference in seconds to consider a tie

};
