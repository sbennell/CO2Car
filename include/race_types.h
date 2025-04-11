#ifndef RACE_TYPES_H
#define RACE_TYPES_H

#include <Arduino.h>
#include <time.h>

// Race state and event type enumerations
enum RaceState {
    RACE_IDLE,
    RACE_READY,
    RACE_RUNNING,
    RACE_FINISHED
};

enum RaceEventType {
    RACE_START,
    RACE_FINISH,
    RACE_CANCEL
};

// Sensor event structure
struct SensorEvent {
    uint8_t sensorId;
    uint16_t distance;
    uint32_t timestamp;
};

// Race result structure
struct RaceResult {
    uint32_t timestamp;      // Unix timestamp
    uint32_t startTime;      // Race start time in millis
    uint32_t finishTime;     // Race finish time in millis
    uint32_t elapsedTime;    // Total race time in millis
    uint16_t lane1Time;      // Lane 1 time in millis
    uint16_t lane2Time;      // Lane 2 time in millis
    uint8_t winningLane;     // 1 or 2, 0 for tie
    bool isTie;             // True if race was a tie
};

// Race event structure
struct RaceEvent {
    RaceEventType type;
    uint32_t timestamp;
    uint16_t lane1Time;
    uint16_t lane2Time;
};

#endif // RACE_TYPES_H
