#include "TimeManager.h"
#include <Arduino.h>

TimeManager::TimeManager() : timeSet(false), lastNTPUpdate(0) {}

void TimeManager::begin() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Wait for time to be set (timeout after 10 seconds)
    int attempts = 0;
    while (!isTimeSet() && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (isTimeSet()) {
        Serial.println("\n✅ NTP time synchronized");
        timeSet = true;
        lastNTPUpdate = millis();
    } else {
        Serial.println("\n❌ Failed to get time from NTP server");
    }
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
    // Update NTP time every hour
    if (millis() - lastNTPUpdate >= NTP_UPDATE_INTERVAL) {
        Serial.println("🕒 Updating NTP time...");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        lastNTPUpdate = millis();
    }
}
