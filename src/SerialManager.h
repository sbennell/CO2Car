#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "TimeManager.h"
#include "Configuration.h"

class SerialManager {
public:
    SerialManager(TimeManager& timeManager, Configuration& config);
    
    // Initialize serial communication
    void begin(unsigned long baudRate = 115200);
    
    // Process incoming serial commands
    void processCommands();
    
    // Send status update to race management system
    void sendStatus();
    
    // Send race start event
    void sendRaceStart(unsigned long heatId);
    
    // Send race finish event with results
    void sendRaceFinish(unsigned long heatId, unsigned long car1Time, unsigned long car2Time, const char* winner);
    
    // Send sensor readings
    void sendSensorReadings(int lane1Distance, bool lane1Detected, int lane2Distance, bool lane2Detected);
    
    // Send error message
    void sendError(const char* message);
    
private:
    TimeManager& _timeManager;
    Configuration& _config;
    unsigned long _lastStatusTime = 0;
    const unsigned long STATUS_INTERVAL = 5000; // Send status every 5 seconds
    
    // Create a JSON status object
    void createStatusJson(JsonDocument& doc);
};

#endif // SERIAL_MANAGER_H
