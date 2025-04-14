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
 #define START_BUTTON        5   // With internal pullup
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
 };
 
 // Global variables
 SystemState currentState = STATE_IDLE;
 RaceData raceData;
 unsigned long lastDebounceTime = 0;
 unsigned long debounceDelay = 50;
 bool lastCarLoadedState = HIGH;
 bool lastStartButtonState = HIGH;
 bool sensorCalibrated = false;
 
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
   delay(10);
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
   const int numReadings = 10;
   int sum1 = 0, sum2 = 0;
   
   for (int i = 0; i < numReadings; i++) {
     sum1 += sensor1.readRangeSingleMillimeters();
     sum2 += sensor2.readRangeSingleMillimeters();
     delay(50);
   }
   
   sensor1DefaultReading = sum1 / numReadings;
   sensor2DefaultReading = sum2 / numReadings;
   
   sensorCalibrated = true;
   
   // Send calibration results
   StaticJsonDocument<200> doc;
   doc["type"] = "calibration";
   doc["sensor1_baseline"] = sensor1DefaultReading;
   doc["sensor2_baseline"] = sensor2DefaultReading;
   doc["success"] = true;
   
   String jsonString;
   serializeJson(doc, jsonString);
   Serial.println(jsonString);
 }
 
 void resetRaceData() {
   raceData.car1_finished = false;
   raceData.car2_finished = false;
   raceData.car1_time = 0;
   raceData.car2_time = 0;
   raceData.race_start_time = 0;
   raceData.winner = "";
 }
 
 void handleButtons() {
   // Read button states
   bool carLoadedReading = digitalRead(CAR_LOADED_BUTTON);
   bool startButtonReading = digitalRead(START_BUTTON);
   
   // Check if car loaded button was pressed (LOW when pressed)
   if (carLoadedReading != lastCarLoadedState) {
     lastDebounceTime = millis();
   }
   
   if ((millis() - lastDebounceTime) > debounceDelay) {
     // Car loaded button pressed
     if (carLoadedReading == LOW && lastCarLoadedState == HIGH) {
       if (currentState == STATE_IDLE) {
         currentState = STATE_CARS_LOADED;
         sendStatus();
       }
     }
   }
   
   lastCarLoadedState = carLoadedReading;
   
   // Check if start button was pressed (LOW when pressed)
   if (startButtonReading != lastStartButtonState) {
     lastDebounceTime = millis();
   }
   
   if ((millis() - lastDebounceTime) > debounceDelay) {
     // Start button pressed
     if (startButtonReading == LOW && lastStartButtonState == HIGH) {
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
   
   lastStartButtonState = startButtonReading;
 }
 
 void startRace() {
   setLedColor(true, false, true); // PURPLE for countdown
   
   // Send race start notification
   StaticJsonDocument<200> doc;
   doc["type"] = "race_start";
   doc["countdown"] = 3;
   
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
   delay(100);
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
   
   // Send race results
   StaticJsonDocument<256> doc;
   doc["type"] = "race_result";
   doc["car1_time"] = raceData.car1_time;
   doc["car2_time"] = raceData.car2_time;
   doc["winner"] = raceData.winner;
   
   String jsonString;
   serializeJson(doc, jsonString);
   Serial.println(jsonString);
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
   doc["cars_loaded"] = (currentState != STATE_IDLE);
   doc["race_started"] = (currentState == STATE_RACING || currentState == STATE_RACE_FINISHED);
   doc["car1_finished"] = raceData.car1_finished;
   doc["car2_finished"] = raceData.car2_finished;
   doc["car1_time"] = raceData.car1_time;
   doc["car2_time"] = raceData.car2_time;
   doc["sensors_calibrated"] = sensorCalibrated;
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
 
 // Handle serial commands from the race management system
 void serialEvent() {
   if (Serial.available()) {
     String input = Serial.readStringUntil('\n');
     
     // Parse JSON command
     StaticJsonDocument<256> doc;
     DeserializationError error = deserializeJson(doc, input);
     
     if (!error) {
       String cmd = doc["cmd"];
       
       if (cmd == "status") {
         // Send current status
         sendStatus();
       }
       else if (cmd == "start_race") {
         // Start race if in correct state
         if (currentState == STATE_CARS_LOADED) {
           currentState = STATE_RACE_READY;
           startRace();
         } else {
           // Send error if cars not loaded
           StaticJsonDocument<128> errDoc;
           errDoc["type"] = "error";
           errDoc["message"] = "Cars not ready";
           
           String errJson;
           serializeJson(errDoc, errJson);
           Serial.println(errJson);
         }
       }
       else if (cmd == "reset") {
         // Reset the system
         resetRaceData();
         currentState = STATE_IDLE;
         sendStatus();
       }
       else if (cmd == "calibrate") {
         // Recalibrate sensors
         calibrateSensors();
         sendStatus();
       }
     }
   }
 } 