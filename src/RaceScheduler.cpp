#include "RaceScheduler.h"

const char* RaceScheduler::SCHEDULE_FILE = "/race_data/schedule.json";
const char* RaceScheduler::RACERS_FILE = "/race_data/racers.json";

static bool ensureDirectory(const char* path) {
    if (!SD.exists(path)) {
        if (!SD.mkdir(path)) {
            Serial.printf("❌ Failed to create directory: %s\n", path);
            return false;
        }
        Serial.printf("✅ Created directory: %s\n", path);
    }
    return true;
}

RaceScheduler::RaceScheduler() : currentRound(0), currentHeat(0) {}

void RaceScheduler::begin() {
    // Initialize SD card if not already initialized
    if (!SD.begin()) {
        Serial.println("❌ Failed to initialize SD card");
        return;
    }
    Serial.println("✅ SD card initialized");

    // Ensure race_data directory exists
    if (!ensureDirectory("/race_data")) {
        Serial.println("❌ Failed to initialize race data directory");
        return;
    }

    if (!loadFromFile()) {
        Serial.println("ℹ️ No existing schedule found, starting fresh");
    } else {
        Serial.println("✅ Loaded existing race data");
    }
}

bool RaceScheduler::addRacer(const String& name) {
    // Validate input
    if (name.length() == 0 || name.length() > 50) {
        Serial.println("❌ Invalid racer name length");
        return false;
    }

    // Check if name already exists
    for (const auto& racer : racers) {
        if (racer.name == name) {
            Serial.println("❌ Racer name already exists");
            return false;
        }
    }

    // Check maximum racers limit
    if (racers.size() >= 50) {
        Serial.println("❌ Maximum number of racers reached");
        return false;
    }
    
    try {
        Racer newRacer;
        newRacer.id = racers.empty() ? 1 : (racers.back().id + 1);
        newRacer.name = name;
        newRacer.checkedIn = false;
        newRacer.totalRaces = 0;
        newRacer.lane1Races = 0;
        newRacer.lane2Races = 0;
        
        racers.push_back(newRacer);
        
        if (!saveToFile()) {
            racers.pop_back();  // Rollback if save fails
            Serial.println("❌ Failed to save racer data");
            return false;
        }
        
        Serial.print("✅ Added racer: ");
        Serial.println(name);
        return true;
    } catch (const std::exception& e) {
        Serial.print("❌ Error adding racer: ");
        Serial.println(e.what());
        return false;
    }
}

bool RaceScheduler::removeRacer(int racerId) {
    for (auto it = racers.begin(); it != racers.end(); ++it) {
        if (it->id == racerId) {
            racers.erase(it);
            saveToFile();
            return true;
        }
    }
    return false;
}

bool RaceScheduler::checkInRacer(int racerId) {
    for (auto& racer : racers) {
        if (racer.id == racerId) {
            racer.checkedIn = true;
            saveToFile();
            return true;
        }
    }
    return false;
}

std::vector<Racer> RaceScheduler::getCheckedInRacers() const {
    std::vector<Racer> checkedIn;
    for (const auto& racer : racers) {
        if (racer.checkedIn) {
            checkedIn.push_back(racer);
        }
    }
    return checkedIn;
}

bool RaceScheduler::generateSchedule() {
    auto checkedInRacers = getCheckedInRacers();
    if (checkedInRacers.size() < 2) {
        Serial.println("❌ Need at least 2 racers checked in to generate schedule");
        return false;
    }
    
    clearSchedule();
    generatePerfectN(checkedInRacers.size());
    balanceLaneAssignments();
    
    if (!validateSchedule()) {
        Serial.println("❌ Generated schedule failed validation");
        clearSchedule();
        return false;
    }
    
    saveToFile();
    return true;
}

