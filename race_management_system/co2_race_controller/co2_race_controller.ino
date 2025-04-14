/*
 * CO2 Car Race Controller for ESP32
 * 
 * Features:
 * - Two VL53L0X distance sensors to track cars
 * - Relay control for CO2 mechanism
 * - Car loaded button must be pressed before race can start
 * - Start button to begin race
 * - RGB LED status indicator
 */

#include <Wire.h>
#include <VL53L0X.h>
#include <ArduinoJson.h>

// Pin definitions
#define I2C_SDA             21
#define I2C_SCL             22
#define SENSOR1_XSHUT       16  // Address: 0x30
#define SENSOR2_XSHUT       17  // Address: 0x31
#define CAR_LOADED_BUTTON   4   // With internal pullup
#define START_BUTTON        13  // With internal pullup
#define RELAY_PIN           14  // Active LOW
#define LED_RED             25  // Active HIGH
#define LED_GREEN           26  // Active HIGH
#define LED_BLUE            33  // Active HIGH

// VL53L0X sensors
VL53L0X sensor1;
VL53L0X sensor2;

// System state
enum SystemState {
  STATE_IDLE,              // System is idle, waiting for cars to be loaded
  STATE_CARS_LOADED,       // Cars are loaded, ready to start race
  STATE_RACE_READY,        // Race is ready to start (car loaded btn pressed)
  STATE_COUNTDOWN,         // Countdown before race start
  STATE_RACING,            // Race in progress
  STATE_RACE_FINISHED      // Race completed
};

// Race data
struct RaceData {
  bool car1_finished;
  bool car2_finished;
  unsigned long car1_time;
  unsigned long car2_time;
  unsigned long race_start_time;
  String winner;
  int race_id;  // Add this field to store the race ID
};

// Global variables
SystemState currentState = STATE_IDLE;
RaceData raceData;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool lastCarLoadedState = HIGH;
bool lastStartButtonState = HIGH;
bool sensorCalibrated = false;
StaticJsonDocument<256> lastCommandParams; // Store last command parameters

// Distance threshold for car detection (in mm)
const int DETECTION_THRESHOLD = 100;

// Default sensor readings (when no car is present)
int sensor1DefaultReading = 0;
int sensor2DefaultReading = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("CO2 Car Race Controller starting...");
  
  // Configure pins
  pinMode(SENSOR1_XSHUT, OUTPUT);
  pinMode(SENSOR2_XSHUT, OUTPUT);
  pinMode(CAR_LOADED_BUTTON, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  // Set initial pin states
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF (active LOW)
  digitalWrite(SENSOR1_XSHUT, LOW);
  digitalWrite(SENSOR2_XSHUT, LOW);
  
  // Run a hardware test at startup
  testHardware();
  
  // Set LED to RED initially (system not ready)
  setLedColor(true, false, false);
  
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Initialize VL53L0X sensors with different addresses
  initSensors();
  
  // Calibrate sensors
  calibrateSensors();
  
  // Set LED to BLUE (system ready, waiting for cars)
  setLedColor(false, false, true);
  
  // Reset race data
  resetRaceData();
  
  // Send initial status
  sendStatus();
}

void loop() {
  // Process any incoming serial commands
  checkSerial();
  
  // Read sensors
  int distance1 = sensor1.readRangeSingleMillimeters();
  int distance2 = sensor2.readRangeSingleMillimeters();
  
  // Handle button presses
  handleButtons();
  
  // State machine
  switch (currentState) {
    case STATE_IDLE:
      // In idle state, waiting for cars to be loaded
      setLedColor(false, false, true);  // BLUE
      break;
      
    case STATE_CARS_LOADED:
      // Cars are loaded, waiting for start button
      setLedColor(false, true, false);  // GREEN
      break;
      
    case STATE_RACE_READY:
      // Ready to start race, waiting for start button
      setLedColor(false, true, true);   // CYAN
      break;
      
    case STATE_COUNTDOWN:
      // Currently in countdown before race start
      // This is handled by the startRace() function
      break;
      
    case STATE_RACING:
      // Race in progress, check for finish
      setLedColor(true, true, false);   // YELLOW
      
      // Check if car 1 has crossed the finish line
      if (!raceData.car1_finished && 
          distance1 < (sensor1DefaultReading - DETECTION_THRESHOLD)) {
        raceData.car1_finished = true;
        raceData.car1_time = millis() - raceData.race_start_time;
        sendRaceUpdate();
      }
      
      // Check if car 2 has crossed the finish line
      if (!raceData.car2_finished && 
          distance2 < (sensor2DefaultReading - DETECTION_THRESHOLD)) {
        raceData.car2_finished = true;
        raceData.car2_time = millis() - raceData.race_start_time;
        sendRaceUpdate();
      }
      
      // Check if race is complete
      if (raceData.car1_finished && raceData.car2_finished) {
        currentState = STATE_RACE_FINISHED;
        finishRace();
      }
      break;
      
    case STATE_RACE_FINISHED:
      // Race completed, flash LED based on winner
      if (raceData.winner == "car1") {
        blinkLed(LED_RED);
      } else if (raceData.winner == "car2") {
        blinkLed(LED_GREEN);
      } else {
        blinkLed(LED_BLUE);  // Tie or error
      }
      break;
  }
  
  // Send sensor data every 100ms
  static unsigned long lastSensorUpdateTime = 0;
  if (millis() - lastSensorUpdateTime > 100) {
    sendSensorData(distance1, distance2);
    lastSensorUpdateTime = millis();
  }
  
  // Small delay to avoid overwhelming the serial port
  delay(5); // Reduced from 10ms to 5ms for better responsiveness
}

