# CO₂ Car Race Timer - Tasks

## 📝 Current Tasks

### 🏁 Race Timer Features
- [ ] Implement automatic reset after race finish (timer reset, car states reset).
- [x] Improve relay control for CO₂ firing mechanism to ensure more reliable trigger timing.
- [ ] Add display of race results on an LCD/OLED screen.
    - Display race status, winner, and times for each car.
    - Ensure real-time updates during the race and after the race finishes.

### ⚙️ Button and Input Handling
- [ ] Add debounce logic to prevent false button presses (for load and start buttons).
- [ ] Test with external momentary switches to ensure reliable input detection.
  
### 📊 Debugging and Logging
- [ ] Implement verbose debug logging for troubleshooting (e.g., add logging for relay activation).
- [ ] Add support for logging race time to an SD card or external storage.

### 🟢 LED and Indicators
- [ ] Add LED indicator for **waiting for cars to load** state.
    - LED should be on when cars are not loaded and waiting for the load button press or 'L' command.
    - LED should turn off once cars are loaded and ready for the race.
- [ ] Add LED indicator for **Car Load and Ready to Race** state.
    - LED should turn on when the load button is pressed or 'L' command is received, signaling the cars are loaded and ready.
    - LED should turn off when the race starts.
- [ ] Add LED indicator for **Racing** state.
    - LED should blink while the race is ongoing to show that the race is in progress.
- [ ] Add LED indicator for **Finish Race** state.
    - LED should turn on solid after the race finishes, indicating the race is completed and the winner is determined.
    
### 📦 Code Organization & Documentation
- [x] Update **README.md** with new features and setup instructions.
- [ ] Add more comments to the code for better maintainability and readability.

## ✅ Completed Tasks

### 🏁 Race Timer Features
- [x] Added car load detection (using load button or 'L' command).
- [x] Prevented race start until cars are loaded.
- [x] Added edge detection for buttons to prevent multiple presses during a single action.

### ⚙️ Button and Input Handling
- [x] Added support for momentary switches on load and start buttons.
- [x] Integrated load and start button press handling with edge detection for accurate button state change.

### 📊 Debugging and Logging
- [x] Implemented debug logging for distance sensor readings (toggleable with the DEBUG flag).