void RaceScheduler::generatePerfectN(int n) {
    // Implementation of Young and Pope's Perfect-N algorithm
    // Each racer races against every other racer exactly once
    // Total number of races = n * (n-1) / 2
    
    int totalRaces = (n * (n-1)) / 2;
    int racesPerRound = n / 2;
    int totalRounds = n - 1;
    
    auto checkedInRacers = getCheckedInRacers();
    std::vector<int> racerIds;
    for (const auto& racer : checkedInRacers) {
        racerIds.push_back(racer.id);
    }
    
    // If odd number of racers, add a "bye" racer (id = 0)
    if (n % 2 != 0) {
        racerIds.push_back(0);
        n++;
    }
    
    // Generate rounds using circle method
    for (int round = 0; round < totalRounds; round++) {
        for (int i = 0; i < n/2; i++) {
            Race race;
            race.round = round + 1;
            race.heat = i + 1;
            race.lane1Racer = racerIds[i];
            race.lane2Racer = racerIds[n-1-i];
            race.completed = false;
            race.scheduledTime = 0; // Will be set when finalizing schedule
            
            // Don't add races with bye racer
            if (race.lane1Racer != 0 && race.lane2Racer != 0) {
                schedule.push_back(race);
            }
        }
        
        // Rotate racers (keeping first racer fixed)
        int last = racerIds[n-1];
        for (int i = n-1; i > 1; i--) {
            racerIds[i] = racerIds[i-1];
        }
        racerIds[1] = last;
    }
}

void RaceScheduler::balanceLaneAssignments() {
    // Count lane assignments for each racer
    std::vector<std::pair<int, int>> laneCounts; // pair of {lane1count, lane2count}
    for (const auto& racer : racers) {
        int lane1 = 0, lane2 = 0;
        for (const auto& race : schedule) {
            if (race.lane1Racer == racer.id) lane1++;
            if (race.lane2Racer == racer.id) lane2++;
        }
        laneCounts.push_back({lane1, lane2});
    }
    
    // Swap lanes in races to balance assignments
    for (auto& race : schedule) {
        int racer1Idx = race.lane1Racer - 1;
        int racer2Idx = race.lane2Racer - 1;
        
        if (laneCounts[racer1Idx].first > laneCounts[racer1Idx].second &&
            laneCounts[racer2Idx].second > laneCounts[racer2Idx].first) {
            // Swap lanes
            std::swap(race.lane1Racer, race.lane2Racer);
            laneCounts[racer1Idx].first--;
            laneCounts[racer1Idx].second++;
            laneCounts[racer2Idx].first++;
            laneCounts[racer2Idx].second--;
        }
    }
}

bool RaceScheduler::validateSchedule() const {
    if (schedule.empty()) return false;
    
    // Check that each racer races against every other racer exactly once
    auto checkedIn = getCheckedInRacers();
    int n = checkedIn.size();
    std::vector<std::vector<bool>> raced(n, std::vector<bool>(n, false));
    
    for (const auto& race : schedule) {
        int idx1 = race.lane1Racer - 1;
        int idx2 = race.lane2Racer - 1;
        
        if (idx1 < 0 || idx2 < 0 || idx1 >= n || idx2 >= n) {
            Serial.println("❌ Invalid racer ID in schedule");
            return false;
        }
        
        if (raced[idx1][idx2] || raced[idx2][idx1]) {
            Serial.println("❌ Duplicate race found in schedule");
            return false;
        }
        
        raced[idx1][idx2] = raced[idx2][idx1] = true;
    }
    
    // Verify all racers race against each other
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (!raced[i][j]) {
                Serial.printf("❌ Racers %d and %d never race against each other\n", 
                            checkedIn[i].id, checkedIn[j].id);
                return false;
            }
        }
    }
    
    return true;
}

std::vector<Race> RaceScheduler::getUpcomingRaces(int count) const {
    std::vector<Race> upcoming;
    for (const auto& race : schedule) {
        if (!race.completed) {
            upcoming.push_back(race);
            if (upcoming.size() >= count) break;
        }
    }
    return upcoming;
}

Race RaceScheduler::getCurrentRace() const {
    for (const auto& race : schedule) {
        if (!race.completed) return race;
    }
    return Race(); // Return empty race if no current race
}

bool RaceScheduler::markRaceComplete(int round, int heat) {
    for (auto& race : schedule) {
        if (race.round == round && race.heat == heat) {
            race.completed = true;
            saveToFile();
            return true;
        }
    }
    return false;
}

void RaceScheduler::clearSchedule() {
    schedule.clear();
    currentRound = 0;
    currentHeat = 0;
    saveToFile();
}

