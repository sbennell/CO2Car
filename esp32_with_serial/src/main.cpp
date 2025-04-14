/*
--- CO₂ Car Race Timer Version 0.6.0 ESP32 with FreeRTOS - 14 April 2025 ---
This system uses two VL53L0X distance sensors to time a CO₂-powered car race.
It measures the time taken for each car to cross the sensor line and declares the winner based on the fastest time.

Features:
- Two distance sensors (VL53L0X) track the progress of two cars.
- Relay control to simulate the CO₂ firing mechanism.
- Serial communication to start the race and display results.
- Supports multiple races by resetting after each one.
- RGB LED indicator to show current race state (waiting, ready, racing, finished).
- Buzzer feedback at race start and finish for audible cues.
- Debounced physical buttons for car load and race start.
- FreeRTOS implementation with dedicated cores:
  - Core 0: Racing functionality (sensor monitoring, timing)
  - Core 1: Everything else (serial, buttons, LED control)

ESP32 Pin Assignments:
- I2C: SDA=GPIO21, SCL=GPIO22
- VL53L0X Sensors: XSHUT1=GPIO16, XSHUT2=GPIO17
- Buttons: LOAD=GPIO4, START=GPIO5
- Relay: GPIO14
- Buzzer: GPIO27
- RGB LED: RED=GPIO25, GREEN=GPIO26, BLUE=GPIO33
*/

#include <Wire.h>
#include <VL53L0X.h>
#include <ArduinoJson.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Function prototypes
void setLEDState(String state);
void startRace();
void declareWinner();
void checkFinish();
void doStartRace();
void handleButtons();
void handleSerialCommands();

// Task function prototypes
void racingTask(void *parameter);
void peripheralTask(void *parameter);

// FreeRTOS task handles
TaskHandle_t racingTaskHandle = NULL;
TaskHandle_t peripheralTaskHandle = NULL;

// FreeRTOS synchronization
SemaphoreHandle_t serialMutex;
SemaphoreHandle_t raceStateMutex;

VL53L0X sensor1;
VL53L0X sensor2;

#define DEBUG false

// Core assignments
#define RACING_CORE 0
#define PERIPHERAL_CORE 1

// Pin Definitions
#define LOAD_BUTTON_PIN 4
#define START_BUTTON_PIN 13
#define XSHUT1 16
#define XSHUT2 17
#define RELAY_PIN 14
#define BUZZER_PIN 33

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
bool shouldStartRace = false;

// Thread-safe serial print
void serialPrint(const String &message) {
    if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.print(message);
        xSemaphoreGive(serialMutex);
    }
}

void serialPrintln(const String &message) {
    if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println(message);
        xSemaphoreGive(serialMutex);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize mutex and semaphores
    serialMutex = xSemaphoreCreateMutex();
    raceStateMutex = xSemaphoreCreateMutex();
    
    serialPrintln("\n--- CO₂ Car Race Timer with FreeRTOS ---");
    serialPrintln("Initializing system...");

    Wire.begin(21, 22);  // SDA = 21, SCL = 22
    delay(100);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    setLEDState("waiting");

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);  // HIGH = Relay OFF (active-LOW relay)
    serialPrintln("✔ Relay initialized (OFF)");

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    pinMode(XSHUT1, OUTPUT);
    pinMode(XSHUT2, OUTPUT);
    pinMode(LOAD_BUTTON_PIN, INPUT_PULLUP);
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(XSHUT1, LOW);
    digitalWrite(XSHUT2, LOW);
    delay(10);

    serialPrintln("🔄 Starting VL53L0X sensors...");

    digitalWrite(XSHUT1, HIGH);
    delay(10);
    if (sensor1.init()) {
        sensor1.setAddress(0x30);
        serialPrintln("✔ Sensor 1 initialized at 0x30.");
    } else {
        serialPrintln("❌ ERROR: Sensor 1 not detected!");
        return;
    }

    digitalWrite(XSHUT2, HIGH);
    delay(10);
    if (sensor2.init()) {
        sensor2.setAddress(0x31);
        serialPrintln("✔ Sensor 2 initialized at 0x31.");
    } else {
        serialPrintln("❌ ERROR: Sensor 2 not detected!");
        return;
    }

    sensor1.startContinuous();
    sensor2.startContinuous();
    serialPrintln("✔ Sensors are now active.");

    serialPrintln("\n✅ System Ready!");
    serialPrintln("Press 'L' via Serial or press the load button to load cars.");

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        racingTask,         // Task function
        "RacingTask",       // Name of task
        10000,              // Stack size (bytes)
        NULL,               // Parameter to pass
        1,                  // Task priority (1 is low)
        &racingTaskHandle,  // Task handle
        RACING_CORE         // Core where the task should run
    );

    xTaskCreatePinnedToCore(
        peripheralTask,         // Task function
        "PeripheralTask",       // Name of task
        10000,                  // Stack size (bytes)
        NULL,                   // Parameter to pass
        1,                      // Task priority (1 is low)
        &peripheralTaskHandle,  // Task handle
        PERIPHERAL_CORE         // Core where the task should run
    );
}

