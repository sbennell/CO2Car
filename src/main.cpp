/*
--- CO₂ Car Race Timer Version 0.8.3 ESP32 - 08 April 2025 ---
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
- Fair tie detection with 2ms tolerance

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
#include "NetworkManager.h"
#include "WebServer.h"
#include "Version.h"
#include "TimeManager.h"
#include "Configuration.h"
#include "Debug.h"

// Function prototypes
void setLEDState(String state);
void startRace();
void checkFinish();
void declareWinner();
void connectToWiFi();
void handleWebSocketCommand(const char* command);

// Global instances
TimeManager timeManager;
Configuration config;
NetworkManager networkManager(config);
WebServer webServer(timeManager, config, networkManager);
VL53L0X sensor1;
VL53L0X sensor2;

// Pin Definitions
#define LOAD_BUTTON_PIN 4
#define START_BUTTON_PIN 5
#define XSHUT1 16
#define XSHUT2 17
#define RELAY_PIN 14  // Changed back to GPIO14 per pin assignments
#define BUZZER_PIN 27

// RGB LED Pins
const int LED_RED = 25;
const int LED_GREEN = 26;
const int LED_BLUE = 33;

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



void handleWebSocketCommand(const char* command) {
    if (strcmp(command, "load") == 0 && !carsLoaded) {
        carsLoaded = true;
        setLEDState("ready");
        webServer.notifyStatus("Ready");
        Serial.println("🚦 Cars loaded. Ready to start!");
    }
    else if (strcmp(command, "start") == 0 && carsLoaded && !raceStarted) {
        startRace();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== CO₂ Car Race Timer ===");
    Serial.printf("Version: %s (Built: %s)\n", "0.8.3", "08-04-2025");
    Serial.println("=========================");
    Serial.println("Initializing system...");

    // Initialize LEDC for buzzer
    ledcSetup(0, 2000, 8);  // Channel 0, 2000 Hz, 8-bit resolution
    ledcAttachPin(BUZZER_PIN, 0);

    // Initialize network
    networkManager.begin();
    
    // Initialize NTP if connected in station mode
    if (networkManager.isConnected() && !networkManager.isAPMode()) {
        Serial.print("🕒 Synchronizing NTP time");
        timeManager.begin();
    }
    
    // Initialize web server (this will mount LittleFS)
    webServer.setCommandHandler(handleWebSocketCommand);
    webServer.begin();
    
    // Initialize configuration (after LittleFS is mounted)
    config.begin();

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
        Serial.println("✔ Sensor 1 initialized at 0x30.");
    } else {
        Serial.println("❌ ERROR: Sensor 1 not detected!");
        return;
    }

    digitalWrite(XSHUT2, HIGH);
    delay(10);
    if (sensor2.init()) {
        sensor2.setAddress(0x31);
        Serial.println("✔ Sensor 2 initialized at 0x31.");
    } else {
        Serial.println("❌ ERROR: Sensor 2 not detected!");
        return;
    }

    sensor1.startContinuous();
    sensor2.startContinuous();
    Serial.println("✔ Sensors are now active.");

    Serial.println("\n✅ System Ready!");
    Serial.println("Press 'L' via Serial or press the load button to load cars.");
}

void loop() {
    networkManager.update();
    timeManager.update();
    static unsigned long lastSensorCheck = 0;
    
    // Update sensor status every second
    if (millis() - lastSensorCheck > 1000) {
        lastSensorCheck = millis();
        bool sensor1Ok = sensor1.readRangeContinuousMillimeters() != 65535;
        bool sensor2Ok = sensor2.readRangeContinuousMillimeters() != 65535;
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

    // Update network status every 5 seconds
    static unsigned long lastNetworkCheck = 0;
    if (millis() - lastNetworkCheck >= 5000) {
        lastNetworkCheck = millis();
        webServer.notifyNetworkStatus();
    }

    if (raceStarted) {
        checkFinish();
    }
}

void startRace() {
    if (!carsLoaded || raceStarted) return;
    
    Serial.println("\n🚦 Race Starting...");
    Serial.println("📍 Firing CO₂ Relay...");

    // Sound start buzzer
    ledcWriteTone(0, 2000);
    delay(100);
    ledcWrite(0, 0);
    
    // Fire the relay (active LOW)
    digitalWrite(RELAY_PIN, LOW);
    delay(config.getRelayActivationTime());  // Configurable activation time
    digitalWrite(RELAY_PIN, HIGH);
    
    // Start the race
    setLEDState("racing");
    raceStarted = true;
    car1Finished = false;
    car2Finished = false;
    car1Time = 0;
    car2Time = 0;
    startTime = millis();
    
    // Update web interface
    webServer.notifyTimes(0, 0);
    Serial.println("✔ Relay deactivated");

    carsLoaded = false;
    Serial.println("🏎 Race in progress...");
}

void checkFinish() {
    if (!raceStarted) return;
    
    // Read both sensors first to get readings as close together as possible
    int dist1 = sensor1.readRangeContinuousMillimeters();
    int dist2 = sensor2.readRangeContinuousMillimeters();
    unsigned long currentTime = millis();
    bool finishedThisCheck = false;
    
    // Check both sensors before updating times to handle simultaneous finishes
    bool car1CrossedLine = !car1Finished && dist1 < config.getSensorThreshold();
    bool car2CrossedLine = !car2Finished && dist2 < config.getSensorThreshold();
    
    // If both cars cross the line within the tie threshold window, consider it simultaneous
    if (car1CrossedLine && car2CrossedLine) {
        car1Time = currentTime - startTime;
        car2Time = currentTime - startTime;
        car1Finished = car2Finished = true;
        finishedThisCheck = true;
        
        Serial.println("🏁 Both cars finished simultaneously!");
        Serial.printf("Time: %lu ms\n", car1Time);
    } else {
        // Handle individual finishes
        if (car1CrossedLine) {
            car1Time = currentTime - startTime;
            car1Finished = true;
            finishedThisCheck = true;
            Serial.printf("🏁 Car 1 Finished! Time: %lu ms\n", car1Time);
            
            // If car 2 already finished, check for tie based on threshold
            if (car2Finished) {
                int timeDiff = abs((int)car1Time - (int)car2Time);
                if (timeDiff <= (config.getTieThreshold() * 1000)) {
                    // For ties, use the average of both times
                    unsigned long avgTime = (car1Time + car2Time) / 2;
                    car1Time = car2Time = avgTime;
                    Serial.println("⚖️ Times within threshold - adjusted to tie!");
                    Serial.printf("Adjusted time: %lu ms\n", avgTime);
                }
            }
        }
        
        if (car2CrossedLine) {
            car2Time = currentTime - startTime;
            car2Finished = true;
            finishedThisCheck = true;
            Serial.printf("🏁 Car 2 Finished! Time: %lu ms\n", car2Time);
            
            // If car 1 already finished, check for tie based on threshold
            if (car1Finished) {
                int timeDiff = abs((int)car1Time - (int)car2Time);
                if (timeDiff <= (config.getTieThreshold() * 1000)) {
                    // For ties, use the average of both times
                    unsigned long avgTime = (car1Time + car2Time) / 2;
                    car1Time = car2Time = avgTime;
                    Serial.println("⚖️ Times within threshold - adjusted to tie!");
                    Serial.printf("Adjusted time: %lu ms\n", avgTime);
                }
            }
        }
    }
    
    // Only update web interface if something changed
    if (finishedThisCheck) {
        webServer.notifyTimes(car1Time / 1000.0, car2Time / 1000.0);
    }

    if (car1Finished && car2Finished) {
        declareWinner();
    }
}

void declareWinner() {
    Serial.println("\n🎉 Race Finished!");

    ledcWriteTone(0, 2000);
    delay(500);
    ledcWrite(0, 0);

    // Times have already been adjusted for ties in checkFinish()
    // Just determine the winner based on final times
    if (car1Time == car2Time) {
        Serial.println("🤝 It's a tie!");
    } else if (car1Time < car2Time) {
        Serial.println("🏆 Car 1 Wins!");
    } else {
        Serial.println("🏆 Car 2 Wins!");
    }

    Serial.print("📊 RESULT: C1=");
    Serial.print(car1Time);
    Serial.print("ms, C2=");
    Serial.print(car2Time);
    Serial.println("ms");

    // Notify race completion to save to history
    webServer.notifyRaceComplete(car1Time / 1000.0, car2Time / 1000.0);

    Serial.println("\n🔄 Getting ready for next race...");
    delay(2000);
    setLEDState("finished");
    raceStarted = false;
    Serial.println("\nPress 'L' via Serial or press the load button to load cars.");
}

void setLEDState(String state) {
    if (state == "waiting") {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
        webServer.notifyStatus("Waiting");
    }
    else if (state == "ready") {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
        webServer.notifyStatus("Ready");
    }
    else if (state == "racing") {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, HIGH);
        webServer.notifyStatus("Racing");
    }
    else if (state == "finished") {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
        webServer.notifyStatus("Finished");
    } else {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
    }
}
