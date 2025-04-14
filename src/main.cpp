/*
--- CO₂ Car Race Timer Version 0.10.0 ESP32 - 14 April 2025 ---
This system uses two VL53L0X distance sensors to time a CO₂-powered car race.
It measures the time taken for each car to cross the sensor line and declares the winner based on the fastest time.

Features:
- Two distance sensors (VL53L0X) track the progress of two cars
- Relay control to simulate the CO₂ firing mechanism
- Web interface for remote race control and monitoring
- Real-time race status and timing updates via WebSocket
- Race history storage with up to 50 past races
- RGB LED indicator for race state (waiting, ready, racing, finished)
- Buzzer feedback at race start and finish
- Debounced physical buttons for local control

ESP32 Pin Assignments:
- I2C: SDA=GPIO21, SCL=GPIO22
- VL53L0X Sensors: XSHUT1=GPIO16, XSHUT2=GPIO17
- Buttons: LOAD=GPIO4, START=GPIO5
- Relay: GPIO14 (active LOW)
- Buzzer: GPIO27
- RGB LED: RED=GPIO25, GREEN=GPIO26, BLUE=GPIO33

Web Interface:
- Real-time race status and timing display
- Remote load and start controls
- Race history table with past results
- System status indicators (WiFi, sensors)
- Tie detection with identical times display
*/

#include <Wire.h>
#include <VL53L0X.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include "NetworkManager.h"
#include "WebServer.h"
#include "Version.h"
#include "TimeManager.h"
#include "Configuration.h"
#include "Debug.h"
#include "SerialManager.h"


// Function prototypes
void setLEDState(String state);
void startRace();
void checkFinish();
void declareWinner();
void connectToWiFi();
void handleWebSocketCommand(const char* command);
bool initSDCard();
bool writeRaceToSD(unsigned long car1Time, unsigned long car2Time, const char* winner);

// Functions for SerialManager
void triggerRaceStart(unsigned long heatId);
void resetRaceTimer();
void calibrateSensors();
void sendSensorData();

// Global instances
TimeManager timeManager;
Configuration config;
NetworkManager networkManager(config);
WebServer webServer(timeManager, config, networkManager);
SerialManager serialManager(timeManager, config);
VL53L0X sensor1;
VL53L0X sensor2;

// Pin Definitions
#define LOAD_BUTTON_PIN 4
#define START_BUTTON_PIN 13
#define XSHUT1 16
#define XSHUT2 17
#define RELAY_PIN 14  // Changed back to GPIO14 per pin assignments
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS 5
#define BUZZER_PIN 33

// RGB LED Pins
const int LED_RED = 25;
const int LED_GREEN = 26;
const int LED_BLUE = 27;

// Race State Variables
unsigned long startTime;
bool raceStarted = false;
bool car1Finished = false;
bool car2Finished = false;
unsigned long car1Time = 0;
unsigned long car2Time = 0;
bool loadButtonPressed = false;
bool loadButtonLastState = HIGH;
bool carsLoaded = false;
bool startButtonPressed = false;
bool startButtonLastState = HIGH;
bool pauseUpdates = false;

// Variables for SerialManager
bool wifiConnected = false;
String wifiSSID = "";
int wifiRSSI = 0;
bool sensor1Ok = false;
bool sensor2Ok = false;
bool relayOk = true;
unsigned long currentHeatId = 0;
unsigned long lastSensorUpdateTime = 0;
const unsigned long SENSOR_UPDATE_INTERVAL = 200; // Send sensor data every 200ms

void handleWebSocketCommand(const char* command) {
    if (strcmp(command, "load") == 0 && !carsLoaded) {
        carsLoaded = true;
        setLEDState("ready");
        webServer.notifyStatus("Ready");
        Serial.println("🚦 Cars loaded. Ready to start!");
    }
    else if (strcmp(command, "start") == 0 && carsLoaded && !raceStarted) {
        setLEDState("racing");
        startRace();
    }
}

bool initSDCard() {
    if (!SD.begin(SD_CS)) {
        Serial.println("❌ SD Card initialization failed!");
        return false;
    }
    Serial.println("✅ SD Card initialized.");
    return true;
}