void initSensors() {
  // Reset both sensors
  digitalWrite(SENSOR1_XSHUT, LOW);
  digitalWrite(SENSOR2_XSHUT, LOW);
  delay(10);
  
  // Initialize first sensor with address 0x30
  digitalWrite(SENSOR1_XSHUT, HIGH);
  delay(10);
  sensor1.init();
  sensor1.setTimeout(500);
  sensor1.setAddress(0x30);
  
  // Initialize second sensor with address 0x31
  digitalWrite(SENSOR2_XSHUT, HIGH);
  delay(10);
  sensor2.init();
  sensor2.setTimeout(500);
  sensor2.setAddress(0x31);
  
  // Set long range mode for both sensors
  sensor1.setSignalRateLimit(0.1);
  sensor1.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor1.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  
  sensor2.setSignalRateLimit(0.1);
  sensor2.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor2.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
}

void calibrateSensors() {
  setLedColor(true, true, true); // WHITE during calibration
  
  // Take multiple readings and average them for better accuracy
  const int numReadings = 15; // Increased from 10 to 15 readings for better accuracy
  int sum1 = 0, sum2 = 0;
  int validReadings1 = 0;
  int validReadings2 = 0;
  
  // Reset calibration flag initially
  sensorCalibrated = false;
  
  // Send calibration start message
  StaticJsonDocument<200> startDoc;
  startDoc["type"] = "calibration_start";
  String startJson;
  serializeJson(startDoc, startJson);
  Serial.println(startJson);
  
  // Take multiple readings with error checking
  for (int i = 0; i < numReadings; i++) {
    int reading1 = sensor1.readRangeSingleMillimeters();
    int reading2 = sensor2.readRangeSingleMillimeters();
    
    // Check if readings are valid (not timeout)
    if (!sensor1.timeoutOccurred() && reading1 > 0 && reading1 < 8000) {
      sum1 += reading1;
      validReadings1++;
    }
    
    if (!sensor2.timeoutOccurred() && reading2 > 0 && reading2 < 8000) {
      sum2 += reading2;
      validReadings2++;
    }
    
    // Flash LED to indicate calibration in progress
    if (i % 2 == 0) {
      setLedColor(true, true, true); // WHITE
    } else {
      setLedColor(false, false, false); // OFF
    }
    
    delay(100); // Increased from 50ms to 100ms for more stable readings
  }
  
  // Calculate averages if we have valid readings
  if (validReadings1 > 0 && validReadings2 > 0) {
    sensor1DefaultReading = sum1 / validReadings1;
    sensor2DefaultReading = sum2 / validReadings2;
    sensorCalibrated = true;
  } else {
    // If calibration failed, set some default values and mark as not calibrated
    sensor1DefaultReading = 500; // Default value
    sensor2DefaultReading = 500; // Default value
    sensorCalibrated = false;
  }
  
  // Send calibration results
  StaticJsonDocument<256> doc;
  doc["type"] = "calibration";
  doc["sensor1_baseline"] = sensor1DefaultReading;
  doc["sensor2_baseline"] = sensor2DefaultReading;
  doc["success"] = sensorCalibrated;
  doc["valid_readings_1"] = validReadings1;
  doc["valid_readings_2"] = validReadings2;
  doc["sensors_calibrated"] = sensorCalibrated;
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
  
  // Set LED based on calibration result
  if (sensorCalibrated) {
    // Blink green 3 times to indicate success
    for (int i = 0; i < 3; i++) {
      setLedColor(false, true, false); // GREEN
      delay(200);
      setLedColor(false, false, false); // OFF
      delay(200);
    }
  } else {
    // Blink red 3 times to indicate failure
    for (int i = 0; i < 3; i++) {
      setLedColor(true, false, false); // RED
      delay(200);
      setLedColor(false, false, false); // OFF
      delay(200);
    }
  }
}

