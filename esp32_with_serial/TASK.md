# CO2 Car Race Timer Project Tasks

## Hardware Setup Tasks

### ESP32 Setup
- [x] Set up ESP32 development environment (Arduino IDE or PlatformIO)
- [x] Test basic ESP32 functionality with blink sketch
- [x] Configure necessary libraries for ESP32
- [ ] Test WiFi connectivity for DerbyNet communication

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

### Race Logic
- [x] Create race start sequence
- [x] Implement dual-lane timing system
- [x] Develop finish detection and timing capture
- [x] Add race result storage functionality

### Testing & Optimization
- [ ] Test timing accuracy
- [ ] Optimize sensor detection reliability
- [ ] Debug and fix communication issues

## Code Structure Tasks

### Libraries to Implement
- [x] VL53L0X library for sensor communication (pololu/VL53L0X @ ^1.3.1)
- [x] Timer library for precise timing (using millis())
- [x] User feedback (RGB LED + Buzzer)
- [x] JSON library for DerbyNet data handling (ArduinoJson @ ^6.21.3)

### Main Program Components
- [x] Sensor reading module (dual VL53L0X with unique addresses)
- [x] Race control state machine (waiting, ready, racing, finished)
- [x] Timer functionality module (millisecond precision)
- [x] User interface module (LED states + button control)

## Documentation Tasks

### Code Documentation
- [ ] Add detailed comments to code
- [ ] Create function documentation
- [ ] Document pin connections and hardware setup
- [ ] Document DerbyNet communication protocol implementation
- [ ] Create user manual for operation

### Project Documentation
- [ ] Document final design
- [ ] Create wiring diagram
- [ ] Document DerbyNet integration steps
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
- [ ] Test DerbyNet integration under varying network conditions
- [ ] Test system recovery from network interruptions