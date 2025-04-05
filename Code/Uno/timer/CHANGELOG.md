# Changelog

All notable changes to this project will be documented in this file.

---
## [0.4.0] - 2025-04-06

### Added:
- **LED Indicators**: 
  - **Waiting**: Red LED on, indicating that cars are not loaded and the system is waiting.
  - **Ready**: Red and Green LEDs on, indicating that cars are loaded and the system is ready for the race.
  - **Racing**: Blue LED blinking, indicating that the race is in progress.
  - **Finished**: Green LED on, indicating that the race is finished and the results are available.
- **Start Button Support**: You can now start the race using a physical momentary pushbutton (in addition to the `'S'` serial command).
- **Debounce Logic for Buttons**: Debounced handling for both Load and Start buttons for reliable edge-triggered inputs.
- **Buzzer Feedback**: 
  - Short beep at **race start**.
  - Long beep at **race finish** for clear audible feedback.
- **Race State LED Indicators**: LEDs now reflect the race state: waiting, ready, racing, and finished.

### Changed:
- **Unified Start Logic**: Both physical button and serial `'S'` command use the same start routine with all conditions checked (e.g., cars must be loaded).
- **Improved Serial Messaging**: Clearer prompts and warnings depending on system state (e.g., trying to start without loading cars).

---

## [0.3.1] - 2025-04-04
### Bug Fixes:
- **Button Press Handling**: Fixed button press handling to ensure proper edge detection.
- **Multiple Race Starts Prevention**: Prevents accidental multiple race starts by handling edge cases where 'S' is pressed too quickly.

---

## [0.3.0] - 2025-04-04
### Added:
- **Car Load Detection**: The race can only be started once cars are loaded. This can be done by pressing the load button or sending the 'L' command through the serial monitor.
- **Race Start Trigger**: The race can only start after cars are loaded. This can be triggered by pressing the start button or sending the 'S' command through the serial monitor.
- **Momentary Switch Support**: Supports momentary switches for both load and start buttons.
- **Load/Start Button Edge Detection**: Only triggers actions on button press release (edge detection), preventing multiple inputs during a single press.
  
### Changed:
- **Race Start Prevention**: The race will not start until cars are loaded. The system prompts the user to load the cars first if 'S' is pressed before loading.
- **Unload Cars on Race Start**: The `carsLoaded` flag is reset when the race starts, ensuring that the system knows cars are no longer loaded.
- **Serial Commands**: 
    - 'L' command to load cars (can also use the load button).
    - 'S' command to start the race (can also use the start button).

---

## [v0.2.0] - 2025-04-04

### Added
- ✅ Optional debug logging: added `#define DEBUG` flag to toggle sensor distance output.
- ✅ About section at the top of the code with project description and instructions.

### Changed
- 🔁 Refactored `startRace()` to trigger relay and start timer simultaneously for improved accuracy.

---

## [v0.1.0] - 2025-04-03

### Added
- 🎯 Basic CO₂ car race timer functionality using two VL53L0X distance sensors.
- 🔌 Relay control to simulate CO₂ canister firing.
- 🖥 Serial interface for race start (`'S'` command) and result output.
- ⏱ Millisecond timing for both lanes with automatic winner declaration.
- 🔄 Automatic system reset after each race for quick repeat runs.
