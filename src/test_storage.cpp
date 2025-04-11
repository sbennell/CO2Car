#include <Arduino.h>
#include "storage.h"
#include "esp_log.h"

#define LOG_TAG "STORAGE_TEST"

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    ESP_LOGI(LOG_TAG, "Starting Storage Test...");
    
    // Initialize SD card
    if (!Storage::begin()) {
        ESP_LOGE(LOG_TAG, "Storage initialization failed!");
        return;
    }
    
    // Create a test race result
    RaceResult testResult = {
        .timestamp = 1681199400,  // Example timestamp
        .startTime = 1000,
        .finishTime = 3500,
        .elapsedTime = 2500,
        .lane1Time = 2500,
        .lane2Time = 2600,
        .winningLane = 1,
        .isTie = false
    };
    
    // Save the test result
    if (!Storage::saveRaceResult(testResult)) {
        ESP_LOGE(LOG_TAG, "Failed to save race result!");
        return;
    }
    ESP_LOGI(LOG_TAG, "Test result saved successfully");
    
    // Read back all results
    RaceResult results[10];
    size_t actualResults;
    
    if (!Storage::loadRaceResults(results, 10, actualResults)) {
        ESP_LOGE(LOG_TAG, "Failed to load race results!");
        return;
    }
    
    // Print results
    ESP_LOGI(LOG_TAG, "Loaded %d results:", actualResults);
    for (size_t i = 0; i < actualResults; i++) {
        ESP_LOGI(LOG_TAG, "Result %d:", i + 1);
        ESP_LOGI(LOG_TAG, "  Timestamp: %lu", results[i].timestamp);
        ESP_LOGI(LOG_TAG, "  Elapsed Time: %lu ms", results[i].elapsedTime);
        ESP_LOGI(LOG_TAG, "  Lane 1: %u ms", results[i].lane1Time);
        ESP_LOGI(LOG_TAG, "  Lane 2: %u ms", results[i].lane2Time);
        ESP_LOGI(LOG_TAG, "  Winner: Lane %u", results[i].winningLane);
        ESP_LOGI(LOG_TAG, "  Tie: %s", results[i].isTie ? "Yes" : "No");
    }
    
    ESP_LOGI(LOG_TAG, "Storage test complete!");
}

void loop() {
    delay(1000);
}
