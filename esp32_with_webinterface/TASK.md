# CO2 Car Race Timer Project Tasks

## Hardware Setup Tasks

### ESP32 Setup
- [x] Set up ESP32 development environment (Arduino IDE or PlatformIO)
- [x] Test basic ESP32 functionality with blink sketch
- [x] Configure necessary libraries for ESP32
- [x] Test WiFi connectivity for web interface

### Sensor Setup
- [x] Connect first GY-VL53L0XV2 sensor to ESP32
- [x] Test basic distance measurement functionality
- [x] Connect second GY-VL53L0XV2 sensor (with address modification)
- [x] Test simultaneous reading from both sensors
- [x] Calibrate sensors for accurate car detection (150mm threshold)

### Relay and Solenoid Setup
- [x] Connect relay module to ESP32 (GPIO14)
- [x] Test relay activation/deactivation
- [x] Connect solenoid to relay with appropriate power supply
- [x] Test solenoid activation (250ms pulse)

### Integration Setup
- [x] Design and create mounting for sensors at finish line
- [x] Set up starting gate mechanism with solenoid
- [x] Position and secure all components on race track
- [x] Test complete hardware integration

## Software Development Tasks

### Core Functionality
- [x] Develop I2C communication with VL53L0XV2 sensors
- [x] Create timer functionality with millisecond precision
- [x] Implement relay control for start sequence
- [x] Develop finish line detection algorithm
- [x] Implement NTP time synchronization
  - [x] Configure NTP client
  - [x] Set system time from NTP
  - [x] Add timezone support
  - [x] Include accurate timestamps in race history
- [x] Improve version information display
  - [x] Show version number on boot via Serial
  - [x] Define version number as a constant
  - [x] Include build date in version info

### Race Logic
- [x] Create race start sequence
- [x] Implement dual-lane timing system
- [x] Develop finish detection and timing capture
- [x] Add race result storage functionality

### Web Interface
- [x] Set up ESP32 as web server
  - [x] Configure AsyncWebServer
  - [x] Set up WebSocket server
  - [x] Initialize LittleFS for web files
- [x] Implement Access Point (AP) fallback mode
  - [x] Create AP if WiFi connection fails
  - [x] Set up DNS for captive portal
  - [x] Configure AP with unique SSID (CO2RaceTimer-XXXX)
  - [x] Add AP status indicator to web interface
  - [x] Auto-switch between AP and Station modes
- [x] Create responsive web UI with:
  - [x] Race status display (waiting/ready/racing/finished)
  - [x] Start/Load controls with button feedback
  - [x] Real-time race timing display
  - [x] Race history table with past results
  - [x] WiFi signal strength indicator
  - [x] System status indicators (sensors, relay)
  - [x] Version display in web interface
  - [x] Show version on initial page load
  - [x] Add version to system status section
- [x] Implement WebSocket features:
  - [x] Real-time sensor readings
  - [x] Race state updates
  - [x] Finish time broadcasts
  - [x] Connection status monitoring
- [x] Add mobile-friendly layout
  - [x] Touch-optimized controls
  - [x] Responsive grid layout
  - [x] Portrait/landscape modes
- [x] Implement race history
  - [x] LittleFS storage
  - [x] Automatic saving
  - [x] History display
  - [x] Race cleanup
- [x] Create configuration page
  - [x] WiFi settings
  - [x] Sensor thresholds
  - [x] Race timing parameters
  - [x] System calibration

### User Interface
- [x] LED status indicators
- [x] Button controls for local operation
- [x] Add configuration options for network settings

### Testing & Optimization
- [x] Core System Testing
  - [x] Timing accuracy verification
  - [x] Sensor detection reliability
  - [x] Relay activation timing
  - [x] LED state transitions
  - [x] Button debouncing
  - [x] Fair tie detection with 2ms tolerance
  - [x] Real-time tie detection and handling
  - [x] Race timing optimization (network paused during races)

- [x] Network Testing
  - [x] WiFi connection stability
  - [x] Reconnection handling
  - [x] WiFi credential management
  - [x] AP mode transitions
  - [x] Offline mode functionality
  - [x] WebSocket reconnection
  - [x] Data persistence

- [x] Performance Optimization
  - [x] Minimize web asset sizes
  - [x] Optimize WebSocket messages
  - [x] Reduce memory usage
  - [x] Cache static content
  - [x] Compress data transfers

## Code Structure Tasks

### Libraries to Implement
- [x] VL53L0X library for sensor communication (pololu/VL53L0X @ ^1.3.1)
- [x] Timer library for precise timing (using millis())
- [x] User feedback (RGB LED + Buzzer)

### SD Card Implementation
- [x] Set up SD card storage system
  - [x] Initialize SD card module
  - [x] Test read/write operations
  - [x] Implement error handling for card failures