void loop() {
    // Empty loop - tasks handle everything
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Just to prevent watchdog issues
}

// Core 0: Racing Task - handles race timing and sensor monitoring
void racingTask(void *parameter) {
    while (true) {
        bool isRacing = false;
        bool shouldCheckFinish = false;
        
        // Check race state (thread-safe)
        if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
            isRacing = raceStarted;
            shouldCheckFinish = isRacing && (!car1Finished || !car2Finished);
            xSemaphoreGive(raceStateMutex);
        }

        // Process race start if requested
        if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
            if (shouldStartRace) {
                // Race start code moved to a function
                doStartRace();
                shouldStartRace = false;
            }
            xSemaphoreGive(raceStateMutex);
        }

        // Check finish if race is active
        if (shouldCheckFinish) {
            checkFinish();
        }

        // Small delay to give other tasks time
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

// Core 1: Peripheral Task - handles buttons, serial, LEDs
void peripheralTask(void *parameter) {
    while (true) {
        // Button handling
        handleButtons();
        
        // Serial command handling
        if (Serial.available() > 0) {
            handleSerialCommands();
        }
        
        // Small delay to prevent CPU hogging
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void handleButtons() {
    bool loadButtonState = digitalRead(LOAD_BUTTON_PIN);
    bool startButtonState = digitalRead(START_BUTTON_PIN);

    // Load button handling
    if (loadButtonState == LOW && loadButtonLastState == HIGH) {
        loadButtonPressed = true;
        
        if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
            if (!carsLoaded && !raceStarted) {
                carsLoaded = true;
                setLEDState("ready");
                serialPrintln("🚦 Cars loaded. Press 'S' to start the race.");
            }
            xSemaphoreGive(raceStateMutex);
        }
    }
    loadButtonLastState = loadButtonState;

    // Start button handling
    if (startButtonState == LOW && startButtonLastState == HIGH) {
        startButtonPressed = true;
        
        if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
            if (carsLoaded && !raceStarted) {
                setLEDState("racing");
                shouldStartRace = true;
            } else if (!carsLoaded) {
                serialPrintln("⚠ Please load the cars first by pressing 'L' or pressing the load button.");
            } else {
                serialPrintln("⚠ Race already in progress!");
            }
            xSemaphoreGive(raceStateMutex);
        }
    }
    startButtonLastState = startButtonState;
}

void handleSerialCommands() {
    char command = Serial.read();
    
    if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.print("📩 Received Serial Command: ");
        Serial.println(command);
        xSemaphoreGive(serialMutex);
    }

    if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
        if (command == 'L') {
            if (!carsLoaded && !raceStarted) {
                carsLoaded = true;
                setLEDState("ready");
                serialPrintln("🚦 Cars loaded. Press 'S' to start the race.");
            } else if (carsLoaded) {
                serialPrintln("⚠ Cars are already loaded.");
            } else {
                serialPrintln("⚠ Cannot load cars during a race.");
            }
        }

        if (command == 'S') {
            if (carsLoaded && !raceStarted) {
                setLEDState("racing");
                shouldStartRace = true;
            } else if (!carsLoaded) {
                serialPrintln("⚠ Please load the cars first by pressing 'L' or pressing the load button.");
            } else {
                serialPrintln("⚠ Race already in progress! Wait for finish.");
            }
        }
        
        xSemaphoreGive(raceStateMutex);
    }
}