void resetRaceData() {
  raceData.car1_finished = false;
  raceData.car2_finished = false;
  raceData.car1_time = 0;
  raceData.car2_time = 0;
  raceData.race_start_time = 0;
  raceData.winner = "";
  raceData.race_id = 0;
}

void handleButtons() {
  // Debug - report button states every 5 seconds
  static unsigned long lastButtonDebugTime = 0;
  if (millis() - lastButtonDebugTime > 5000) {
    bool carLoadedReading = digitalRead(CAR_LOADED_BUTTON);
    bool startButtonReading = digitalRead(START_BUTTON);
    
    // Report button states via serial
    StaticJsonDocument<128> btnDoc;
    btnDoc["type"] = "button_status";
    btnDoc["car_loaded_button"] = carLoadedReading == LOW ? "PRESSED" : "RELEASED";
    btnDoc["start_button"] = startButtonReading == LOW ? "PRESSED" : "RELEASED";
    btnDoc["car_loaded_state"] = lastCarLoadedState == LOW ? "PRESSED" : "RELEASED";
    btnDoc["start_button_state"] = lastStartButtonState == LOW ? "PRESSED" : "RELEASED";
    String btnJson;
    serializeJson(btnDoc, btnJson);
    Serial.println(btnJson);
    
    // If button has been held down for multiple debug cycles, force state change
    if (carLoadedReading == LOW && lastCarLoadedState == LOW && currentState == STATE_IDLE) {
      StaticJsonDocument<128> btnHoldDoc;
      btnHoldDoc["type"] = "button_hold_detected";
      btnHoldDoc["button"] = "car_loaded";
      btnHoldDoc["action"] = "forcing_state_change";
      String btnHoldJson;
      serializeJson(btnHoldDoc, btnHoldJson);
      Serial.println(btnHoldJson);
      
      // Force state change
      setStateToLoaded();
    }
    
    lastButtonDebugTime = millis();
  }

  // Read button states
  bool carLoadedReading = digitalRead(CAR_LOADED_BUTTON);
  bool startButtonReading = digitalRead(START_BUTTON);
  
  // Track previous readings for edge detection
  static bool prevCarLoadedReading = HIGH;
  static bool prevStartButtonReading = HIGH;
  
  // Button edge detection - detect the moment the button is pressed (HIGH to LOW transition)
  if (prevCarLoadedReading == HIGH && carLoadedReading == LOW && currentState == STATE_IDLE) {
    // Button has just been pressed - send notification
    StaticJsonDocument<128> btnDoc;
    btnDoc["type"] = "button_pressed_edge";
    btnDoc["button"] = "car_loaded";
    btnDoc["previous_state"] = getStateString(currentState);
    String btnJson;
    serializeJson(btnDoc, btnJson);
    Serial.println(btnJson);
    
    // Change state immediately on button press
    setStateToLoaded();
  }
  
  // Start button edge detection - detect the moment the button is pressed (HIGH to LOW transition)
  if (prevStartButtonReading == HIGH && startButtonReading == LOW) {
    // Button has just been pressed - send notification
    StaticJsonDocument<128> btnDoc;
    btnDoc["type"] = "button_pressed_edge";
    btnDoc["button"] = "start";
    btnDoc["previous_state"] = getStateString(currentState);
    String btnJson;
    serializeJson(btnDoc, btnJson);
    Serial.println(btnJson);
    
    // Handle start button press immediately
    if (currentState == STATE_CARS_LOADED) {
      currentState = STATE_RACE_READY;
      startRace();
    } else if (currentState == STATE_RACE_FINISHED) {
      // Reset after race is finished
      resetRaceData();
      currentState = STATE_IDLE;
      sendStatus();
    }
  }
  
  // Update previous readings for next cycle
  prevCarLoadedReading = carLoadedReading;
  prevStartButtonReading = startButtonReading;
  
  // We have a more immediate response for button state changes
  // Check if car loaded button changed state
  if (carLoadedReading != lastCarLoadedState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Car loaded button pressed or released
    if (carLoadedReading != lastCarLoadedState) {
      // Button state changed and is stable
      if (carLoadedReading == LOW) {
        // Button is pressed (active LOW)
        if (currentState == STATE_IDLE) {
          // Send button press notification
          StaticJsonDocument<128> btnDoc;
          btnDoc["type"] = "button_pressed";
          btnDoc["button"] = "car_loaded";
          btnDoc["previous_state"] = getStateString(currentState);
          String btnJson;
          serializeJson(btnDoc, btnJson);
          Serial.println(btnJson);
          
          // Change state to cars loaded
          currentState = STATE_CARS_LOADED;
          
          // Flash green LED to confirm
          for (int i = 0; i < 3; i++) {
            digitalWrite(LED_GREEN, HIGH);
            delay(100);
            digitalWrite(LED_GREEN, LOW);
            delay(100);
          }
          
          // Send immediate status update with new state
          StaticJsonDocument<256> statusDoc;
          statusDoc["type"] = "status";
          statusDoc["race_state"] = getStateString(currentState);
          statusDoc["cars_loaded"] = true; // Explicitly set cars_loaded to true
          statusDoc["race_started"] = false;
          statusDoc["car1_finished"] = false;
          statusDoc["car2_finished"] = false;
          statusDoc["car1_time"] = 0;
          statusDoc["car2_time"] = 0;
          statusDoc["sensors_calibrated"] = sensorCalibrated;
          statusDoc["sensor1_baseline"] = sensor1DefaultReading;
          statusDoc["sensor2_baseline"] = sensor2DefaultReading;
          statusDoc["timestamp"] = millis();
          
          String statusJson;
          serializeJson(statusDoc, statusJson);
          Serial.println(statusJson);
        }
      }
    }
  }
  
  lastCarLoadedState = carLoadedReading;
  
  // Similar improvements for start button
  if (startButtonReading != lastStartButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Start button state changed and is stable
    if (startButtonReading != lastStartButtonState) {
      if (startButtonReading == LOW) {
        // Button is pressed (active LOW)
        
        // Send button press notification
        StaticJsonDocument<128> btnDoc;
        btnDoc["type"] = "button_pressed";
        btnDoc["button"] = "start";
        String btnJson;
        serializeJson(btnDoc, btnJson);
        Serial.println(btnJson);
        
        if (currentState == STATE_CARS_LOADED) {
          currentState = STATE_RACE_READY;
          startRace();
        } else if (currentState == STATE_RACE_FINISHED) {
          // Reset after race is finished
          resetRaceData();
          currentState = STATE_IDLE;
          sendStatus();
        }
      }
    }
  }
  
  lastStartButtonState = startButtonReading;
}