bool writeRaceToSD(unsigned long car1Time, unsigned long car2Time, const char* winner) {
    // Get current date for filename
    char filename[20];
    time_t now = time(nullptr);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    sprintf(filename, "/%04d-%02d-%02d.json", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    // Create race data JSON
    StaticJsonDocument<256> raceDoc;
    raceDoc["timestamp"] = timeManager.getEpochTime();
    raceDoc["car1_time"] = car1Time;
    raceDoc["car2_time"] = car2Time;
    raceDoc["winner"] = winner;
    
    // Check if daily file exists
    bool fileExists = SD.exists(filename);
    
    // Create or load daily file
    File dailyFile;
    StaticJsonDocument<4096> dailyDoc;
    
    if (fileExists) {
        dailyFile = SD.open(filename, FILE_READ);
        if (!dailyFile) {
            Serial.println("❌ Failed to open daily file for reading");
            return false;
        }
        
        DeserializationError error = deserializeJson(dailyDoc, dailyFile);
        dailyFile.close();
        
        if (error) {
            Serial.print("❌ JSON deserialization error: ");
            Serial.println(error.c_str());
            return false;
        }
    } else {
        // Create a new array if file doesn't exist
        dailyDoc.to<JsonArray>();
    }
    
    // Add race to daily array
    JsonArray races = dailyDoc.as<JsonArray>();
    races.add(raceDoc);
    
    // Write updated file
    dailyFile = SD.open(filename, FILE_WRITE);
    if (!dailyFile) {
        Serial.println("❌ Failed to open daily file for writing");
        return false;
    }
    
    if (serializeJson(dailyDoc, dailyFile) == 0) {
        Serial.println("❌ Failed to write race data");
        dailyFile.close();
        return false;
    }
    
    dailyFile.close();
    Serial.println("✅ Race data saved to SD: " + String(filename));
    return true;
}

// SerialManager integration functions
void triggerRaceStart(unsigned long heatId) {
    if (carsLoaded && !raceStarted) {
        currentHeatId = heatId;
        setLEDState("racing");
        startRace();
        
        // Send race start event to race management system
        serialManager.sendRaceStart(currentHeatId);
    } else if (!carsLoaded) {
        serialManager.sendError("Cars not loaded. Please load cars first.");
    } else if (raceStarted) {
        serialManager.sendError("Race already in progress.");
    }
}

void resetRaceTimer() {
    // Reset race state
    raceStarted = false;
    car1Finished = false;
    car2Finished = false;
    car1Time = 0;
    car2Time = 0;
    carsLoaded = false;
    pauseUpdates = false;
    
    // Reset LED state
    setLEDState("waiting");
    
    // Notify web clients
    webServer.notifyStatus("Waiting");
    webServer.notifyRaceComplete(0.0, 0.0);
    
    Serial.println("🔄 Race timer reset.");
}

void calibrateSensors() {
    // Temporary pause race functionality
    bool wasRacing = raceStarted;
    if (wasRacing) {
        raceStarted = false;
        pauseUpdates = true;
    }
    
    Serial.println("🔄 Calibrating sensors...");
    
    // Reset sensors
    digitalWrite(XSHUT1, LOW);
    digitalWrite(XSHUT2, LOW);
    delay(10);
    
    // Restart sensor 1
    digitalWrite(XSHUT1, HIGH);
    delay(10);
    if (sensor1.init()) {
        sensor1.setAddress(0x30);
        sensor1.startContinuous();
        sensor1Ok = true;
        Serial.println("✅ Sensor 1 calibrated.");
    } else {
        sensor1Ok = false;
        Serial.println("❌ Sensor 1 calibration failed!");
    }
    
    // Restart sensor 2
    digitalWrite(XSHUT2, HIGH);
    delay(10);
    if (sensor2.init()) {
        sensor2.setAddress(0x31);
        sensor2.startContinuous();
        sensor2Ok = true;
        Serial.println("✅ Sensor 2 calibrated.");
    } else {
        sensor2Ok = false;
        Serial.println("❌ Sensor 2 calibration failed!");
    }
    
    // Notify web clients of sensor status
    webServer.notifySensorStates(sensor1Ok, sensor2Ok);
    
    // Resume if we were racing
    if (wasRacing) {
        pauseUpdates = false;
        raceStarted = true;
    }
    
    Serial.println("✅ Calibration complete.");
}

void sendSensorData() {
    // Only send sensor data periodically to avoid flooding the serial port
    if (millis() - lastSensorUpdateTime > SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdateTime = millis();
        
        // Read sensor values
        int lane1Distance = sensor1.readRangeContinuousMillimeters();
        int lane2Distance = sensor2.readRangeContinuousMillimeters();
        
        // Check if readings are valid
        if (lane1Distance == 65535) lane1Distance = 0;
        if (lane2Distance == 65535) lane2Distance = 0;
        
        // Determine if car is detected (threshold from configuration)
        bool lane1Detected = (lane1Distance > 0 && lane1Distance < config.getSensorThreshold());
        bool lane2Detected = (lane2Distance > 0 && lane2Distance < config.getSensorThreshold());
        
        // Send sensor data to race management system
        serialManager.sendSensorReadings(lane1Distance, lane1Detected, lane2Distance, lane2Detected);
    }
}

void setup() {
    // Initialize serial communication via SerialManager
    serialManager.begin(115200);
    Serial.println("\n=== CO₂ Car Race Timer ===");
    Serial.printf("Version: %s (Built: %s)\n", VERSION_STRING, BUILD_DATE);
    Serial.println("=========================");
    Serial.println("Initializing system...");
    
    // Send initial status message during setup
    delay(500);
    #ifdef DEBUG
    Serial.println("Sending initial status to Race Management System...");
    #endif
    serialManager.sendStatus();
    
    // Send a second status update after all initialization is complete

    // Initialize LEDC for buzzer
    ledcSetup(0, 2000, 8);  // Channel 0, 2000 Hz, 8-bit resolution
    ledcAttachPin(BUZZER_PIN, 0);

    // Initialize network
    networkManager.begin();
    
    // Update WiFi status variables for SerialManager
    wifiConnected = networkManager.isConnected();
    wifiSSID = networkManager.getSSID();
    wifiRSSI = networkManager.getRSSI();
    
    // Initialize NTP if connected in station mode
    if (networkManager.isConnected() && !networkManager.isAPMode()) {
        Serial.print("🕒 Synchronizing NTP time");
        timeManager.begin();
        
        // Wait briefly for initial time sync
        for (int i = 0; i < 5; i++) {
            timeManager.update();
            delay(100);
            Serial.print(".");
        }
        Serial.println();
    }
    
    // Initialize web server (this will mount LittleFS)
    webServer.setCommandHandler(handleWebSocketCommand);
    webServer.begin();
    
    // Initialize configuration (after LittleFS is mounted)
    config.begin();

    // Initialize SPI for SD card
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!initSDCard()) {
        Serial.println("⚠️ System will continue without SD card logging");
    }
    
    Wire.begin(21, 22);  // SDA = 21, SCL = 22
    delay(100);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    setLEDState("waiting");

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);  // HIGH = Relay OFF (active-LOW relay)
    Serial.println("✔ Relay initialized (OFF)");

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    pinMode(XSHUT1, OUTPUT);
    pinMode(XSHUT2, OUTPUT);
    pinMode(LOAD_BUTTON_PIN, INPUT_PULLUP);
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(XSHUT1, LOW);
    digitalWrite(XSHUT2, LOW);
    delay(10);

    Serial.println("🔄 Starting VL53L0X sensors...");

    digitalWrite(XSHUT1, HIGH);
    delay(10);
    if (sensor1.init()) {
        sensor1.setAddress(0x30);
        sensor1Ok = true;
        Serial.println("✔ Sensor 1 initialized at 0x30.");
    } else {
        sensor1Ok = false;
        Serial.println("❌ ERROR: Sensor 1 not detected!");
    }

    digitalWrite(XSHUT2, HIGH);
    delay(10);
    if (sensor2.init()) {
        sensor2.setAddress(0x31);
        sensor2Ok = true;
        Serial.println("✔ Sensor 2 initialized at 0x31.");
    } else {
        sensor2Ok = false;
        Serial.println("❌ ERROR: Sensor 2 not detected!");
    }

    sensor1.startContinuous();
    sensor2.startContinuous();

    Serial.println("✅ System initialization complete!");
    Serial.println("Press 'L' via Serial or press the load button to load cars.");
    
    // Send final status update after initialization is complete
    delay(1000);
    #ifdef DEBUG
    Serial.println("Sending complete system status to Race Management System...");
    #endif
    serialManager.sendStatus();
}

