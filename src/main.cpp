/**
 * @file main.cpp
 * @brief Main implementation file for CO2 Car Race Timer for ESP32
 * @version 0.11.5
 * @date 2025-04-12
 *
 * This file implements the core functionality of the CO2 Car Race Timer,
 * including sensor management, race timing, and control logic using FreeRTOS.
 * The system uses dual VL53L0X sensors for precise start/finish detection.
 *
 * @copyright Copyright (c) 2025
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>
#include <esp_log.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Project includes
#include "version.h"
#include "storage.h"
#include "race_types.h"
#include "pin_config.h"
#include "config_manager.h"
#include "wifi_manager.h"

// Sensor libraries
#include <Adafruit_VL53L0X.h>

#define LOG_TAG "RACE_TIMER"

// Global objects
Adafruit_VL53L0X sensor1 = Adafruit_VL53L0X();
Adafruit_VL53L0X sensor2 = Adafruit_VL53L0X();

WifiManager& wifiManager = WifiManager::getInstance();

// Task handles
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t timerTaskHandle = NULL;
TaskHandle_t raceControlTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;

// Queue handles
QueueHandle_t sensorQueue = NULL;
QueueHandle_t raceEventQueue = NULL;

// Semaphore handles
SemaphoreHandle_t i2cMutex = NULL;
SemaphoreHandle_t raceMutex = NULL;

// Task functions declarations
void sensorTask(void *pvParameters);
void timerTask(void *pvParameters);
void raceControlTask(void *pvParameters);
void webServerTask(void *pvParameters);

// Function declarations
bool initSensors();
bool initStorage();
void handleRaceFinish(uint32_t startTime, uint32_t finishTime, uint16_t lane1Time, uint16_t lane2Time);

// Storage initialization function
bool initStorage() {
    // Initialize storage
    if (!Storage::begin()) {
        ESP_LOGE(LOG_TAG, "Failed to initialize storage!");
        return false;
    }
    return true;
}

// Sensor initialization function
bool initSensors() {
    ESP_LOGI(LOG_TAG, "Initializing VL53L0X sensors...");
    
    // Start with both sensors off
    digitalWrite(PIN_SENSOR1_XSHUT, LOW);
    digitalWrite(PIN_SENSOR2_XSHUT, LOW);
    delay(100);
    
    // Initialize first sensor
    digitalWrite(PIN_SENSOR1_XSHUT, HIGH);
    delay(100);
    
    if (!sensor1.begin(0x29)) {
        ESP_LOGE(LOG_TAG, "Failed to initialize first sensor!");
        return false;
    }
    
    if (!sensor1.setAddress(0x30)) {
        ESP_LOGE(LOG_TAG, "Failed to set first sensor address!");
        return false;
    }
    ESP_LOGI(LOG_TAG, "First sensor initialized at 0x30");
    
    // Initialize second sensor
    digitalWrite(PIN_SENSOR2_XSHUT, HIGH);
    delay(100);
    
    if (!sensor2.begin(0x29)) {
        ESP_LOGE(LOG_TAG, "Failed to initialize second sensor!");
        return false;
    }
    
    if (!sensor2.setAddress(0x31)) {
        ESP_LOGE(LOG_TAG, "Failed to set second sensor address!");
        return false;
    }
    ESP_LOGI(LOG_TAG, "Second sensor initialized at 0x31");
    
    // Configure sensors for faster readings
    sensor1.setMeasurementTimingBudgetMicroSeconds(20000);
    sensor2.setMeasurementTimingBudgetMicroSeconds(20000);
    
    return true;
}

// Sensor task implementation
void sensorTask(void *pvParameters) {
    VL53L0X_RangingMeasurementData_t measure1, measure2;
    SensorEvent event;
    TickType_t lastWakeTime;
    
    // Initialize task timing
    lastWakeTime = xTaskGetTickCount();
    
    while (true) {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Read sensor 1
            sensor1.rangingTest(&measure1, false);
            if (measure1.RangeStatus != 4) {
                event.sensorId = 1;
                event.distance = measure1.RangeMilliMeter;
                event.timestamp = millis();
                xQueueSend(sensorQueue, &event, 0);
            }
            
            // Read sensor 2
            sensor2.rangingTest(&measure2, false);
            if (measure2.RangeStatus != 4) {
                event.sensorId = 2;
                event.distance = measure2.RangeMilliMeter;
                event.timestamp = millis();
                xQueueSend(sensorQueue, &event, 0);
            }
            
            xSemaphoreGive(i2cMutex);
        }
        
        // Run at 50Hz (20ms period)
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20));
    }
}

// Timer task implementation
void timerTask(void *pvParameters) {
    uint32_t startTime = 0;
    uint32_t currentTime = 0;
    bool raceInProgress = false;
    
    while (true) {
        if (raceInProgress) {
            currentTime = millis();
            // Update race time and check for finish conditions
            // Implementation depends on race logic
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz update rate
    }
}

// Race control task implementation
void raceControlTask(void *pvParameters) {
    RaceEvent event;
    bool raceInProgress = false;
    bool carsLoaded = false;
    uint32_t startTime = 0;
    uint32_t lane1FinishTime = 0;
    uint32_t lane2FinishTime = 0;
    const uint16_t DETECTION_THRESHOLD = 150; // 150mm detection threshold
    
    while (true) {
        // Check LOAD button
        if (!digitalRead(PIN_BUTTON_LOAD) && !raceInProgress && !carsLoaded) {
            carsLoaded = true;
            digitalWrite(PIN_LED_RED, HIGH);
            digitalWrite(PIN_LED_GREEN, HIGH);
            ESP_LOGI(LOG_TAG, "Cars loaded, ready to race");
            vTaskDelay(pdMS_TO_TICKS(500)); // Debounce
        }
        
        // Check START button
        if (!digitalRead(PIN_BUTTON_START) && carsLoaded && !raceInProgress) {
            // Start race sequence
            digitalWrite(PIN_LED_RED, LOW);
            digitalWrite(PIN_LED_GREEN, LOW);
            digitalWrite(PIN_LED_BLUE, HIGH);
            
            // Trigger relay (active LOW) for 250ms
            digitalWrite(PIN_RELAY, LOW);
            vTaskDelay(pdMS_TO_TICKS(250));
            digitalWrite(PIN_RELAY, HIGH);
            
            startTime = millis();
            raceInProgress = true;
            ESP_LOGI(LOG_TAG, "Race started");
            
            // Send race start event
            event.type = RACE_START;
            event.timestamp = startTime;
            xQueueSend(raceEventQueue, &event, 0);
        }
        
        // Check sensor readings during race
        if (raceInProgress) {
            SensorEvent sensorEvent;
            if (xQueueReceive(sensorQueue, &sensorEvent, 0) == pdTRUE) {
                if (sensorEvent.distance < DETECTION_THRESHOLD) {
                    // Car detected at finish line
                    if (sensorEvent.sensorId == 1 && lane1FinishTime == 0) {
                        lane1FinishTime = sensorEvent.timestamp;
                        ESP_LOGI(LOG_TAG, "Lane 1 finish: %lu ms", lane1FinishTime - startTime);
                    } else if (sensorEvent.sensorId == 2 && lane2FinishTime == 0) {
                        lane2FinishTime = sensorEvent.timestamp;
                        ESP_LOGI(LOG_TAG, "Lane 2 finish: %lu ms", lane2FinishTime - startTime);
                    }
                    
                    // Check if race is complete
                    if (lane1FinishTime > 0 && lane2FinishTime > 0) {
                        // Race finished
                        raceInProgress = false;
                        carsLoaded = false;
                        digitalWrite(PIN_LED_BLUE, LOW);
                        digitalWrite(PIN_LED_GREEN, HIGH);
                        
                        // Calculate race times
                        uint16_t time1 = lane1FinishTime - startTime;
                        uint16_t time2 = lane2FinishTime - startTime;
                        
                        // Handle race finish and save results
                        handleRaceFinish(startTime, max(lane1FinishTime, lane2FinishTime),
                                        time1, time2);
                        
                        // Reset finish times
                        lane1FinishTime = 0;
                        lane2FinishTime = 0;
                        
                        // Beep to indicate finish
                        digitalWrite(PIN_BUZZER, HIGH);
                        vTaskDelay(pdMS_TO_TICKS(100));
                        digitalWrite(PIN_BUZZER, LOW);
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void webServerTask(void *pvParameters) {
    ESP_LOGI(LOG_TAG, "Starting web server task...");
    
    // Get network configuration
    ConfigManager::NetworkConfig netConfig = ConfigManager::getInstance().getNetworkConfig();
    
    // Initialize WiFi manager
    WifiManager& wifi = WifiManager::getInstance();
    bool apMode = (netConfig.wifiMode == "ap");
    const char* ssid = apMode ? netConfig.apSsid.c_str() : netConfig.ssid.c_str();
    const char* password = apMode ? netConfig.apPassword.c_str() : netConfig.password.c_str();
    
    ESP_LOGI(LOG_TAG, "Starting WiFi in %s mode...", apMode ? "AP" : "Station");
    if (!wifi.begin(ssid, password, apMode)) {
        ESP_LOGE(LOG_TAG, "Failed to initialize WiFi!");
        return;
    }
    
    ESP_LOGI(LOG_TAG, "WiFi initialized successfully");
    ESP_LOGI(LOG_TAG, "Mode: %s", apMode ? "AP" : "Station");
    ESP_LOGI(LOG_TAG, "SSID: %s", ssid);
    ESP_LOGI(LOG_TAG, "IP: %s", apMode ? WiFi.softAPIP().toString().c_str() : WiFi.localIP().toString().c_str());
    
    while (1) {
        // Nothing to do here as AsyncWebServer handles requests asynchronously
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void handleRaceFinish(uint32_t startTime, uint32_t finishTime, uint16_t lane1Time, uint16_t lane2Time) {
    RaceResult result;
    result.timestamp = time(nullptr);
    result.startTime = startTime;
    result.finishTime = finishTime;
    result.elapsedTime = finishTime - startTime;
    result.lane1Time = lane1Time;
    result.lane2Time = lane2Time;
    
    // Determine winner
    if (lane1Time == lane2Time) {
        result.winningLane = 0;
        result.isTie = true;
    } else {
        result.winningLane = (lane1Time < lane2Time) ? 1 : 2;
        result.isTie = false;
    }
    
    // Save race result
    if (!Storage::saveRaceResult(result)) {
        ESP_LOGE(LOG_TAG, "Failed to save race result!");
    } else {
        ESP_LOGI(LOG_TAG, "Race result saved successfully");
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize GPIO pins
    pinMode(PIN_BUTTON_LOAD, INPUT_PULLUP);
    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_SENSOR1_XSHUT, OUTPUT);
    pinMode(PIN_SENSOR2_XSHUT, OUTPUT);
    
    // Set initial states
    digitalWrite(PIN_RELAY, HIGH);  // Relay off (active LOW)
    digitalWrite(PIN_LED_BLUE, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    
    // Initialize I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    
    // Initialize SPI for SD card
    SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    
    // Create mutexes
    i2cMutex = xSemaphoreCreateMutex();
    raceMutex = xSemaphoreCreateMutex();
    
    if (!i2cMutex || !raceMutex) {
        ESP_LOGE(LOG_TAG, "Failed to create semaphores!");
        return;
    }
    
    // Initialize storage
    if (!initStorage()) {
        ESP_LOGE(LOG_TAG, "Failed to initialize storage!");
        return;
    }
    
    // Initialize configuration
    if (!ConfigManager::getInstance().begin()) {
        ESP_LOGE(LOG_TAG, "Failed to initialize configuration!");
        return;
    }
    
    // Initialize sensors
    if (!initSensors()) {
        ESP_LOGE(LOG_TAG, "Failed to initialize sensors!");
        return;
    }
    
    // Create queues
    sensorQueue = xQueueCreate(10, sizeof(SensorEvent));
    raceEventQueue = xQueueCreate(10, sizeof(RaceEvent));
    
    if (!sensorQueue || !raceEventQueue) {
        ESP_LOGE(LOG_TAG, "Failed to create queues!");
        return;
    }
    
    // Create tasks
    xTaskCreatePinnedToCore(
        sensorTask,          // Function
        "SensorTask",        // Name
        8192,               // Stack size (bytes)
        NULL,               // Parameters
        2,                  // Priority (0-24, higher number = higher priority)
        &sensorTaskHandle,  // Task handle
        1                   // Core (1 = real-time core)
    );
    
    xTaskCreatePinnedToCore(
        timerTask,
        "TimerTask",
        4096,
        NULL,
        3,                  // Higher priority for timing accuracy
        &timerTaskHandle,
        1                   // Real-time core
    );
    
    xTaskCreatePinnedToCore(
        raceControlTask,
        "RaceControl",
        4096,
        NULL,
        1,
        &raceControlTaskHandle,
        1                   // Real-time core
    );
    
    xTaskCreatePinnedToCore(
        webServerTask,
        "WebServer",
        8192,
        NULL,
        1,
        &webServerTaskHandle,
        0                   // Network core
    );
    
    ESP_LOGI(LOG_TAG, "All tasks created");
    ESP_LOGI(LOG_TAG, "Setup complete");
}

void loop() {
    // Empty loop - tasks handle everything
    vTaskDelay(pdMS_TO_TICKS(1000));
}