bool RaceScheduler::saveToFile() {
    // Verify SD card is working
    if (!SD.exists("/race_data")) {
        Serial.println("❌ Race data directory not found. Please initialize SD card first.");
        return false;
    }

    // Remove existing file if it exists
    if (SD.exists(RACERS_FILE)) {
        if (!SD.remove(RACERS_FILE)) {
            Serial.println("❌ Failed to remove existing racers file");
            return false;
        }
    }

    // Create new file
    File file = SD.open(RACERS_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("❌ Failed to create racers file");
        return false;
    }

    // Create JSON document for racers
    StaticJsonDocument<4096> racerDoc;
    JsonArray racerArray = racerDoc.to<JsonArray>();
    
    for (const auto& racer : racers) {
        JsonObject racerObj = racerArray.createNestedObject();
        racerObj["id"] = racer.id;
        racerObj["name"] = racer.name;
        racerObj["checkedIn"] = racer.checkedIn;
        racerObj["totalRaces"] = racer.totalRaces;
        racerObj["lane1Races"] = racer.lane1Races;
        racerObj["lane2Races"] = racer.lane2Races;
    }
    
    if (serializeJson(racerDoc, file) == 0) {
        Serial.println("❌ Failed to write racers file");
        file.close();
        return false;
    }
    
    file.close();
    
    // Save schedule
    File scheduleFile = SD.open(SCHEDULE_FILE, FILE_WRITE);
    if (!scheduleFile) {
        Serial.println("❌ Failed to open schedule file for writing");
        return false;
    }
    
    StaticJsonDocument<4096> scheduleDoc;
    JsonArray scheduleArray = scheduleDoc.to<JsonArray>();
    
    for (const auto& race : schedule) {
        JsonObject raceObj = scheduleArray.createNestedObject();
        raceObj["round"] = race.round;
        raceObj["heat"] = race.heat;
        raceObj["lane1Racer"] = race.lane1Racer;
        raceObj["lane2Racer"] = race.lane2Racer;
        raceObj["completed"] = race.completed;
        raceObj["scheduledTime"] = race.scheduledTime;
    }
    
    if (serializeJson(scheduleDoc, scheduleFile) == 0) {
        Serial.println("❌ Failed to write schedule file");
        return false;
    }
    scheduleFile.close();
    
    return true;
}

bool RaceScheduler::loadFromFile() {
    // Load racers
    File racerFile = SD.open(RACERS_FILE, FILE_READ);
    if (racerFile) {
        StaticJsonDocument<2048> racerDoc;
        DeserializationError error = deserializeJson(racerDoc, racerFile);
        racerFile.close();
        
        if (error) {
            Serial.println("❌ Failed to parse racers file");
            return false;
        }
        
        racers.clear();
        JsonArray racerArray = racerDoc.as<JsonArray>();
        for (JsonObject racerObj : racerArray) {
            Racer racer;
            racer.id = racerObj["id"];
            racer.name = racerObj["name"].as<String>();
            racer.checkedIn = racerObj["checkedIn"];
            racer.totalRaces = racerObj["totalRaces"];
            racer.lane1Races = racerObj["lane1Races"];
            racer.lane2Races = racerObj["lane2Races"];
            racers.push_back(racer);
        }
    }
    
    // Load schedule
    File scheduleFile = SD.open(SCHEDULE_FILE, FILE_READ);
    if (scheduleFile) {
        StaticJsonDocument<4096> scheduleDoc;
        DeserializationError error = deserializeJson(scheduleDoc, scheduleFile);
        scheduleFile.close();
        
        if (error) {
            Serial.println("❌ Failed to parse schedule file");
            return false;
        }
        
        schedule.clear();
        JsonArray scheduleArray = scheduleDoc.as<JsonArray>();
        for (JsonObject raceObj : scheduleArray) {
            Race race;
            race.round = raceObj["round"];
            race.heat = raceObj["heat"];
            race.lane1Racer = raceObj["lane1Racer"];
            race.lane2Racer = raceObj["lane2Racer"];
            race.completed = raceObj["completed"];
            race.scheduledTime = raceObj["scheduledTime"];
            schedule.push_back(race);
        }
    }
    
    return true;
}