void loop() {
    if (!pauseUpdates) {
        networkManager.update();
        timeManager.update();
    }
    
    // Process serial commands from race management system
    serialManager.processCommands();
    
    // Send sensor data to race management system
    sendSensorData();
    
    static unsigned long lastSensorCheck = 0;
    
    // Update sensor status every second when not racing
    if (!pauseUpdates && millis() - lastSensorCheck > 1000) {
        lastSensorCheck = millis();
        sensor1Ok = sensor1.readRangeContinuousMillimeters() != 65535;
        sensor2Ok = sensor2.readRangeContinuousMillimeters() != 65535;
        webServer.notifySensorStates(sensor1Ok, sensor2Ok);
    }

    bool loadButtonState = digitalRead(LOAD_BUTTON_PIN);
    bool startButtonState = digitalRead(START_BUTTON_PIN);

    if (loadButtonState == LOW && loadButtonLastState == HIGH) {
        loadButtonPressed = true;
        if (!carsLoaded) {
            carsLoaded = true;
            setLEDState("ready");
            webServer.notifyStatus("Ready");
            Serial.println("🚦 Cars loaded. Press 'S' to start the race.");
        }
    }
    loadButtonLastState = loadButtonState;

    if (startButtonState == LOW && startButtonLastState == HIGH) {
        startButtonPressed = true;
        if (carsLoaded && !raceStarted) {
            setLEDState("racing");
            startRace();
        } else if (!carsLoaded) {
            Serial.println("⚠ Please load the cars first by pressing 'L' or pressing the load button.");
        } else {
            Serial.println("⚠ Race already in progress!");
        }
    }
    startButtonLastState = startButtonState;

    if (Serial.available() > 0) {
        char command = Serial.read();
        Serial.print("📩 Received Serial Command: ");
        Serial.println(command);

        if (command == 'L') {
            if (!carsLoaded) {
                carsLoaded = true;
                setLEDState("ready");
                Serial.println("🚦 Cars loaded. Press 'S' to start the race.");
            } else {
                Serial.println("⚠ Cars are already loaded.");
            }
        }

        if (command == 'S' && carsLoaded) {
            if (!raceStarted) {
                setLEDState("racing");
                startRace();
            } else {
                Serial.println("⚠ Race already in progress! Wait for finish.");
            }
        } else if (command == 'S' && !carsLoaded) {
            Serial.println("⚠ Please load the cars first by pressing 'L' or pressing the load button.");
        }
    }

    // Update network status every 5 seconds when not racing
    static unsigned long lastNetworkCheck = 0;
    if (!pauseUpdates && millis() - lastNetworkCheck >= 5000) {
        lastNetworkCheck = millis();
        webServer.notifyNetworkStatus();
        
        // Update WiFi status variables for SerialManager
        wifiConnected = networkManager.isConnected();
        wifiSSID = networkManager.getSSID();
        wifiRSSI = networkManager.getRSSI();
        
        // Send comprehensive status update to race management system
        serialManager.sendStatus();
    }

    if (raceStarted) {
        checkFinish();
    }
}