// This function is called from the Racing Task when shouldStartRace is true
void doStartRace() {
    serialPrintln("\n🚦 Race Starting...");
    serialPrintln("🔹 Firing CO₂ Relay...");

    tone(BUZZER_PIN, 1000, 200);

    digitalWrite(RELAY_PIN, LOW);  // LOW = Relay ON
    startTime = millis();
    raceStarted = true;
    car1Finished = false;
    car2Finished = false;
    delay(250);  // Increased delay to 250ms to ensure relay has time to actuate
    digitalWrite(RELAY_PIN, HIGH);  // HIGH = Relay OFF
    serialPrintln("✔ Relay deactivated");

    carsLoaded = false;
    serialPrintln("🏎 Race in progress...");
}

// This function is called by startRace to trigger race start
void startRace() {
    if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
        shouldStartRace = true;
        xSemaphoreGive(raceStateMutex);
    }
}

void checkFinish() {
    int dist1 = sensor1.readRangeContinuousMillimeters();
    int dist2 = sensor2.readRangeContinuousMillimeters();

    if (dist1 == -1) {
        serialPrintln("❌ Error reading from Sensor 1");
        return;
    }
    if (dist2 == -1) {
        serialPrintln("❌ Error reading from Sensor 2");
        return;
    }

    if (DEBUG) {
        if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
            Serial.print("📏 Sensor Readings: C1 = ");
            Serial.print(dist1);
            Serial.print(" mm, C2 = ");
            Serial.print(dist2);
            Serial.println(" mm");
            xSemaphoreGive(serialMutex);
        }
    }

    // Critical section for checking race state
    if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
        if (dist1 < 150 && !car1Finished) {
            car1Time = millis() - startTime;
            car1Finished = true;
            
            if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
                Serial.print("🏁 Car 1 Finished! Time: ");
                Serial.print(car1Time);
                Serial.println(" ms");
                xSemaphoreGive(serialMutex);
            }
        }

        if (dist2 < 150 && !car2Finished) {
            car2Time = millis() - startTime;
            car2Finished = true;
            
            if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
                Serial.print("🏁 Car 2 Finished! Time: ");
                Serial.print(car2Time);
                Serial.println(" ms");
                xSemaphoreGive(serialMutex);
            }
        }

        bool bothFinished = car1Finished && car2Finished;
        xSemaphoreGive(raceStateMutex);
        
        if (bothFinished) {
            declareWinner();
        }
    }
}

void declareWinner() {
    unsigned long c1Time, c2Time;
    
    // Get race times safely
    if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
        c1Time = car1Time;
        c2Time = car2Time;
        xSemaphoreGive(raceStateMutex);
    }
    
    serialPrintln("\n🎉 Race Finished!");

    tone(BUZZER_PIN, 2000, 500);

    if (c1Time < c2Time) {
        serialPrintln("🏆 Car 1 Wins!");
    } else if (c2Time < c1Time) {
        serialPrintln("🏆 Car 2 Wins!");
    } else {
        serialPrintln("🤝 It's a tie!");
    }

    if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.print("📊 RESULT: C1=");
        Serial.print(c1Time);
        Serial.print("ms, C2=");
        Serial.print(c2Time);
        Serial.println("ms");
        xSemaphoreGive(serialMutex);
    }

    serialPrintln("\n🔄 Getting ready for next race...");
    delay(2000);
    setLEDState("finished");
    
    // Reset race state safely
    if (xSemaphoreTake(raceStateMutex, portMAX_DELAY) == pdTRUE) {
        raceStarted = false;
        xSemaphoreGive(raceStateMutex);
    }
    
    serialPrintln("\nPress 'L' via Serial or press the load button to load cars.");
}

void setLEDState(String state) {
    if (state == "waiting") {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
    } else if (state == "ready") {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
    } else if (state == "racing") {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, HIGH);
    } else if (state == "finished") {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
    } else {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
    }
}