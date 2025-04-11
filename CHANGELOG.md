# CO₂ Car Race Timer for ESP32 – Changelog

All notable changes to this project will be documented in this file.

---

## [0.11.3] - 2025-04-11
### Added
- **Race Control**:
  - Implemented complete race sequence with relay trigger
  - Added proper finish line detection for both lanes
  - Improved state management for race progression
- **LED Status**:
  - Updated LED indicators for all race states
  - Added buzzer feedback for race completion

## [0.11.2] - 2025-04-11
### Added
- **Web Server Implementation**:
  - Set up ESP32 as web server using AsyncWebServer
  - Configured WebSocket server for real-time updates
  - Initialized SPIFFS for web file storage
- **Network Resilience**:
  - Implemented Access Point (AP) fallback mode
  - Added captive portal with DNS server
  - Auto-switching between AP and Station modes
  - AP status indicators in web interface

### Fixed
- **Build System**:
  - Fixed WiFi manager integration
  - Corrected race event type definitions
  - Added missing function declarations

## [0.11.1] - 2025-04-11
### Added
- **SD Card Integration**:
  - Implemented SD card initialization and testing
  - Added race result storage functionality
  - Proper SPI pin configuration for SD card interface
  - Error handling and status reporting for SD operations
- **Build System**:
  - Added separate build environments for main app and testing
  - Improved dependency management

## [0.11.0] - 2025-04-11
Started again with FreeRTOS Implementation
### Added
- **FreeRTOS Implementation**:
  - Sensor task for handling dual VL53L0X sensors (50Hz sampling)
  - Timer task for precise race timing (100Hz update)
  - Race control task for state management
  - Web server task for future UI implementation
- **Enhanced Sensor Management**:
  - Proper I2C mutex handling
  - Improved sensor initialization sequence
  - Faster measurement timing (20ms budget)
- **Race State Management**:
  - Clear state transitions (IDLE → READY → RUNNING → FINISHED)
  - Event-based communication between tasks
  - Protected race state access with mutex

---