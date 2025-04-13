#include "TimeManager.h"
#include <Arduino.h>

TimeManager::TimeManager() : timeClient(ntpUDP, "pool.ntp.org"), timeSynced(false) {}

void TimeManager::begin() {
    timeClient.begin();
    timeClient.setTimeOffset(0); // UTC
    timeSynced = false;
}

void TimeManager::setTimezone(const char* tz) {
    setenv("TZ", tz, 1);
    tzset();
}

time_t TimeManager::getEpochTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

bool TimeManager::isTimeSet() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    return timeinfo.tm_year > (2024 - 1900);  // Check if year is at least 2024
}

void TimeManager::update() {
    timeClient.update();
    if (!timeSynced && timeClient.isTimeSet()) {
        timeSynced = true;
    }
}

String TimeManager::getFormattedTime() {
    if (!timeSynced) {
        return "Time not synced";
    }
    
    time_t now = timeClient.getEpochTime();
    struct tm *timeinfo = localtime(&now);
    
    char timeString[20];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(timeString);
}

bool TimeManager::isTimeSynced() {
    return timeSynced;
}
