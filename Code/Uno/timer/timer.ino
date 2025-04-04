/*
--- CO₂ Car Race Timer Version 0.2.0 - 04 April 2025 ---
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

[v0.2.0] - 2025-04-04
Added

    ✅ Optional debug logging: added #define DEBUG flag to toggle sensor distance output.

    ✅ About section at top of code (project description placeholder).

Changed

    🔁 Refactored startRace() to initiate relay trigger and timer simultaneously for improved timing accuracy.

*/

#include <Wire.h>
#include <VL53L0X.h>

#define DEBUG false  // Set to false to disable debug sensor readings

VL53L0X sensor1;
VL53L0X sensor2;

#define XSHUT1 2  // GPIO pin for first sensor XSHUT
#define XSHUT2 3  // GPIO pin for second sensor XSHUT
#define relayPin 8  // GPIO pin for CO₂ fire pin

unsigned long startTime;
bool raceStarted = false;
bool car1Finished = false;
bool car2Finished = false;
unsigned long car1Time = 0;
unsigned long car2Time = 0;

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
    Serial.println("Send 'S' via Serial to start the race.");
}

void loop() {
    // Check for serial command
    if (Serial.available() > 0) {
        char command = Serial.read();
        Serial.print("📩 Received Serial Command: ");
        Serial.println(command);

        // Only start race if it hasn’t already started
        if (command == 'S') {
            if (!raceStarted) {
                startRace();
            } else {
                Serial.println("⚠ Race already in progress! Wait for finish.");
            }
        } else {
            Serial.println("⚠ Invalid command. Send 'S' to start.");
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

    Serial.println("\n🔄 System resetting...");
    delay(2000);  // Cooldown period before accepting new race command

    Serial.println("✅ Ready for next race. Send 'S' to start.");
    raceStarted = false;  // Reset race state
}