- [x] ESPAsyncWebServer for web interface (me-no-dev/ESPAsyncWebServer)
- [x] AsyncTCP for WebSocket support (me-no-dev/AsyncTCP)
- [x] LittleFS for file system (built-in ESP32)
- [x] ArduinoJson for data handling (bblanchon/ArduinoJson @ ^6.21.5)
- [x] Bootstrap for responsive UI (served from CDN)
- [x] Chart.js for race timing visualization (served from CDN)

## Race Management System 

### Web Interface with Python Backend
- [ ] Implement Python-based web application
  - [ ] Set up Flask/FastAPI backend
  - [ ] Create RESTful API endpoints
  - [ ] Implement WebSocket support for real-time updates
  - [ ] Design database schema (SQLite/PostgreSQL)
  - [ ] Create user authentication system
  - [ ] Implement session management

### Frontend Development
- [ ] Create responsive web interface
  - [ ] Use modern frontend framework (React/Vue.js)
  - [ ] Implement real-time race status display
  - [ ] Create race control dashboard
  - [ ] Design results display interface
  - [ ] Add admin control panel
  - [ ] Implement mobile-responsive design

### ESP32 Communication
- [ ] Implement serial communication protocol
  - [ ] Create Python serial interface
  - [ ] Design message protocol
  - [ ] Implement error handling
  - [ ] Add reconnection logic
  - [ ] Create data synchronization system

### Race Scheduling
- [ ] Implement Young and Pope "Partial Perfect-N" charts
  - [ ] Create race schedule generator
  - [ ] Support variable number of racers
  - [ ] Calculate optimal heat assignments
  - [ ] Generate round-robin matchups

### Real-time Standings
- [ ] Implement standings calculation system
  - [ ] Track points per racer
  - [ ] Calculate rankings
  - [ ] Update standings in real-time
  - [ ] Display race progression

### Race Operations Interface
- [ ] Create "On Deck" display system
  - [ ] Show upcoming races
  - [ ] Display next racers
  - [ ] Highlight lane assignments
  - [ ] Add preparation countdown

### Check-in System
- [ ] Implement racer check-in functionality
  - [ ] Digital check-in interface
  - [ ] Real-time check-in status updates
  - [ ] "Please Check-In" display
  - [ ] Automated notifications
  - [ ] Check-in deadline management

### Race Data Management
- [ ] Create participant database
  - [ ] Racer profiles
  - [ ] Race history per participant
  - [ ] Performance statistics
- [ ] Implement results tracking
  - [ ] Heat results
  - [ ] Round summaries
  - [ ] Tournament progression
- [ ] Results Export System
  - [ ] Spreadsheet export functionality
    - [ ] Individual race results
    - [ ] Complete tournament data
    - [ ] Participant statistics
  - [ ] Multiple format support
    - [ ] Excel (.xlsx)
    - [ ] CSV format
    - [ ] Google Sheets compatible
  - [ ] Custom report templates
    - [ ] Race summary reports
    - [ ] Participant performance reports
    - [ ] Tournament brackets

### Main Program Components
- [x] Sensor reading module (dual VL53L0X with unique addresses)
- [x] Race control module (start/finish/timing)
- [x] Timer functionality module (millisecond precision)
- [x] Web server module
  - [x] HTTP request handlers
  - [x] WebSocket server
  - [x] File system for web assets
- [x] Network connectivity manager
  - [x] WiFi configuration portal
  - [x] Connection recovery
- [x] User interface module (LED states + button control)
- [ ] Python Backend Components
  - [ ] Serial communication handler
  - [ ] WebSocket server
  - [ ] Database interface
  - [ ] API endpoints
  - [ ] Authentication system
  - [ ] Race management logic

## Documentation Tasks

### Code Documentation
- [ ] Add detailed comments to code
- [ ] Create function documentation
- [ ] Document pin connections and hardware setup
- [ ] Create user manual for operation

### Project Documentation
- [ ] Document final design
- [ ] Create wiring diagram
- [ ] Document calibration procedure
- [ ] Create troubleshooting guide for common issues
- [x] Create network setup guide

## Testing Tasks

### Component Testing
- [ ] Test sensor accuracy and repeatability
- [ ] Test relay and solenoid reliability
- [ ] Test timing accuracy
- [ ] Test WiFi connectivity stability

### System Testing
- [ ] Conduct full system test with CO2 cars
- [ ] Test edge cases (simultaneous finishes, false triggers)
- [ ] Test system reliability over multiple races
- [ ] Test system recovery from network interruptions

## Completed Tasks
- [x] Implement daily race history files (YYYY-MM-DD.json) - Completed 09-04-2025
- [x] Each daily file contains an array of all races from that day - Completed 09-04-2025
- [x] Improved race history organization and management - Completed 09-04-2025

## Pending Tasks
// ... existing code ...