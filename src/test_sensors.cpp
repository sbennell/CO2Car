#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>
#include <HardwareSerial.h>
#include <Adafruit_VL53L0X.h>
#include <vl53l0x_api.h>

#define LOG_TAG "VL53L0X_TEST"

Adafruit_VL53L0X sensor1 = Adafruit_VL53L0X();
Adafruit_VL53L0X sensor2 = Adafruit_VL53L0X();

// Pin Definitions
#define PIN_I2C_SDA           8
#define PIN_I2C_SCL           9
#define PIN_SENSOR1_XSHUT     16
#define PIN_SENSOR2_XSHUT     15
#define PIN_LED_RED          47

// Sensor addresses
#define SENSOR1_ADDRESS      0x30
#define SENSOR2_ADDRESS      0x31

bool sensor1Initialized = false;
bool sensor2Initialized = false;

void scanI2C() {
    ESP_LOGI(LOG_TAG, "Scanning I2C bus...");
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            ESP_LOGI(LOG_TAG, "Device found at address 0x%02X", address);
        }
    }
}

void initVL53L0X() {
    ESP_LOGI(LOG_TAG, "Initializing VL53L0X sensors...");
    
    // Start with both sensors off
    digitalWrite(PIN_SENSOR1_XSHUT, LOW);
    digitalWrite(PIN_SENSOR2_XSHUT, LOW);
    delay(100);
    ESP_LOGI(LOG_TAG, "Both sensors powered down");
    
    // Reset I2C
    Wire.end();
    delay(100);
    
    // Reinitialize I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
    delay(100);
    ESP_LOGI(LOG_TAG, "I2C reinitialized");
    
    // Initialize first sensor
    digitalWrite(PIN_SENSOR1_XSHUT, HIGH);
    delay(100);
    
    ESP_LOGI(LOG_TAG, "Initializing first sensor...");
    if (!sensor1.begin(0x29)) {
        ESP_LOGE(LOG_TAG, "Failed to initialize first sensor!");
        digitalWrite(PIN_LED_RED, HIGH);
        return;
    }
    
    // Change first sensor address
    if (!sensor1.setAddress(SENSOR1_ADDRESS)) {
        ESP_LOGE(LOG_TAG, "Failed to set first sensor address!");
        return;
    }
    ESP_LOGI(LOG_TAG, "First sensor initialized at 0x%02X", SENSOR1_ADDRESS);
    
    // Initialize second sensor
    digitalWrite(PIN_SENSOR2_XSHUT, HIGH);
    delay(100);
    
    ESP_LOGI(LOG_TAG, "Initializing second sensor...");
    if (!sensor2.begin(0x29)) {
        ESP_LOGE(LOG_TAG, "Failed to initialize second sensor!");
        digitalWrite(PIN_LED_RED, HIGH);
        return;
    }
    
    // Change second sensor address
    if (!sensor2.setAddress(SENSOR2_ADDRESS)) {
        ESP_LOGE(LOG_TAG, "Failed to set second sensor address!");
        return;
    }
    ESP_LOGI(LOG_TAG, "Second sensor initialized at 0x%02X", SENSOR2_ADDRESS);
    
    // Configure sensors
    sensor1.setMeasurementTimingBudgetMicroSeconds(50000);
    sensor2.setMeasurementTimingBudgetMicroSeconds(50000);
    
    // Test measurements
    VL53L0X_RangingMeasurementData_t measure1, measure2;
    
    ESP_LOGI(LOG_TAG, "Testing measurements...");
    
    sensor1.rangingTest(&measure1, false);
    sensor2.rangingTest(&measure2, false);
    
    if (measure1.RangeStatus != 4 && measure2.RangeStatus != 4) {
        ESP_LOGI(LOG_TAG, "Sensor 1 Distance (mm): %d", measure1.RangeMilliMeter);
        ESP_LOGI(LOG_TAG, "Sensor 2 Distance (mm): %d", measure2.RangeMilliMeter);
        digitalWrite(PIN_LED_RED, LOW);
        sensor1Initialized = true;
        sensor2Initialized = true;
    } else {
        ESP_LOGW(LOG_TAG, "One or both sensors out of range");
        digitalWrite(PIN_LED_RED, HIGH);
    }
}

void setup() {
    // Initialize USB CDC Serial
    Serial.begin(115200);
    
    // Wait for USB CDC with timeout
    uint32_t startTime = millis();
    while (!Serial && (millis() - startTime < 2000)) {
        yield();
    }
    
    // Set logging levels
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set(LOG_TAG, ESP_LOG_INFO);
    
    ESP_LOGI(LOG_TAG, "Starting setup...");
    
    // Initialize GPIO
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_SENSOR1_XSHUT, OUTPUT);
    pinMode(PIN_SENSOR2_XSHUT, OUTPUT);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_SENSOR1_XSHUT, LOW); // Start with sensors off
    digitalWrite(PIN_SENSOR2_XSHUT, LOW);
    ESP_LOGI(LOG_TAG, "GPIO pins initialized");
    
    // Initialize I2C with lower speed
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
    ESP_LOGI(LOG_TAG, "I2C initialized at 100kHz");
    
    ESP_LOGI(LOG_TAG, "VL53L0X Single Sensor Test");
    ESP_LOGI(LOG_TAG, "========================");
    
    delay(500); // Give time for everything to settle
    
    // Initialize VL53L0X
    initVL53L0X();
    
    ESP_LOGI(LOG_TAG, "Setup complete");
}

void loop() {
    if (!sensor1Initialized || !sensor2Initialized) {
        ESP_LOGW(LOG_TAG, "One or both sensors not initialized, retrying...");
        initVL53L0X();
        delay(1000);
        return;
    }
    
    // Test measurements every second
    VL53L0X_RangingMeasurementData_t measure1, measure2;
    
    sensor1.rangingTest(&measure1, false);
    sensor2.rangingTest(&measure2, false);
    
    if (measure1.RangeStatus != 4 && measure2.RangeStatus != 4) {
        ESP_LOGI(LOG_TAG, "Sensor 1 Distance (mm): %d", measure1.RangeMilliMeter);
        ESP_LOGI(LOG_TAG, "Sensor 2 Distance (mm): %d", measure2.RangeMilliMeter);
        // Light LED if either sensor detects something close
        digitalWrite(PIN_LED_RED, 
            (measure1.RangeMilliMeter < 100) || 
            (measure2.RangeMilliMeter < 100));
    } else {
        ESP_LOGW(LOG_TAG, "One or both sensors out of range");
        digitalWrite(PIN_LED_RED, HIGH);
    }
    
    delay(1000);
}


