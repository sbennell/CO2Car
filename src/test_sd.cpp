#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "esp_log.h"

#define LOG_TAG "SD_TEST"
#define PIN_SD_SCK  12
#define PIN_SD_MISO 13
#define PIN_SD_MOSI 11
#define PIN_SD_CS   10

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\nSD Card Test Starting...");
    ESP_LOGI(LOG_TAG, "SPI Pins: SCK=%d, MISO=%d, MOSI=%d, CS=%d", 36, 37, 35, 38);
    
    // Initialize SPI for SD Card
    SPIClass spi = SPIClass(HSPI);
    spi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS); // SCK, MISO, MOSI, CS
    ESP_LOGI(LOG_TAG, "SPI initialized with pins: SCK=%d, MISO=%d, MOSI=%d, CS=%d",
             PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    
    if(!SD.begin(PIN_SD_CS, spi)) {
        ESP_LOGE(LOG_TAG, "SD Card initialization failed!");
        ESP_LOGE(LOG_TAG, "Please check:");
        ESP_LOGE(LOG_TAG, "1. SD card is properly inserted");
        ESP_LOGE(LOG_TAG, "2. SD card is formatted (FAT32)");
        ESP_LOGE(LOG_TAG, "3. All pins are properly connected");
        return;
    }
    
    ESP_LOGI(LOG_TAG, "SD Card initialized successfully");
    
    // Create a test file
    File testFile = SD.open("/test.txt", FILE_WRITE);
    if (!testFile) {
        ESP_LOGE(LOG_TAG, "Failed to create test file!");
        return;
    }
    
    testFile.println("Hello from ESP32-S3!");
    testFile.close();
    
    // Read back the file
    testFile = SD.open("/test.txt");
    if (!testFile) {
        ESP_LOGE(LOG_TAG, "Failed to open test file for reading!");
        return;
    }
    
    ESP_LOGI(LOG_TAG, "File contents:");
    while (testFile.available()) {
        Serial.write(testFile.read());
    }
    testFile.close();
    
    ESP_LOGI(LOG_TAG, "SD Card test complete!");
}

void loop() {
    delay(1000);
}