void startRace() {
    if (!carsLoaded || raceStarted) return;
    
    raceStarted = true;
    car1Finished = false;
    car2Finished = false;
    car1Time = 0;
    car2Time = 0;
    
    // Play start sound
    ledcWriteTone(0, 880);
    delay(100);
    ledcWriteTone(0, 1760);
    delay(100);
    ledcWriteTone(0, 0);
    
    // Activate relay to release cars
    digitalWrite(RELAY_PIN, LOW);  // LOW = Relay ON
    delay(250);  // Hold for 250ms
    digitalWrite(RELAY_PIN, HIGH); // HIGH = Relay OFF
    
    // Record start time
    startTime = millis();
    
    // Send race start notification to web clients
    webServer.notifyStatus("Racing");
    webServer.notifyStatus("Race Started");
    
    // Send race start event to race management system
    serialManager.sendRaceStart(currentHeatId);
    
    Serial.println("🏁 Race started!");
    
    // Pause other updates during race to ensure timing accuracy
    pauseUpdates = true;
}

void checkFinish() {
    if (!raceStarted) return;
    
    // Read sensor values
    int lane1Distance = sensor1.readRangeContinuousMillimeters();
    int lane2Distance = sensor2.readRangeContinuousMillimeters();
    
    // Check for sensor errors
    bool lane1Error = (lane1Distance == 65535);
    bool lane2Error = (lane2Distance == 65535);
    
    // Get threshold from configuration
    int threshold = config.getSensorThreshold();
    
    // Check if car 1 has finished
    if (!car1Finished && !lane1Error && lane1Distance < threshold) {
        car1Time = millis() - startTime;
        car1Finished = true;
        Serial.printf("🏎️ Car 1 finished! Time: %lu ms\n", car1Time);
        webServer.notifyTimes(car1Time, car2Time);
    }
    
    // Check if car 2 has finished
    if (!car2Finished && !lane2Error && lane2Distance < threshold) {
        car2Time = millis() - startTime;
        car2Finished = true;
        Serial.printf("🏎️ Car 2 finished! Time: %lu ms\n", car2Time);
        webServer.notifyTimes(car1Time, car2Time);
    }
    
    // If both cars have finished or timeout (10 seconds)
    if ((car1Finished && car2Finished) || (millis() - startTime > 10000)) {
        declareWinner();
    }
}

