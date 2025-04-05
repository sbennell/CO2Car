# CO₂ Car Race Timer

## Description

The **CO₂ Car Race Timer** is an automated race timer system designed for CO₂-powered cars. It uses two VL53L0X distance sensors to measure the time taken for each car to cross the finish line. The system integrates a relay to trigger the CO₂ firing mechanism and provides race results based on the fastest time. 

The system ensures that the race cannot start until both cars are loaded, which can be triggered by a button or serial command. Once the race starts, the system tracks each car's progress and declares the winner once both cars cross the finish line.

## Features

- **Two VL53L0X distance sensors**: Tracks the cars as they pass through the sensor line.
- **Relay control**: Fires the CO₂ mechanism to start the race.
- **Load detection**: Requires cars to be loaded before the race can start, using a button or serial command.
- **Race result output**: Displays the time taken for each car to complete the race and declares a winner.
- **Button press handling**: Supports momentary switches for load and start buttons, with proper edge detection.
- **Debug logging**: Optionally outputs sensor distance readings for troubleshooting.
- **LED indicators**: Visual feedback of race state (waiting, ready, racing, finished).
- **Buzzer feedback**: Audible cues at race start and finish.

## Hardware Requirements

- **Two VL53L0X distance sensors**: Used to track the cars' progress through the track.
- **Relay module**: Triggers the CO₂ firing mechanism.
- **Load button**: Used to signify when the cars are loaded.
- **Start button**: Used to start the race after the cars are loaded.
- **Arduino Uno**: Manages communication between the sensors, relay, buttons, and the system logic.
- **Tri-color LED**: Visual indication of system state.
- **Buzzer**: Provides audible feedback for race events.

## Pinout

| Pin       | Component            |
|-----------|----------------------|
| Digital 2 | Sensor 1 XSHUT       |
| Digital 3 | Sensor 2 XSHUT       |
| Digital 4 | Load Button (INPUT)  |
| Digital 5 | Start Button (INPUT) |
| Digital 8 | Relay (OUTPUT)       |
| Digital 9 | Buzzer (OUTPUT)      |
| Digital 10| LED Red (OUTPUT)     |
| Digital 11| LED Green (OUTPUT)   |
| Digital 12| LED Blue (OUTPUT)    |

## Installation

### 1. Hardware Setup

- **VL53L0X Sensors**: Connect both sensors to the Arduino Uno via I2C (A4/SDA and A5/SCL pins). Use `XSHUT` pins (Digital 2 and Digital 3) to reset each sensor individually.
- **Relay Module**: Connect the relay to Digital 8 on the Arduino Uno to trigger the CO₂ mechanism.
- **Buttons**: Connect the load button to Digital 4 and the start button to Digital 5.
- **LED**: Connect the tri-color LED to pins 10 (Red), 11 (Green), and 12 (Blue).
- **Buzzer**: Connect the buzzer to Digital 9.

### 2. Software Setup

- **Arduino IDE**: Install the [Arduino IDE](https://www.arduino.cc/en/software).
- **Install Required Libraries**: You will need the `VL53L0X` library for sensor communication. Install it via the Arduino Library Manager:
  - Go to **Sketch** > **Include Library** > **Manage Libraries**.
  - Search for `VL53L0X` and install it.

### 3. Upload Code to Arduino Uno

1. Open the project in the Arduino IDE.
2. Select **Arduino Uno** board in **Tools** > **Board**.
3. Select the correct port in **Tools** > **Port**.
4. Upload the code to the Arduino by clicking the **Upload** button.

### 4. Open the Serial Monitor

Once the upload is complete, open the Serial Monitor (`Tools` > `Serial Monitor`), and set the baud rate to `9600`. You should see the system initialize and wait for the cars to be loaded.

## Usage

### 1. **Loading the Cars**
- **Via Load Button**: Press the **Load Button** (Digital 4) to signify that the cars are loaded and ready to race.
- **Via Serial Command**: Send **'L'** via Serial Monitor to load the cars.

Once the cars are loaded, the system will display a message indicating that the cars are ready to race, and the LED will show **amber** (red + green).

### 2. **Starting the Race**
- **Via Start Button**: Press the **Start Button** (Digital 5) to begin the race once the cars are loaded.
- **Via Serial Command**: Send **'S'** via Serial Monitor to start the race.

When the race starts, you'll hear a short beep and the LED will turn **blue**.

### 3. **Race Results**
Once both cars cross the finish line, the system will display the race results with the times for each car and declare the winner. You'll hear a longer beep and the LED will turn **green**.

Example output:
```
🚦 Cars loaded. Press 'S' to start the race.
📩 Received Serial Command: S
🚦 Race Starting...
🔹 Firing CO₂ Relay...
✔ Relay deactivated
🏎 Race in progress...
📏 Sensor Readings: C1 = 145 mm, C2 = 130 mm
🏁 Car 1 Finished! Time: 1234 ms
🏁 Car 2 Finished! Time: 1120 ms
🎉 Race Finished!
🏆 Car 2 Wins!
📊 RESULT: C1=1234ms, C2=1120ms
```

### 4. **Reset for Next Race**

After the race, the system will reset and wait for the next race. To reset:
- Press 'L' via Serial or press the Load Button to load the cars again.
- The LED will turn **red** when the system is waiting for cars to be loaded.

## LED Status Indicators

| Race State | LED Color    | Meaning                                     |
|------------|--------------|---------------------------------------------|
| Waiting    | Red          | System initialized, waiting for cars         |
| Ready      | Amber (R+G)  | Cars loaded, ready to start race            |
| Racing     | Blue         | Race in progress                            |
| Finished   | Green        | Race complete, results available            |

## Configuration

- **DEBUG Mode**: Set the `#define DEBUG` flag to true in the code to enable detailed debug logging of sensor readings.
- **Debounce Delay**: The `DEBOUNCE_DELAY` constant (default: 50ms) can be adjusted to fine-tune button responsiveness.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements

- **VL53L0X Library**: The VL53L0X library used in this project allows communication with the VL53L0X distance sensors.
- **Arduino Platform**: The Arduino platform powers the system and handles all sensor and relay management.

Enjoy your CO₂ Car Race Timer! Let me know if you have any questions or improvements to suggest.