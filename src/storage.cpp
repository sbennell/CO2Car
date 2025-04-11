#include "storage.h"

const char* Storage::RESULTS_FILE = "/race_results.json";

bool Storage::begin() {
    // Initialize SPI for SD Card using default SPI0
    SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    
    ESP_LOGI(LOG_TAG_STORAGE, "Initializing SD card with pins: SCK=%d, MISO=%d, MOSI=%d, CS=%d",
             PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    
    // Configure SPI settings
    SPI.setFrequency(4000000); // Set to 4MHz for stability
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    // Initialize SD card
    if (!SD.begin(PIN_SD_CS, SPI)) {
        ESP_LOGE(LOG_TAG_STORAGE, "SD card initialization failed!");
        ESP_LOGE(LOG_TAG_STORAGE, "Please check:");
        ESP_LOGE(LOG_TAG_STORAGE, "1. SD card is properly inserted");
        ESP_LOGE(LOG_TAG_STORAGE, "2. SD card is formatted (FAT32)");
        ESP_LOGE(LOG_TAG_STORAGE, "3. All pins are properly connected");
        return false;
    }
    
    ESP_LOGI(LOG_TAG_STORAGE, "SD card initialized successfully");
    
    // Create results directory if it doesn't exist
    if (!ensureDirectory("/results")) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to create results directory!");
        return false;
    }
    
    // Check if results file exists, create if not
    if (!SD.exists(RESULTS_FILE)) {
        File file = SD.open(RESULTS_FILE, FILE_WRITE);
        if (!file) {
            ESP_LOGE(LOG_TAG_STORAGE, "Failed to create results file!");
            return false;
        }
        // Initialize with empty JSON array
        file.println("[]");
        file.close();
    }
    
    return true;
}

bool Storage::saveRaceResult(const RaceResult& result) {
    StaticJsonDocument<256> doc;
    
    doc["timestamp"] = result.timestamp;
    doc["startTime"] = result.startTime;
    doc["finishTime"] = result.finishTime;
    doc["elapsedTime"] = result.elapsedTime;
    doc["lane1Time"] = result.lane1Time;
    doc["lane2Time"] = result.lane2Time;
    doc["winningLane"] = result.winningLane;
    doc["isTie"] = result.isTie;
    
    return appendToJsonArray(RESULTS_FILE, doc);
}

bool Storage::loadRaceResults(RaceResult* results, size_t maxResults, size_t& actualResults) {
    File file = SD.open(RESULTS_FILE, FILE_READ);
    if (!file) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to open results file!");
        return false;
    }
    
    DynamicJsonDocument doc(16384); // Adjust size based on expected number of results
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to parse results file: %s", error.c_str());
        return false;
    }
    
    JsonArray array = doc.as<JsonArray>();
    actualResults = min(array.size(), maxResults);
    
    for (size_t i = 0; i < actualResults; i++) {
        JsonObject obj = array[i];
        results[i].timestamp = obj["timestamp"] | 0;
        results[i].startTime = obj["startTime"] | 0;
        results[i].finishTime = obj["finishTime"] | 0;
        results[i].elapsedTime = obj["elapsedTime"] | 0;
        results[i].lane1Time = obj["lane1Time"] | 0;
        results[i].lane2Time = obj["lane2Time"] | 0;
        results[i].winningLane = obj["winningLane"] | 0;
        results[i].isTie = obj["isTie"] | false;
    }
    
    return true;
}

bool Storage::clearRaceResults() {
    if (!SD.remove(RESULTS_FILE)) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to delete results file!");
        return false;
    }
    
    File file = SD.open(RESULTS_FILE, FILE_WRITE);
    if (!file) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to create new results file!");
        return false;
    }
    
    file.println("[]");
    file.close();
    return true;
}

bool Storage::ensureDirectory(const char* path) {
    if (!SD.exists(path)) {
        if (!SD.mkdir(path)) {
            ESP_LOGE(LOG_TAG_STORAGE, "Failed to create directory: %s", path);
            return false;
        }
    }
    return true;
}

bool Storage::appendToJsonArray(const char* filename, const JsonDocument& doc) {
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to open file for reading: %s", filename);
        return false;
    }
    
    DynamicJsonDocument existingDoc(16384);
    DeserializationError error = deserializeJson(existingDoc, file);
    file.close();
    
    if (error) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to parse existing file: %s", error.c_str());
        return false;
    }
    
    JsonArray array = existingDoc.as<JsonArray>();
    array.add(doc);
    
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to open file for writing: %s", filename);
        return false;
    }
    
    if (serializeJson(existingDoc, file) == 0) {
        ESP_LOGE(LOG_TAG_STORAGE, "Failed to write to file: %s", filename);
        file.close();
        return false;
    }
    
    file.close();
    return true;
}
