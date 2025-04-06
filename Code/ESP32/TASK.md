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
- [ ] Implement NTP time synchronization
  - [ ] Configure NTP client
  - [ ] Set system time from NTP
  - [ ] Add timezone support
  - [ ] Include accurate timestamps in race history

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
- [x] Create responsive web UI with:
  - [x] Race status display (waiting/ready/racing/finished)
  - [x] Start/Load controls with button feedback
  - [x] Real-time race timing display
  - [x] Race history table with past results
  - [x] WiFi signal strength indicator
  - [x] System status indicators (sensors, relay)
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
- [ ] Create configuration page
  - [ ] WiFi settings
  - [ ] Sensor thresholds
  - [ ] Race timing parameters
  - [ ] System calibration
- [ ] Implement offline functionality
  - [ ] Cache static assets
  - [ ] Local race history storage
  - [ ] Reconnection handling

### User Interface
- [x] LED status indicators
- [x] Button controls for local operation
- [ ] Add configuration options for network settings

### Testing & Optimization
- [x] Core System Testing
  - [x] Timing accuracy verification
  - [x] Sensor detection reliability
  - [x] Relay activation timing
  - [x] LED state transitions
  - [x] Button debouncing
  - [x] Fair tie detection with 2ms tolerance

- [ ] Web Interface Testing
  - [ ] Page load performance (<500ms)
  - [ ] WebSocket latency (<50ms)
  - [ ] Race timing accuracy display
  - [ ] Mobile responsiveness
  - [ ] Touch input reliability
  - [ ] Cross-browser compatibility

- [ ] Network Testing
  - [ ] WiFi connection stability
  - [ ] Reconnection handling
  - [ ] Offline mode functionality
  - [ ] WebSocket reconnection
  - [ ] Data persistence

- [ ] Performance Optimization
  - [ ] Minimize web asset sizes
  - [ ] Optimize WebSocket messages
  - [ ] Reduce memory usage
  - [ ] Cache static content
  - [ ] Compress data transfers

## Code Structure Tasks

### Libraries to Implement
- [x] VL53L0X library for sensor communication (pololu/VL53L0X @ ^1.3.1)
- [x] Timer library for precise timing (using millis())
- [x] User feedback (RGB LED + Buzzer)
- [x] ESPAsyncWebServer for web interface (me-no-dev/ESPAsyncWebServer)
- [x] AsyncTCP for WebSocket support (me-no-dev/AsyncTCP)
- [x] LittleFS for file system (built-in ESP32)
- [x] ArduinoJson for data handling (bblanchon/ArduinoJson @ ^6.21.5)
- [x] Bootstrap for responsive UI (served from CDN)
- [x] Chart.js for race timing visualization (served from CDN)

### Main Program Components
- [x] Sensor reading module (dual VL53L0X with unique addresses)
- [x] Race control module (start/finish/timing)
- [x] Timer functionality module (millisecond precision)
- [ ] Web server module
  - [ ] HTTP request handlers
  - [ ] WebSocket server
  - [ ] File system for web assets
- [ ] Network connectivity manager
  - [ ] WiFi configuration portal
  - [ ] Connection recovery
- [x] User interface module (LED states + button control)

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
- [ ] Create network setup guide

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