void startRace() {
  setLedColor(true, false, true); // PURPLE for countdown
  
  // Get race ID from the last command if available
  if (lastCommandParams.containsKey("race_id")) {
    raceData.race_id = lastCommandParams["race_id"].as<int>();
  } else if (lastCommandParams.containsKey("heat_id")) {
    raceData.race_id = lastCommandParams["heat_id"].as<int>();
  } else {
    raceData.race_id = 0; // Default if no ID provided
  }
  
  // Send race start notification
  StaticJsonDocument<200> doc;
  doc["type"] = "race_start";
  doc["countdown"] = 3;
  doc["race_id"] = raceData.race_id;
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
  
  // Countdown from 3
  for (int i = 3; i > 0; i--) {
    // Blink LED according to countdown
    for (int j = 0; j < i; j++) {
      setLedColor(true, false, true); // PURPLE
      delay(250);
      setLedColor(false, false, false); // OFF
      delay(250);
    }
    
    // Send countdown update
    doc["countdown"] = i - 1;
    jsonString = "";
    serializeJson(doc, jsonString);
    Serial.println(jsonString);
  }
  
  // Fire the relay to release CO2
  digitalWrite(RELAY_PIN, LOW);  // Active LOW
  delay(500);  // Increased from 100ms to 500ms for more reliable relay activation
  digitalWrite(RELAY_PIN, HIGH); // Turn off relay
  
  // Set race state and start time
  currentState = STATE_RACING;
  raceData.race_start_time = millis();
  
  // Send race started message
  doc["type"] = "race_started";
  doc["timestamp"] = raceData.race_start_time;
  jsonString = "";
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void finishRace() {
  // Determine winner
  if (raceData.car1_time < raceData.car2_time) {
    raceData.winner = "car1";
  } else if (raceData.car2_time < raceData.car1_time) {
    raceData.winner = "car2";
  } else {
    raceData.winner = "tie";
  }
  
  // Create the JSON document for results
  StaticJsonDocument<256> doc;
  doc["car1_time"] = raceData.car1_time;
  doc["car2_time"] = raceData.car2_time;
  doc["winner"] = raceData.winner;
  doc["race_id"] = raceData.race_id;  // Include the race ID
  
  String jsonString;
  
  // Send race_result message (original)
  doc["type"] = "race_result";
  jsonString = "";
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
  
  // Send race_completed message (for frontend compatibility)
  doc["type"] = "race_completed";
  jsonString = "";
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
  
  // Flash LEDs to indicate race completion
  for (int i = 0; i < 3; i++) {
    if (raceData.winner == "car1") {
      setLedColor(true, false, false); // RED for car1
    } else if (raceData.winner == "car2") {
      setLedColor(false, true, false); // GREEN for car2
    } else {
      setLedColor(false, false, true); // BLUE for tie
    }
    delay(200);
    setLedColor(false, false, false); // OFF
    delay(200);
  }
  
  // Wait 3 seconds to show the race result before auto-resetting
  delay(3000);
  
  // Send message about auto-reset
  StaticJsonDocument<128> resetDoc;
  resetDoc["type"] = "auto_reset";
  resetDoc["message"] = "Auto-resetting to IDLE state";
  jsonString = "";
  serializeJson(resetDoc, jsonString);
  Serial.println(jsonString);
  
  // Reset race data and return to IDLE state
  resetRaceData();
  currentState = STATE_IDLE;
  
  // Set LED back to BLUE (system ready)
  setLedColor(false, false, true);
  
  // Send status update with the new state
  sendStatus();
}

void sendSensorData(int distance1, int distance2) {
  StaticJsonDocument<200> doc;
  doc["type"] = "sensor_reading";
  doc["sensor1"] = distance1;
  doc["sensor2"] = distance2;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void sendRaceUpdate() {
  StaticJsonDocument<256> doc;
  doc["type"] = "race_update";
  doc["car1_finished"] = raceData.car1_finished;
  doc["car2_finished"] = raceData.car2_finished;
  doc["car1_time"] = raceData.car1_time;
  doc["car2_time"] = raceData.car2_time;
  doc["elapsed_time"] = millis() - raceData.race_start_time;
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void sendStatus() {
  StaticJsonDocument<256> doc;
  doc["type"] = "status";
  doc["race_state"] = getStateString(currentState);
  
  // Explicitly set cars_loaded based on current state
  bool carsLoaded = (currentState == STATE_CARS_LOADED || 
                    currentState == STATE_RACE_READY || 
                    currentState == STATE_COUNTDOWN || 
                    currentState == STATE_RACING || 
                    currentState == STATE_RACE_FINISHED);
  doc["cars_loaded"] = carsLoaded;
  
  doc["race_started"] = (currentState == STATE_RACING || currentState == STATE_RACE_FINISHED);
  doc["car1_finished"] = raceData.car1_finished;
  doc["car2_finished"] = raceData.car2_finished;
  doc["car1_time"] = raceData.car1_time;
  doc["car2_time"] = raceData.car2_time;
  doc["sensors_calibrated"] = sensorCalibrated;
  doc["sensor1_baseline"] = sensor1DefaultReading;
  doc["sensor2_baseline"] = sensor2DefaultReading;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

String getStateString(SystemState state) {
  switch (state) {
    case STATE_IDLE: return "IDLE";
    case STATE_CARS_LOADED: return "CARS_LOADED";
    case STATE_RACE_READY: return "RACE_READY";
    case STATE_COUNTDOWN: return "COUNTDOWN";
    case STATE_RACING: return "RACING";
    case STATE_RACE_FINISHED: return "RACE_FINISHED";
    default: return "UNKNOWN";
  }
}

void setLedColor(bool red, bool green, bool blue) {
  digitalWrite(LED_RED, red ? HIGH : LOW);
  digitalWrite(LED_GREEN, green ? HIGH : LOW);
  digitalWrite(LED_BLUE, blue ? HIGH : LOW);
}

void blinkLed(int pin) {
  static unsigned long lastBlinkTime = 0;
  static bool ledState = false;
  
  if (millis() - lastBlinkTime > 300) {
    ledState = !ledState;
    digitalWrite(pin, ledState ? HIGH : LOW);
    lastBlinkTime = millis();
  }
}

// Check for serial commands - called from loop and can be called more frequently
void checkSerial() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();  // Remove any whitespace
    
    // Debug - echo back the received command
    StaticJsonDocument<128> echoDoc;
    echoDoc["type"] = "echo";
    echoDoc["received"] = input;
    String echoJson;
    serializeJson(echoDoc, echoJson);
    Serial.println(echoJson);
    
    // First check for direct string commands (non-JSON)
    // This allows for simpler command handling from the web UI
    if (input == "testrace") {
      // This is a direct command for testing race start
      Serial.println("{\"type\":\"direct_command\",\"cmd\":\"testrace\",\"message\":\"Starting test race\"}");
      
      // Always set cars loaded first if needed
      if (currentState == STATE_IDLE) {
        currentState = STATE_CARS_LOADED;
        // Flash green LED to confirm
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_GREEN, HIGH);
          delay(100);
          digitalWrite(LED_GREEN, LOW);
          delay(100);
        }
      }
      
      if (currentState == STATE_CARS_LOADED) {
        currentState = STATE_RACE_READY;
        startRace();
        Serial.println("{\"type\":\"success\",\"message\":\"Test race started\"}");
      } else {
        Serial.println("{\"type\":\"error\",\"message\":\"Cannot start test race in current state\"}");
      }
      return;
    }
    else if (input == "calibrate") {
      // Direct command for sensor calibration
      Serial.println("{\"type\":\"direct_command\",\"cmd\":\"calibrate\",\"message\":\"Starting calibration\"}");
      
      // Visual feedback
      digitalWrite(LED_RED, HIGH);
      delay(200);
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
      delay(200);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, HIGH);
      delay(200);
      digitalWrite(LED_BLUE, LOW);
      
      calibrateSensors();
      sendStatus();
      Serial.println("{\"type\":\"success\",\"message\":\"Calibration complete\"}");
      return;
    }
    else if (input == "resetTimer") {
      // Direct command for resetting the timer
      Serial.println("{\"type\":\"direct_command\",\"cmd\":\"resetTimer\",\"message\":\"Resetting timer\"}");
      
      resetRaceData();
      currentState = STATE_IDLE;
      sendStatus();
      Serial.println("{\"type\":\"success\",\"message\":\"Timer reset complete\"}");
      return;
    }
    else if (input == "carLoaded") {
      // Direct command for setting cars loaded
      Serial.println("{\"type\":\"direct_command\",\"cmd\":\"carLoaded\",\"message\":\"Setting cars loaded\"}");
      
      if (currentState == STATE_IDLE) {
        currentState = STATE_CARS_LOADED;
        
        // Flash green LED to confirm
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_GREEN, HIGH);
          delay(100);
          digitalWrite(LED_GREEN, LOW);
          delay(100);
        }
        
        sendStatus();
        Serial.println("{\"type\":\"success\",\"message\":\"Cars loaded successfully\"}");
      } else {
        Serial.println("{\"type\":\"error\",\"message\":\"Can only set cars loaded in IDLE state\"}");
      }
      return;
    }
    else if (input == "forceReset") {
      // Direct command for force reset
      Serial.println("{\"type\":\"direct_command\",\"cmd\":\"forceReset\",\"message\":\"Force resetting system\"}");
      
      resetRaceData();
      currentState = STATE_IDLE;
      
      // Flash all LEDs to confirm reset
      for (int i = 0; i < 3; i++) {
        setLedColor(true, true, true); // WHITE
        delay(200);
        setLedColor(false, false, false); // OFF
        delay(200);
      }
      
      // Set back to BLUE (system ready, waiting for cars)
      setLedColor(false, false, true);
      
      sendStatus();
      Serial.println("{\"type\":\"success\",\"message\":\"System forcibly reset to IDLE\"}");
      return;
    }
    else if (input == "status") {
      // Direct command for status update
      Serial.println("{\"type\":\"direct_command\",\"cmd\":\"status\",\"message\":\"Requesting status\"}");
      sendStatus();
      return;
    }
    else if (input == "ping") {
      // Simple ping command for testing connection
      Serial.println("{\"type\":\"pong\",\"message\":\"ESP32 is alive\"}");
      return;
    }
    
    // If we get here, it wasn't a direct command, so try to parse as JSON
    try {
      // Parse JSON command
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, input);
      
      if (error) {
        // Failed to parse as JSON
        Serial.println("{\"type\":\"error\",\"message\":\"Invalid JSON command\"}");
        return;
      }
      
      // Extract command
      const char* cmd = doc["cmd"];
      
      // Store the command parameters for later use
      lastCommandParams = doc;
      
      if (cmd) {
        // Handle commands
        if (strcmp(cmd, "start_race") == 0 || strcmp(cmd, "startRace") == 0) {
          // Log the start race command
          Serial.println("{\"type\":\"command_received\",\"cmd\":\"start_race\"}");
          
          // Start a race (if in correct state)
          if (currentState == STATE_CARS_LOADED || currentState == STATE_RACE_READY) {
            currentState = STATE_RACE_READY;
            startRace();
          } else {
            Serial.println("{\"type\":\"error\",\"message\":\"Cannot start race in current state\"}");
          }
        }
        else if (strcmp(cmd, "reset_timer") == 0) {
          // Reset the race timer
          Serial.println("{\"type\":\"command_received\",\"cmd\":\"reset_timer\"}");
          resetRaceData();
          currentState = STATE_IDLE;
          sendStatus();
        }
        else if (strcmp(cmd, "calibrate") == 0) {
          // Immediately show visual feedback that command was received
          // Flash RED-GREEN-BLUE in sequence to show command received
          digitalWrite(LED_RED, HIGH);
          delay(200);
          digitalWrite(LED_RED, LOW);
          digitalWrite(LED_GREEN, HIGH);
          delay(200);
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_BLUE, HIGH);
          delay(200);
          digitalWrite(LED_BLUE, LOW);
          
          // Send acknowledgment before starting calibration
          StaticJsonDocument<128> ackDoc;
          ackDoc["type"] = "calibrate_ack";
          ackDoc["message"] = "Starting calibration...";
          String ackJson;
          serializeJson(ackDoc, ackJson);
          Serial.println(ackJson);
          
          // Recalibrate sensors
          calibrateSensors();
          sendStatus();
        }
        else if (strcmp(cmd, "car_loaded") == 0) {
          // Simulate car loaded button press
          StaticJsonDocument<128> ackDoc;
          ackDoc["type"] = "car_loaded_ack";
          ackDoc["message"] = "Setting cars loaded status...";
          ackDoc["previous_state"] = getStateString(currentState);
          String ackJson;
          serializeJson(ackDoc, ackJson);
          Serial.println(ackJson);
          
          // Set state to cars loaded
          if (currentState == STATE_IDLE) {
            currentState = STATE_CARS_LOADED;
            
            // Flash green LED to confirm
            for (int i = 0; i < 3; i++) {
              digitalWrite(LED_GREEN, HIGH);
              delay(100);
              digitalWrite(LED_GREEN, LOW);
              delay(100);
            }
            
            // Send immediate status update with new state
            StaticJsonDocument<256> statusDoc;
            statusDoc["type"] = "status";
            statusDoc["race_state"] = getStateString(currentState);
            statusDoc["cars_loaded"] = true; // Explicitly set cars_loaded to true
            statusDoc["race_started"] = false;
            statusDoc["car1_finished"] = false;
            statusDoc["car2_finished"] = false;
            statusDoc["car1_time"] = 0;
            statusDoc["car2_time"] = 0;
            statusDoc["sensors_calibrated"] = sensorCalibrated;
            statusDoc["sensor1_baseline"] = sensor1DefaultReading;
            statusDoc["sensor2_baseline"] = sensor2DefaultReading;
            statusDoc["timestamp"] = millis();
            
            String statusJson;
            serializeJson(statusDoc, statusJson);
            Serial.println(statusJson);
            
            // Send success confirmation
            StaticJsonDocument<128> successDoc;
            successDoc["type"] = "success";
            successDoc["message"] = "Cars loaded successfully";
            successDoc["new_state"] = getStateString(currentState);
            String successJson;
            serializeJson(successDoc, successJson);
            Serial.println(successJson);
          } else {
            // Send error if not in idle state
            StaticJsonDocument<128> errDoc;
            errDoc["type"] = "error";
            errDoc["message"] = "Can only set cars loaded in IDLE state";
            errDoc["current_state"] = getStateString(currentState);
            
            String errJson;
            serializeJson(errDoc, errJson);
            Serial.println(errJson);
          }
        }
        else if (strcmp(cmd, "force_reset") == 0) {
          // Force reset the system to IDLE state
          StaticJsonDocument<128> resetAckDoc;
          resetAckDoc["type"] = "force_reset_ack";
          resetAckDoc["previous_state"] = getStateString(currentState);
          String resetAckJson;
          serializeJson(resetAckDoc, resetAckJson);
          Serial.println(resetAckJson);
          
          // Reset the system
          resetRaceData();
          currentState = STATE_IDLE;
          
          // Flash all LEDs to confirm reset
          for (int i = 0; i < 3; i++) {
            setLedColor(true, true, true); // WHITE
            delay(200);
            setLedColor(false, false, false); // OFF
            delay(200);
          }
          
          // Set back to BLUE (system ready, waiting for cars)
          setLedColor(false, false, true);
          
          // Send immediate status update
          sendStatus();
          
          // Send success confirmation
          StaticJsonDocument<128> resetSuccessDoc;
          resetSuccessDoc["type"] = "success";
          resetSuccessDoc["message"] = "System forcibly reset to IDLE";
          resetSuccessDoc["new_state"] = getStateString(currentState);
          String resetSuccessJson;
          serializeJson(resetSuccessDoc, resetSuccessJson);
          Serial.println(resetSuccessJson);
        }
        else if (strcmp(cmd, "fire_relay") == 0) {
          // Fire the relay directly without starting a race
          StaticJsonDocument<128> relayAckDoc;
          relayAckDoc["type"] = "relay_ack";
          relayAckDoc["message"] = "Firing relay...";
          String relayAckJson;
          serializeJson(relayAckDoc, relayAckJson);
          Serial.println(relayAckJson);
          
          // Flash RED LED briefly to indicate relay firing
          setLedColor(true, false, false); // RED
          delay(100);
          
          // Fire the relay
          digitalWrite(RELAY_PIN, LOW);  // Active LOW
          delay(500);  // 500ms for reliable relay activation
          digitalWrite(RELAY_PIN, HIGH); // Turn off relay
          
          // Revert LED color to current state
          if (currentState == STATE_RACING) {
            setLedColor(true, true, false); // YELLOW for racing
          } else {
            setLedColor(false, false, true); // BLUE default
          }
          
          // Send success confirmation
          StaticJsonDocument<128> relaySuccessDoc;
          relaySuccessDoc["type"] = "success";
          relaySuccessDoc["message"] = "Relay fired successfully";
          String relaySuccessJson;
          serializeJson(relaySuccessDoc, relaySuccessJson);
          Serial.println(relaySuccessJson);
        }
      }
    } catch (const std::exception& e) {
      // Handle any exceptions that might occur during JSON parsing
      StaticJsonDocument<128> errDoc;
      errDoc["type"] = "error";
      errDoc["message"] = "Exception occurred during command processing";
      errDoc["exception"] = e.what();
      
      String errJson;
      serializeJson(errDoc, errJson);
      Serial.println(errJson);
    }
  }
}

