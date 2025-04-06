#include "RaceHistory.h"
#include <time.h>

const char* RaceHistory::HISTORY_FILE = "/race_history.json";

bool ensureFileExists() {
    if (!LittleFS.exists("/race_history.json")) {
        Serial.println("📄 Creating new race history file...");
        File file = LittleFS.open("/race_history.json", "w");
        if (!file) {
            Serial.println("❌ Failed to create race history file");
            return false;
        }
        // Write empty array
        file.print("[]");
        file.close();
        Serial.println("✅ Created new race history file");
    } else {
        Serial.println("✅ Race history file exists");
        // Verify file is valid JSON array
        File file = LittleFS.open("/race_history.json", "r");
        if (!file) {
            Serial.println("❌ Failed to open race history file");
            return false;
        }
        String content = file.readString();
        file.close();
        
        if (content.length() == 0) {
            Serial.println("❌ Race history file is empty, recreating...");
            file = LittleFS.open("/race_history.json", "w");
            file.print("[]");
            file.close();
        }
    }
    return true;
}

RaceHistory::RaceHistory() {}

void RaceHistory::begin() {
    if (!ensureFileExists()) {
        Serial.println("❌ Failed to initialize race history");
        return;
    }
    loadFromFile();
}

void RaceHistory::addRace(float lane1Time, float lane2Time) {
    RaceResult result;
    result.timestamp = time(nullptr);
    result.lane1Time = lane1Time;
    result.lane2Time = lane2Time;
    
    // Consider times within 0.002 seconds (2ms) as a tie
    float timeDiff = abs(lane1Time - lane2Time);
    if (timeDiff <= 0.002) {
        result.winner = 0; // Tie
    } else {
        result.winner = (lane1Time < lane2Time) ? 1 : 2;
    }
    
    races.push_back(result);
    if (races.size() > 50) { // Keep only last 50 races
        races.erase(races.begin());
    }
    saveToFile();
}

void RaceHistory::getHistory(JsonDocument& doc, int limit) {
    doc.clear();
    JsonArray array = doc.to<JsonArray>();
    
    int count = 0;
    for (auto it = races.rbegin(); it != races.rend() && count < limit; ++it, ++count) {
        JsonObject race = array.createNestedObject();
        race["timestamp"] = it->timestamp;
        race["lane1"] = it->lane1Time;
        race["lane2"] = it->lane2Time;
        race["winner"] = it->winner;
    }
}

void RaceHistory::clear() {
    races.clear();
    saveToFile();
}

void RaceHistory::loadFromFile() {
    races.clear();
    
    File file = LittleFS.open(HISTORY_FILE, "r");
    if (!file) {
        Serial.println("❌ Failed to open race history file for reading");
        return;
    }
    
    String content = file.readString();
    file.close();
    
    if (content.length() == 0) {
        Serial.println("❌ Race history file is empty");
        return;
    }
    
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, content);
    
    if (error) {
        Serial.print("❌ Failed to parse race history: ");
        Serial.println(error.c_str());
        return;
    }
    
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject raceObj : array) {
        RaceResult result;
        result.timestamp = raceObj["timestamp"] | 0;
        result.lane1Time = raceObj["lane1"] | 0.0f;
        result.lane2Time = raceObj["lane2"] | 0.0f;
        result.winner = raceObj["winner"] | 0;
        races.push_back(result);
    }
    
    Serial.print("✅ Loaded ");
    Serial.print(races.size());
    Serial.println(" races from history");
}

void RaceHistory::saveToFile() {
    File file = LittleFS.open(HISTORY_FILE, "w");
    if (!file) {
        Serial.println("❌ Failed to open race history file for writing");
        return;
    }
    
    StaticJsonDocument<4096> doc;
    JsonArray array = doc.to<JsonArray>();
    
    for (const auto& race : races) {
        JsonObject raceObj = array.createNestedObject();
        raceObj["timestamp"] = race.timestamp;
        raceObj["lane1"] = race.lane1Time;
        raceObj["lane2"] = race.lane2Time;
        raceObj["winner"] = race.winner;
    }
    
    String output;
    serializeJson(doc, output);
    file.print(output);
    file.close();
    
    Serial.print("✅ Saved ");
    Serial.print(races.size());
    Serial.println(" races to history");
}
