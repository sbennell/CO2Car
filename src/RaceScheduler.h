#pragma once

#include <vector>
#include <string>
#include <ArduinoJson.h>
#include <SD.h>

struct Race {
    int round;
    int heat;
    int lane1Racer;  // Racer ID for lane 1
    int lane2Racer;  // Racer ID for lane 2
    bool completed;
    unsigned long scheduledTime;  // Unix timestamp
};

struct Racer {
    int id;
    String name;
    bool checkedIn;
    int totalRaces;
    int lane1Races;
    int lane2Races;
};

class RaceScheduler {
public:
    RaceScheduler();
    void begin();
    
    // Racer management
    bool addRacer(const String& name);
    bool removeRacer(int racerId);
    bool checkInRacer(int racerId);
    std::vector<Racer> getCheckedInRacers() const;
    
    // Schedule generation
    bool generateSchedule();  // Creates a Perfect-N schedule for checked-in racers
    std::vector<Race> getUpcomingRaces(int count = 5) const;
    Race getCurrentRace() const;
    bool markRaceComplete(int round, int heat);
    
    // Schedule management
    void clearSchedule();
    bool saveToFile();
    bool loadFromFile();
    
private:
    std::vector<Racer> racers;
    std::vector<Race> schedule;
    int currentRound;
    int currentHeat;
    
    // Perfect-N algorithm helpers
    void generatePerfectN(int n);
    void balanceLaneAssignments();
    bool validateSchedule() const;
    
    // File operations
    static const char* SCHEDULE_FILE;
    static const char* RACERS_FILE;
};
