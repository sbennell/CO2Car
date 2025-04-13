#include "SDCardManager.h"

SDCardManager::SDCardManager() : initialized(false), lastError("") {}

bool SDCardManager::begin() {
    if (initialized) {
        return true;
    }

    // Initialize SPI with custom pins
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    if (!SD.begin(SD_CS_PIN)) {
        lastError = "SD card initialization failed";
        return false;
    }

    // Create races directory if it doesn't exist
    if (!SD.exists("/races")) {
        if (!SD.mkdir("/races")) {
            lastError = "Failed to create races directory";
            return false;
        }
    }

    initialized = true;
    return true;
}

bool SDCardManager::saveRaceData(const JsonDocument& data) {
    if (!initialized) {
        lastError = "SD card not initialized";
        return false;
    }

    // Generate filename with timestamp
    char filename[32];
    snprintf(filename, sizeof(filename), "/races/race_%lu.json", millis());

    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        lastError = "Failed to open file for writing";
        return false;
    }

    if (serializeJson(data, file) == 0) {
        lastError = "Failed to write data to file";
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool SDCardManager::readRaceData(JsonDocument& data, const char* filename) {
    if (!initialized) {
        lastError = "SD card not initialized";
        return false;
    }

    File file = SD.open(filename);
    if (!file) {
        lastError = "Failed to open file for reading";
        return false;
    }

    DeserializationError error = deserializeJson(data, file);
    file.close();

    if (error) {
        lastError = "Failed to parse JSON data";
        return false;
    }

    return true;
}

bool SDCardManager::listFiles() {
    if (!initialized) {
        lastError = "SD card not initialized";
        return false;
    }

    File root = SD.open("/races");
    if (!root) {
        lastError = "Failed to open races directory";
        return false;
    }

    if (!root.isDirectory()) {
        lastError = "Not a directory";
        root.close();
        return false;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            Serial.print("File: ");
            Serial.println(file.name());
        }
        file = root.openNextFile();
    }

    root.close();
    return true;
}

bool SDCardManager::removeFile(const char* filename) {
    if (!initialized) {
        lastError = "SD card not initialized";
        return false;
    }

    if (!SD.exists(filename)) {
        lastError = "File does not exist";
        return false;
    }

    if (!SD.remove(filename)) {
        lastError = "Failed to remove file";
        return false;
    }

    return true;
}

bool SDCardManager::formatCard() {
    if (!initialized) {
        lastError = "SD card not initialized";
        return false;
    }

    // Note: This is a destructive operation
    // Implementation depends on the specific SD card library capabilities
    lastError = "Format operation not implemented";
    return false;
}

bool SDCardManager::isCardPresent() {
    return initialized && SD.cardSize() > 0;
}

String SDCardManager::getLastError() {
    return lastError;
} 