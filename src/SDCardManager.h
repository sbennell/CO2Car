#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

class SDCardManager {
public:
    SDCardManager();
    bool begin();
    bool saveRaceData(const JsonDocument& data);
    bool readRaceData(JsonDocument& data, const char* filename);
    bool listFiles();
    bool removeFile(const char* filename);
    bool formatCard();
    bool isCardPresent();
    String getLastError();

private:
    bool initialized;
    String lastError;
    static const int SD_CS_PIN = 5;  // CS pin for SD card
    static const int SD_MOSI_PIN = 23;
    static const int SD_MISO_PIN = 19;
    static const int SD_SCK_PIN = 18;
};

#endif // SDCARD_MANAGER_H 