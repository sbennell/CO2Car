# CO2 Car Race Timer

**Version: 0.11.5** (2025-04-11)

Latest Changes:
- Fixed WiFi configuration management
- Improved network mode switching
- Enhanced WebSocket status broadcasting
- Added detailed network logging and logging

 for ESP32-S3

A high-precision dual-lane race timer system for CO₂ powered cars using ESP32-S3 and VL53L0X distance sensors. The system uses FreeRTOS for real-time task management and precise timing control.

## Hardware Configuration

### ESP32-S3 Pinout

| ESP32-S3 Pin | Component      | Notes                  |
|-------------|---------------|------------------------|
| GPIO8       | I2C SDA       | Shared by both sensors |
| GPIO9       | I2C SCL       | Shared by both sensors |
| GPIO16      | Sensor 1 XSHUT | VL53L0X address: 0x30 |
| GPIO15      | Sensor 2 XSHUT | VL53L0X address: 0x31 |
| GPIO4       | Load Button    | INPUT, internal pullup |
| GPIO5       | Start Button   | INPUT, internal pullup |
| GPIO14      | Relay          | OUTPUT, Active LOW     |
| GPIO48      | LED Blue       | OUTPUT, Active HIGH    |
| GPIO47      | LED Red        | OUTPUT, Active HIGH    |
| GPIO21      | LED Green      | OUTPUT, Active HIGH    |
| GPIO45      | Buzzer         | OUTPUT, Active HIGH    |
| GPIO12    | SD Card SCK          | SPI0 Clock              |
| GPIO13    | SD Card MISO         | SPI0 MISO               |
| GPIO11    | SD Card MOSI         | SPI0 MOSI               |
| GPIO10    | SD Card CS           | SPI0 Chip Select        |

### Components

- **Microcontroller**: ESP32-S3-WROOM-1-N16R8
  - 16MB Flash Memory
  - 8MB PSRAM
  - Dual-core Xtensa® 32-bit LX7 up to 240MHz
  - FreeRTOS for task management
- **Distance Sensors**: 2× VL53L0X Time-of-Flight sensors (one per lane at finish line)
  - 50Hz sampling rate
  - 20ms measurement budget
  - I²C addresses: Lane 1: 0x30, Lane 2: 0x31
- **User Interface**:
  - RGB LED for status indication
  - 2 momentary push buttons (Load and Start)
  - Buzzer for audio feedback
- **Storage**: SD Card via SPI
- **Actuator**: 5V Relay for CO₂ canister trigger (250ms pulse)

### I2C Configuration

- **Bus**: Shared I2C bus on GPIO8 (SDA) and GPIO9 (SCL)
- **Speed**: 100kHz standard mode
- **Addressing**: 
  - Sensor 1: 0x30 (XSHUT on GPIO16)
  - Sensor 2: 0x31 (XSHUT on GPIO15)
- **Access Control**: Protected by FreeRTOS mutex

### System Architecture

#### FreeRTOS Tasks
- **Sensor Task** (Core 0, Priority 2)
  - Handles both VL53L0X sensors
  - 50Hz sampling rate
  - Protected I²C access

- **Timer Task** (Core 0, Priority 2)
  - Manages race timing
  - 100Hz update rate
  - Microsecond precision

- **Race Control Task** (Core 1, Priority 1)
  - Manages race state
  - Handles button inputs
  - Controls relay and indicators

- **Web Server Task** (Core 1, Priority 1)
  - Handles web interface
  - WebSocket communications
  - Race result display

### Race States and Events

#### States
- **IDLE**: Initial state, waiting for cars to be loaded
- **READY**: Cars loaded, waiting for race start
- **RUNNING**: Race in progress, timing active
- **FINISHED**: Race complete with results

#### Events
- **Sensor Events**:
  - Timestamp: Microsecond precision
  - Distance: Millimeter resolution
  - Sensor ID: 1 (Start) or 2 (Finish)

- **Race Events**:
  - State transitions
  - Start/finish times
  - Elapsed time
  - Winner determination

### LED Status Indicators

- **Waiting** (Red): System is waiting for cars to be loaded
- **Ready** (Red + Green): Cars loaded, ready to start
- **Racing** (Blue Blinking): Race in progress
- **Finished** (Green): Race complete, results available

### SD Card Interface

The SD card uses dedicated SPI pins on the ESP32-S3 for optimal performance and reliable data storage. Race results are automatically saved after each race.
