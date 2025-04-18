#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>
#include "TimeManager.h"

struct RaceResult {
    unsigned long timestamp;
    float lane1Time;
    float lane2Time;
    int winner;
};

class RaceHistory {
public:
    RaceHistory(TimeManager& timeManager);
    void begin();
    void addRace(float lane1Time, float lane2Time);
    void getHistory(JsonDocument& doc, int limit = 10);
    void clear();

private:
    static const char* HISTORY_FILE;
    std::vector<RaceResult> races;
    TimeManager& timeManager;
    void loadFromFile();
    void saveToFile();
};
