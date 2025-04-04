/*
--- CO₂ Car Race Timer Version 0.3.1 - 05 April 2025 ---
This system uses two VL53L0X distance sensors to time a CO₂-powered car race.
It measures the time taken for each car to cross the sensor line and declares the winner based on the fastest time.

Features:
- Two distance sensors (VL53L0X) track the progress of two cars.
- Relay control to simulate the CO₂ firing mechanism.
- Serial communication to start the race and display results.
- Supports multiple races by resetting after each one.

Instructions:
1. Connect the VL53L0X sensors and CO₂ relay to the specified pins.
2. Open the Serial Monitor and send 'S' to start the race.
3. The sensors will detect the cars as they pass, and the times will be displayed.
4. After each race, the system resets and is ready for a new race.

Hardware:
- Two VL53L0X distance sensors.
- Relay connected to fire CO₂ for the race.
- Uses I2C communication to interact with sensors.
*/

#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor1;
VL53L0X sensor2;

#define DEBUG false  // Set to false to disable debug sensor readings
#define LOAD_BUTTON_PIN 4  // GPIO pin for the car load button
#define START_BUTTON_PIN 5  // GPIO pin for the start button
#define XSHUT1 2  // GPIO pin for first sensor XSHUT
#define XSHUT2 3  // GPIO pin for second sensor XSHUT
#define relayPin 8  // GPIO pin for CO₂ fire pin

unsigned long startTime;
bool raceStarted = false;
bool car1Finished = false;
bool car2Finished = false;
unsigned long car1Time = 0;
unsigned long car2Time = 0;
bool loadButtonPressed = false;  // To track the load button press state
bool loadButtonLastState = HIGH;  // Last state of the button (used for edge detection)
bool carsLoaded = false;  // Flag to track if cars have been loaded
bool startButtonPressed = false;  // To track the start button press state
bool startButtonLastState = HIGH;  // Last state of the start button (used for edge detection)

void setup() {
    Serial.begin(9600);
    Serial.println("\n--- CO₂ Car Race Timer ---");
    Serial.println("Initializing system...");
    
    Wire.begin();  // Initialize the I2C communication
    delay(100);

    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, HIGH);  // Ensure relay is off at startup
    Serial.println("✔ Relay initialized (OFF)");

    pinMode(XSHUT1, OUTPUT);
    pinMode(XSHUT2, OUTPUT);
    pinMode(LOAD_BUTTON_PIN, INPUT_PULLUP);  // Set load button pin as input with internal pull-up
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);  // Set start button pin as input with internal pull-up

    // Reset both sensors
    digitalWrite(XSHUT1, LOW);
    digitalWrite(XSHUT2, LOW);
    delay(10);

    Serial.println("🔄 Starting VL53L0X sensors...");

    // Start first sensor
    digitalWrite(XSHUT1, HIGH);
    delay(10);
    if (sensor1.init()) {
        sensor1.setAddress(0x30);  // Change default address
        Serial.println("✔ Sensor 1 initialized at 0x30.");
    } else {
        Serial.println("❌ ERROR: Sensor 1 not detected!");
        return;  // Exit setup if sensor fails
    }

    // Start second sensor
    digitalWrite(XSHUT2, HIGH);
    delay(10);
    if (sensor2.init()) {
        sensor2.setAddress(0x31);  // Change default address
        Serial.println("✔ Sensor 2 initialized at 0x31.");
    } else {
        Serial.println("❌ ERROR: Sensor 2 not detected!");
        return;  // Exit setup if sensor fails
    }

    sensor1.startContinuous();
    sensor2.startContinuous();
    Serial.println("✔ Sensors are now active.");

    Serial.println("\n✅ System Ready!");
    Serial.println("Press 'L' via Serial or press the load button to load cars.");
}