void declareWinner() {
    if (!raceStarted) return;
    
    // Resume other updates
    pauseUpdates = false;
    
    // Determine winner
    const char* winner = "";
    
    if (car1Finished && car2Finished) {
        // Check for tie (within 2ms)
        if (abs((long)car1Time - (long)car2Time) <= 2) {
            winner = "tie";
            Serial.println("🏆 It's a tie!");
        } else if (car1Time < car2Time) {
            winner = "1";
            Serial.println("🏆 Car 1 wins!");
        } else {
            winner = "2";
            Serial.println("🏆 Car 2 wins!");
        }
    } else if (car1Finished) {
        winner = "1";
        Serial.println("🏆 Car 1 wins! (Car 2 did not finish)");
    } else if (car2Finished) {
        winner = "2";
        Serial.println("🏆 Car 2 wins! (Car 1 did not finish)");
    } else {
        Serial.println("⚠ No cars finished the race!");
    }
    
    // Play finish sound
    ledcWriteTone(0, 1760);
    delay(100);
    ledcWriteTone(0, 880);
    delay(100);
    ledcWriteTone(0, 0);
    
    // Send race finish notification to web clients
    webServer.notifyTimes(car1Time, car2Time);
    webServer.notifyRaceComplete(car1Time / 1000.0, car2Time / 1000.0);
    
    // Send race finish event to race management system
    serialManager.sendRaceFinish(currentHeatId, car1Time, car2Time, winner);
    
    // Save race results to SD card
    writeRaceToSD(car1Time, car2Time, winner);
    
    // Reset race state
    raceStarted = false;
    setLEDState("waiting");
    
    // Reset current heat ID
    currentHeatId = 0;
}

void setLEDState(String state) {
    if (state == "waiting") {
        // Blue
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, HIGH);
    } else if (state == "ready") {
        // Yellow
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
    } else if (state == "racing") {
        // Green
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
    } else if (state == "finished") {
        // Purple
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, HIGH);
    } else if (state == "error") {
        // Red
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
    }
}