// Test LEDs and hardware components
void testHardware() {
  // Send hardware test message
  StaticJsonDocument<128> testDoc;
  testDoc["type"] = "hardware_test";
  testDoc["message"] = "Testing hardware...";
  String testJson;
  serializeJson(testDoc, testJson);
  Serial.println(testJson);
  
  // Test each LED individually
  digitalWrite(LED_RED, HIGH);
  delay(300);
  digitalWrite(LED_RED, LOW);
  
  digitalWrite(LED_GREEN, HIGH);
  delay(300);
  digitalWrite(LED_GREEN, LOW);
  
  digitalWrite(LED_BLUE, HIGH);
  delay(300);
  digitalWrite(LED_BLUE, LOW);
  
  // Test relay (brief pulse) - removed to prevent relay clicking during boot
  // digitalWrite(RELAY_PIN, LOW);  // Active LOW
  // delay(50);
  // digitalWrite(RELAY_PIN, HIGH); // Turn off relay
  
  // Test car loaded pin
  pinMode(CAR_LOADED_BUTTON, INPUT_PULLUP);
  delay(10);
  
  // Read button states for diagnostic purposes
  bool carLoadedState = digitalRead(CAR_LOADED_BUTTON);
  bool startBtnState = digitalRead(START_BUTTON);
  
  // Report button states
  StaticJsonDocument<128> btnDoc;
  btnDoc["type"] = "button_status";
  btnDoc["car_loaded_button"] = carLoadedState == LOW ? "PRESSED" : "RELEASED";
  btnDoc["start_button"] = startBtnState == LOW ? "PRESSED" : "RELEASED";
  btnDoc["car_loaded_pin"] = CAR_LOADED_BUTTON;
  btnDoc["start_button_pin"] = START_BUTTON;
  String btnJson;
  serializeJson(btnDoc, btnJson);
  Serial.println(btnJson);
  
  // Send a simulated car_loaded command to test the functionality
  StaticJsonDocument<128> testCmd;
  testCmd["cmd"] = "car_loaded";
  testCmd["test"] = true;
  String testCmdJson;
  serializeJson(testCmd, testCmdJson);
  Serial.println("Simulating car_loaded command during hardware test:");
  Serial.println(testCmdJson);
  
  // Process the car_loaded command for testing
  if (currentState == STATE_IDLE) {
    StaticJsonDocument<128> testResponse;
    testResponse["type"] = "test_car_loaded";
    testResponse["state_before"] = getStateString(currentState);
    
    // Set state to cars loaded temporarily for testing
    SystemState previousState = currentState;
    currentState = STATE_CARS_LOADED;
    
    testResponse["state_after"] = getStateString(currentState);
    String testResponseJson;
    serializeJson(testResponse, testResponseJson);
    Serial.println(testResponseJson);
    
    // Revert to previous state
    currentState = previousState;
  }
  
  // Test complete
  testDoc["message"] = "Hardware test complete";
  testJson = "";
  serializeJson(testDoc, testJson);
  Serial.println(testJson);
}

// Add a new function to directly set the cars loaded state
void setStateToLoaded() {
  // Only change state if we're in IDLE state
  if (currentState == STATE_IDLE) {
    // Send state change notification
    StaticJsonDocument<128> stateDoc;
    stateDoc["type"] = "state_change";
    stateDoc["previous_state"] = getStateString(currentState);
    
    // Change state to cars loaded
    currentState = STATE_CARS_LOADED;
    
    stateDoc["new_state"] = getStateString(currentState);
    String stateJson;
    serializeJson(stateDoc, stateJson);
    Serial.println(stateJson);
    
    // Flash green LED to confirm
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_GREEN, HIGH);
      delay(100);
      digitalWrite(LED_GREEN, LOW);
      delay(100);
    }
    
    // Send immediate status update with new state
    sendStatus();
  }
} 