void loop() {
    // Read the current state of the load button and start button
    bool loadButtonState = digitalRead(LOAD_BUTTON_PIN);
    bool startButtonState = digitalRead(START_BUTTON_PIN);

    // Detect the falling edge of the button press (button pressed momentarily)
    if (loadButtonState == LOW && loadButtonLastState == HIGH) {
        loadButtonPressed = true;
        if (!carsLoaded) {
            carsLoaded = true;
            Serial.println("🚦 Cars loaded. Press 'S' to start the race.");
        }
    }

    loadButtonLastState = loadButtonState;  // Update the last state of load button

    if (startButtonState == LOW && startButtonLastState == HIGH) {
        startButtonPressed = true;
        if (carsLoaded && !raceStarted) {
            startRace();
        } else if (!carsLoaded) {
            Serial.println("⚠ Please load the cars first by pressing 'L' or pressing the load button.");
        } else {
            Serial.println("⚠ Race already in progress!");
        }
    }

    startButtonLastState = startButtonState;  // Update the last state of start button

    // Check for serial command to load cars
    if (Serial.available() > 0) {
        char command = Serial.read();
        Serial.print("📩 Received Serial Command: ");
        Serial.println(command);

        // Load cars if 'L' command is received
        if (command == 'L') {
            if (!carsLoaded) {
                carsLoaded = true;
                Serial.println("🚦 Cars loaded. Press 'S' to start the race.");
            } else {
                Serial.println("⚠ Cars are already loaded.");
            }
        }

        // Only allow starting the race if the cars are loaded
        if (command == 'S' && carsLoaded) {
            if (!raceStarted) {
                startRace();
            } else {
                Serial.println("⚠ Race already in progress! Wait for finish.");
            }
        } else if (command == 'S' && !carsLoaded) {
            Serial.println("⚠ Please load the cars first by pressing 'L' or pressing the load button.");
        }
    }

    if (raceStarted) {
        checkFinish();
    }
}

void startRace() {
    Serial.println("\n🚦 Race Starting...");
    Serial.println("🔹 Firing CO₂ Relay...");
    
    // Activate relay to fire CO₂
    digitalWrite(relayPin, LOW);     // Fire CO₂
    startTime = millis();             // Start timer
    raceStarted = true;               // Set race flag
    car1Finished = false;             // Reset finish flags
    car2Finished = false;
    delay(100);
    digitalWrite(relayPin, HIGH);
    Serial.println("✔ Relay deactivated");

    // Unload cars once the race starts
    carsLoaded = false;  // Reset carsLoaded status when the race starts

    Serial.println("🏎 Race in progress...");
}

void checkFinish() {
    int dist1 = sensor1.readRangeContinuousMillimeters();
    int dist2 = sensor2.readRangeContinuousMillimeters();

    if (dist1 == -1) {
        Serial.println("❌ Error reading from Sensor 1");
        return;
    }
    if (dist2 == -1) {
        Serial.println("❌ Error reading from Sensor 2");
        return;
    }

    if (DEBUG) {
        Serial.print("📏 Sensor Readings: C1 = ");
        Serial.print(dist1);
        Serial.print(" mm, C2 = ");
        Serial.print(dist2);
        Serial.println(" mm");
    }

    if (dist1 < 150 && !car1Finished) {  // Car 1 crosses the sensor
        car1Time = millis() - startTime;
        car1Finished = true;
        Serial.print("🏁 Car 1 Finished! Time: ");
        Serial.print(car1Time);
        Serial.println(" ms");
    }

    if (dist2 < 150 && !car2Finished) {  // Car 2 crosses the sensor
        car2Time = millis() - startTime;
        car2Finished = true;
        Serial.print("🏁 Car 2 Finished! Time: ");
        Serial.print(car2Time);
        Serial.println(" ms");
    }

    if (car1Finished && car2Finished) {
        declareWinner();
    }
}

void declareWinner() {
    Serial.println("\n🎉 Race Finished!");
    if (car1Time < car2Time) {
        Serial.println("🏆 Car 1 Wins!");
    } else if (car2Time < car1Time) {
        Serial.println("🏆 Car 2 Wins!");
    } else {
        Serial.println("🤝 It's a tie!");
    }

    // Send race results via Serial
    Serial.print("📊 RESULT: C1=");
    Serial.print(car1Time);
    Serial.print("ms, C2=");
    Serial.print(car2Time);
    Serial.println("ms");

    Serial.println("\n🔄 Geting ready for next race...");
    delay(2000);  // Cooldown period before accepting new race command

    Serial.println("\nPress 'L' via Serial or press the load button to load cars.");
    raceStarted = false;  // Reset race state
}
