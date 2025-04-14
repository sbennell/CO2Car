# CO2 Car Race Timer - Project Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Hardware Components](#hardware-components)
3. [System Architecture](#system-architecture)
4. [Installation Guide](#installation-guide)
5. [Wiring Diagram](#wiring-diagram)
6. [Race Operation Procedures](#race-operation-procedures)
7. [Sensor Calibration](#sensor-calibration)
8. [Race Management System Integration](#race-management-system-integration)
9. [Communication Protocol](#communication-protocol)
10. [LED Status Indicators](#led-status-indicators)
11. [Button Operation Guide](#button-operation-guide)
12. [Race State Machine](#race-state-machine)
13. [Troubleshooting Guide](#troubleshooting-guide)
14. [Maintenance Guide](#maintenance-guide)
15. [System Limitations](#system-limitations)
16. [Upgrade Path](#upgrade-path)

## System Overview

The CO2 Car Race Timer is an automated timing system designed for CO2 car racing events. It uses VL53L0X distance sensors to detect when cars cross the finish line, providing accurate timing with millisecond precision. The system includes a relay-controlled starting gate mechanism and provides visual feedback through RGB LEDs.

### Key Features
- Dual-lane timing system
- Millisecond precision timing
- Automated starting gate control
- Visual status indicators
- JSON-based communication protocol
- Integration with race management systems
- Real-time sensor data streaming
- Remote race control capabilities

## Hardware Components

### ESP32 Microcontroller
- Main processing unit
- Handles sensor communication, timing, and control logic
- Provides serial communication interface

### VL53L0X Distance Sensors (2)
- Time-of-flight sensors for car detection
- I2C communication with ESP32
- Configured with different I2C addresses (0x30 and 0x31)
- Mounted at the finish line

### Relay Module
- Controls the starting gate mechanism
- Connected to GPIO14 on ESP32
- Active LOW control (relay activates when pin is set LOW)

### Solenoid
- Connected to relay module
- Controls the starting gate mechanism
- Activated with a 500ms pulse

### RGB LED
- Provides visual status feedback
- Red LED: GPIO25
- Green LED: GPIO26
- Blue LED: GPIO33

### Buttons
- Car Loaded Button: GPIO4 (with internal pullup)
- Start Button: GPIO13 (with internal pullup)

## System Architecture

The system is built around an ESP32 microcontroller that manages all components through a state machine architecture. The main components are:

1. **Sensor Module**: Handles communication with the VL53L0X sensors
2. **Control Module**: Manages the relay and starting gate mechanism
3. **User Interface Module**: Handles button inputs and LED outputs
4. **Communication Module**: Manages serial communication with the race management system
5. **Race Logic Module**: Implements the race state machine and timing logic

### State Machine
The system operates in the following states:
- **IDLE**: System is ready, waiting for cars to be loaded
- **CARS_LOADED**: Cars are loaded, waiting for race start
- **RACE_READY**: Race is ready to start
- **COUNTDOWN**: Countdown before race start
- **RACING**: Race in progress
- **RACE_FINISHED**: Race completed

## Installation Guide

### Required Components
- ESP32 development board
- 2x VL53L0X distance sensors
- 1x Relay module
- 1x Solenoid
- 1x RGB LED
- 2x Push buttons
- Jumper wires
- Power supply (5V)
- Mounting hardware

### Software Requirements
- Arduino IDE or PlatformIO
- VL53L0X library (pololu/VL53L0X @ ^1.3.1)
- ArduinoJson library (ArduinoJson @ ^6.21.3)

### Installation Steps
1. Connect the VL53L0X sensors to the ESP32:
   - Connect VCC to 3.3V
   - Connect GND to GND
   - Connect SDA to GPIO21
   - Connect SCL to GPIO22
   - Connect XSHUT of sensor 1 to GPIO16
   - Connect XSHUT of sensor 2 to GPIO17

2. Connect the relay module:
   - Connect VCC to 5V
   - Connect GND to GND
   - Connect IN to GPIO14

3. Connect the RGB LED:
   - Connect Red LED to GPIO25
   - Connect Green LED to GPIO26
   - Connect Blue LED to GPIO33
   - Connect all LED cathodes to GND through appropriate resistors (220Ω)

4. Connect the buttons:
   - Connect Car Loaded Button to GPIO4
   - Connect Start Button to GPIO13
   - Connect other side of buttons to GND

5. Upload the firmware:
   - Open the project in Arduino IDE or PlatformIO
   - Install required libraries
   - Compile and upload to the ESP32

## Wiring Diagram

```
ESP32                     VL53L0X Sensor 1
+----------------+       +----------------+
|                |       |                |
| 3.3V           +-------+ VCC            |
| GND            +-------+ GND            |
| GPIO21 (SDA)   +-------+ SDA            |
| GPIO22 (SCL)   +-------+ SCL            |
| GPIO16 (XSHUT) +-------+ XSHUT          |
|                |       |                |
+----------------+       +----------------+

ESP32                     VL53L0X Sensor 2
+----------------+       +----------------+
|                |       |                |
| 3.3V           +-------+ VCC            |
| GND            +-------+ GND            |
| GPIO21 (SDA)   +-------+ SDA            |
| GPIO22 (SCL)   +-------+ SCL            |
| GPIO17 (XSHUT) +-------+ XSHUT          |
|                |       |                |
+----------------+       +----------------+

ESP32                     Relay Module
+----------------+       +----------------+
|                |       |                |
| 5V             +-------+ VCC            |
| GND            +-------+ GND            |
| GPIO14         +-------+ IN             |
|                |       |                |
+----------------+       +----------------+

ESP32                     RGB LED
+----------------+       +----------------+
|                |       |                |
| GPIO25         +-------+ Red (with 220Ω) |
| GPIO26         +-------+ Green (with 220Ω)|
| GPIO33         +-------+ Blue (with 220Ω) |
| GND            +-------+ Common Cathode  |
|                |       |                |
+----------------+       +----------------+

ESP32                     Buttons
+----------------+       +----------------+
|                |       |                |
| GPIO4          +-------+ Car Loaded     |
| GPIO13         +-------+ Start          |
| GND            +-------+ Other Side     |
|                |       |                |
+----------------+       +----------------+
```

## Race Operation Procedures

### Preparing for a Race
1. Power on the system
2. Wait for the blue LED to indicate the system is ready
3. Load the cars into the starting positions
4. Press the Car Loaded button
5. Wait for the green LED to indicate cars are loaded

### Starting a Race
1. Press the Start button
2. The system will perform a 3-second countdown with purple LED flashes
3. The starting gate will release automatically
4. The yellow LED will indicate the race is in progress

### During a Race
1. The system will automatically detect when cars cross the finish line
2. Real-time sensor data will be sent to the race management system
3. The first car to cross the finish line will be recorded

### After a Race
1. The system will determine the winner
2. The LED will flash the color of the winning car (red for car 1, green for car 2, blue for a tie)
3. Race results will be sent to the race management system
4. After 3 seconds, the system will automatically reset to the IDLE state

## Sensor Calibration

The system performs sensor calibration at startup and can be manually calibrated using the "calibrate" command.

### Automatic Calibration
1. The system takes 15 readings from each sensor
2. It calculates the average reading for each sensor
3. These baseline readings are used to detect when cars cross the finish line
4. The system indicates successful calibration with 3 green LED flashes
5. If calibration fails, the system indicates with 3 red LED flashes

### Manual Calibration
1. Send the "calibrate" command via serial
2. The system will perform the calibration procedure
3. Results will be sent via serial

### Calibration Threshold
- The system uses a 100mm threshold for car detection
- When a sensor reading drops below (baseline - 100mm), a car is considered to have crossed the finish line

## Race Management System Integration

The system communicates with the race management system using a JSON-based protocol over serial communication.

### Communication Protocol
- Baud rate: 115200
- Data format: JSON
- Line endings: Newline

### Message Types
1. **Status Messages**
   - Sent periodically and on state changes
   - Include current state, sensor readings, and race data

2. **Race Control Messages**
   - Received from the race management system
   - Control race operations (start, reset, calibrate)

3. **Race Result Messages**
   - Sent when a race is completed
   - Include timing data and winner information

4. **Sensor Data Messages**
   - Sent every 100ms during a race
   - Include raw sensor readings

### Integration Steps
1. Connect the ESP32 to the race management system via USB
2. Configure the race management system to communicate at 115200 baud
3. Implement the JSON protocol in the race management system
4. Test the integration with sample races

## Communication Protocol

### JSON Message Format

#### Status Message
```json
{
  "type": "status",
  "race_state": "IDLE",
  "cars_loaded": false,
  "race_started": false,
  "car1_finished": false,
  "car2_finished": false,
  "car1_time": 0,
  "car2_time": 0,
  "sensors_calibrated": true,
  "sensor1_baseline": 500,
  "sensor2_baseline": 500,
  "timestamp": 1234567890
}
```

#### Race Result Message
```json
{
  "type": "race_result",
  "car1_time": 1234,
  "car2_time": 1345,
  "winner": "car1",
  "race_id": 42
}
```

#### Sensor Reading Message
```json
{
  "type": "sensor_reading",
  "sensor1": 450,
  "sensor2": 480,
  "timestamp": 1234567890
}
```

#### Command Message (from race management system)
```json
{
  "cmd": "start_race",
  "race_id": 42
}
```

## LED Status Indicators

The RGB LED provides visual feedback about the system state:

- **Blue**: System is ready, waiting for cars (IDLE state)
- **Green**: Cars are loaded (CARS_LOADED state)
- **Cyan**: Race is ready to start (RACE_READY state)
- **Purple**: Countdown in progress (COUNTDOWN state)
- **Yellow**: Race in progress (RACING state)
- **Red/Green/Blue Flashing**: Race finished (RACE_FINISHED state)
  - Red: Car 1 won
  - Green: Car 2 won
  - Blue: Tie

## Button Operation Guide

### Car Loaded Button (GPIO4)
- **Function**: Indicates that cars are loaded and ready
- **Behavior**: Press to transition from IDLE to CARS_LOADED state
- **Feedback**: Green LED flashes 3 times when pressed

### Start Button (GPIO13)
- **Function**: Starts the race
- **Behavior**: Press to transition from CARS_LOADED to RACE_READY state
- **Feedback**: Purple LED flashes during countdown

## Race State Machine

The system operates as a state machine with the following states and transitions:

1. **IDLE**
   - Initial state
   - Blue LED on
   - Transitions to CARS_LOADED when Car Loaded button is pressed

2. **CARS_LOADED**
   - Green LED on
   - Transitions to RACE_READY when Start button is pressed

3. **RACE_READY**
   - Cyan LED on
   - Transitions to COUNTDOWN immediately

4. **COUNTDOWN**
   - Purple LED flashing
   - 3-second countdown
   - Transitions to RACING after countdown

5. **RACING**
   - Yellow LED on
   - Monitors sensors for car detection
   - Transitions to RACE_FINISHED when both cars finish

6. **RACE_FINISHED**
   - LED flashing based on winner
   - 3-second display
   - Automatically transitions to IDLE

## Troubleshooting Guide

### Common Issues and Solutions

1. **Sensors Not Detecting Cars**
   - Check sensor connections
   - Recalibrate sensors using the "calibrate" command
   - Verify sensor baseline readings in status messages
   - Check for obstructions in the sensor path

2. **Starting Gate Not Releasing**
   - Check relay connections
   - Verify relay is receiving the activation signal
   - Test relay with the "fire_relay" command
   - Check power supply to the relay

3. **System Not Responding to Buttons**
   - Check button connections
   - Verify button states in status messages
   - Try resetting the system with the "forceReset" command
   - Check for button debounce issues

4. **Communication Issues**
   - Verify serial connection
   - Check baud rate (115200)
   - Monitor for JSON parsing errors
   - Reset the system if communication is lost

5. **Incorrect Timing**
   - Recalibrate sensors
   - Check for interference in the sensor path
   - Verify sensor baseline readings
   - Test with known good cars

## Maintenance Guide

### Regular Maintenance
1. **Clean Sensors**
   - Remove dust and debris from sensor lenses
   - Clean monthly or after heavy use

2. **Check Connections**
   - Verify all connections are secure
   - Check for loose wires
   - Inspect monthly

3. **Test System**
   - Run a test race weekly
   - Verify timing accuracy
   - Check sensor readings

4. **Update Firmware**
   - Check for firmware updates
   - Update when new features or bug fixes are available

### Long-term Maintenance
1. **Replace Components**
   - Buttons may need replacement after extended use
   - Relay may need replacement after 10,000 activations
   - Sensors may need replacement if readings become unreliable

2. **Backup Configuration**
   - Save sensor baseline readings
   - Document any custom configurations

## System Limitations

1. **Sensor Limitations**
   - Maximum detection range: 1200mm
   - Minimum detection range: 30mm
   - May be affected by ambient light
   - May be affected by reflective surfaces

2. **Timing Limitations**
   - Millisecond precision
   - May have slight variations in start time between lanes
   - Maximum race duration: 65 seconds (due to millis() overflow)

3. **Communication Limitations**
   - Serial communication only
   - No wireless connectivity
   - Limited to one race management system connection

4. **Physical Limitations**
   - Two-lane system only
   - Fixed sensor positions
   - Requires physical button presses

## Upgrade Path

### Potential Upgrades

**Hardware Upgrades**
   - Add display screen
   - Add buzzer for audio feedback
   - Add support for more lanes

### Implementation Considerations
- Check compatibility with existing hardware
- Verify power requirements for new components
- Test thoroughly before deployment
- Document changes and new features
- Provide user training for new features 
