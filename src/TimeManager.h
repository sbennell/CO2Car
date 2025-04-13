#pragma once

#include <time.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeManager {
public:
    TimeManager();
    void begin();
    void setTimezone(const char* tz);
    time_t getEpochTime();
    bool isTimeSet();
    void update();
    String getFormattedTime();
    bool isTimeSynced();

private:
    bool timeSet;
    const char* ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 36000;     // GMT+10 (Sydney)
    const int daylightOffset_sec = 3600;   // +1 hour for DST
    unsigned long lastNTPUpdate;
    const unsigned long NTP_UPDATE_INTERVAL = 3600000; // Update every hour
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    bool timeSynced;
};
