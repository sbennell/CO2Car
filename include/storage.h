#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "esp_log.h"
#include "race_types.h"
#include "pin_config.h"

#define LOG_TAG_STORAGE "STORAGE"





class Storage {
public:
    static bool begin();
    static bool saveRaceResult(const RaceResult& result);
    static bool loadRaceResults(RaceResult* results, size_t maxResults, size_t& actualResults);
    static bool clearRaceResults();
    
private:
    static const char* RESULTS_FILE;
    static bool ensureDirectory(const char* path);
    static bool appendToJsonArray(const char* filename, const JsonDocument& doc);
};

#endif // STORAGE_H
