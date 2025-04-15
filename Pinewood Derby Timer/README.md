# ESP32 Pinewood Derby Timer

An ESP32-based Pinewood Derby timer that uses VL53L0X Time-of-Flight distance sensors for accurate finish line detection.

Website: [www.dfgtec.com/pdt](http://www.dfgtec.com/pdt)

## Overview

This flexible and affordable Pinewood Derby timer interfaces with popular racing software:
- PD Test/Tune/Track Utility
- Grand Prix Race Manager software

The ESP32 version improves upon the original Arduino-based design by providing better processing power, more reliable timing, and advanced distance sensor support for finish line detection.

## Features

- Support for 2 lanes with VL53L0X distance sensors for finish detection
- Compatible with standard Pinewood Derby software via serial interface
- Remote start capability with solenoid control
- LED status indicators
- Optional LED display support for showing race times and positions
- Adaptable to different track configurations

## Hardware Requirements

- ESP32 development board
- Two VL53L0X distance sensors
- Solenoid for remote start (optional)
- Status LEDs (Red, Green, Blue)
- Push button for reset
- Push button for start gate detection
- Wiring and connectors
- 5V power supply

## Pin Connections

| Component | ESP32 Pin |
|-----------|-----------|
| I2C SDA | 21 |
| I2C SCL | 22 |
| Sensor 1 XSHUT | 16 |
| Sensor 2 XSHUT | 17 |
| Start Gate Button | 4 |
| Start Solenoid | 14 |
| Reset Switch | 13 |
| Status LED Red | 25 |
| Status LED Green | 26 |
| Status LED Blue | 33 |
| Brightness Level | 34 |

## Installation

1. Install the Arduino IDE
2. Install ESP32 board support in Arduino IDE
3. Install the required libraries:
   - Wire.h (built-in)
   - VL53L0X by Pololu
   - Adafruit_GFX and Adafruit_LEDBackpack (if using LED displays)
4. Download and open timer.ino
5. Select your ESP32 board in the Arduino IDE
6. Upload the sketch to your ESP32

## Sensor Setup

The timer uses two VL53L0X sensors to detect cars crossing the finish line. These sensors measure distance using time-of-flight technology, which provides highly accurate timing.

- Sensors are configured with different I2C addresses (0x30 and 0x31)
- Detection threshold is set to 150mm by default (adjustable in code)
- Sensors should be positioned above each lane pointing down at the track

## Usage

### Basic Operation

1. Power on the timer
2. Connect to your computer via USB
3. Open your race management software
4. The timer will automatically be identified
5. Load cars on the track
6. Start races either via software command or manual start button
7. Times are recorded and sent to connected software

### Serial Commands

The timer accepts the following commands via serial:
- 'S' - Start solenoid (activates for 100ms to start race)
- 'R' - Reset timer
- 'G' - Check gate status
- 'F' - Force end of race
- 'Q' - Resend race data
- Other commands for diagnostics and configuration

## Configuration

Edit the following parameters in the code as needed:

```c
#define NUM_LANES    2                 // number of lanes
#define GATE_RESET   0                 // Enable closing start gate to reset timer
#define SHOW_PLACE   1                 // Show place mode
#define PLACE_DELAY  3                 // Delay (secs) when displaying place/time
#define DISTANCE_THRESHOLD 150         // Distance in mm to detect car crossing finish line
```

## Change Log

### Version 3.11 - 15 APR 2025
- Updated for ESP32 platform
- Added support for VL53L0X distance sensors for finish line detection
- Updated pin definitions for ESP32 hardware
- Modified solenoid control: inverted logic, auto-off after 100ms
- Added auto-start functionality when 'S' command is received
- Improved debug messaging

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

## Credits

Original PDT software by David Gadberry (2011-2020)  
ESP32 and VL53L0X modifications by Steweartb 