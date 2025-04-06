/*
--- CO₂ Car Race Timer Version 0.5.0 ESP32 - 06 April 2025 ---
This system uses two VL53L0X distance sensors to time a CO₂-powered car race.
It measures the time taken for each car to cross the sensor line and declares the winner based on the fastest time.

Features:
- Two distance sensors (VL53L0X) track the progress of two cars.
- Relay control to simulate the CO₂ firing mechanism.
- Serial communication to start the race and display results.
- Supports multiple races by resetting after each one.
- RGB LED indicator to show current race state (waiting, ready, racing, finished).
- Buzzer feedback at race start and finish for audible cues.
- Debounced physical buttons for car load and race start.

ESP32 Pin Assignments:
- I2C: SDA=GPIO21, SCL=GPIO22
- VL53L0X Sensors: XSHUT1=GPIO16, XSHUT2=GPIO17
- Buttons: LOAD=GPIO4, START=GPIO5
- Relay: GPIO14
- Buzzer: GPIO27
- RGB LED: RED=GPIO25, GREEN=GPIO26, BLUE=GPIO33
*/

#include <Wire.h>
#include <VL53L0X.h>

// Function prototypes
void setLEDState(String state);
void startRace();
void checkFinish();
void declareWinner();

VL53L0X sensor1;
VL53L0X sensor2;

#define DEBUG false

// Pin Definitions
#define LOAD_BUTTON_PIN 4
#define START_BUTTON_PIN 5
#define XSHUT1 16
#define XSHUT2 17
#define RELAY_PIN 14  // Changed back to GPIO14 per pin assignments
#define BUZZER_PIN 27

// RGB LED Pins
const int LED_RED = 25;
const int LED_GREEN = 26;
const int LED_BLUE = 33;

// Race State Variables
unsigned long startTime;
bool raceStarted = false;
bool car1Finished = false;
bool car2Finished = false;
unsigned long car1Time = 0;
unsigned long car2Time = 0;
bool loadButtonPressed = false;
bool loadButtonLastState = HIGH;
bool carsLoaded = false;
bool startButtonPressed = false;
bool startButtonLastState = HIGH;

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- CO₂ Car Race Timer ---");
    Serial.println("Initializing system...");

    Wire.begin(21, 22);  // SDA = 21, SCL = 22
    delay(100);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    setLEDState("waiting");

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);  // HIGH = Relay OFF (active-LOW relay)
    Serial.println("✔ Relay initialized (OFF)");

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    pinMode(XSHUT1, OUTPUT);
    pinMode(XSHUT2, OUTPUT);
    pinMode(LOAD_BUTTON_PIN, INPUT_PULLUP);
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(XSHUT1, LOW);
    digitalWrite(XSHUT2, LOW);
    delay(10);

    Serial.println("🔄 Starting VL53L0X sensors...");

    digitalWrite(XSHUT1, HIGH);
    delay(10);
    if (sensor1.init()) {
        sensor1.setAddress(0x30);
        Serial.println("✔ Sensor 1 initialized at 0x30.");
    } else {
        Serial.println("❌ ERROR: Sensor 1 not detected!");
        return;
    }

    digitalWrite(XSHUT2, HIGH);
    delay(10);
    if (sensor2.init()) {
        sensor2.setAddress(0x31);
        Serial.println("✔ Sensor 2 initialized at 0x31.");
    } else {
        Serial.println("❌ ERROR: Sensor 2 not detected!");
        return;
    }

    sensor1.startContinuous();
    sensor2.startContinuous();
    Serial.println("✔ Sensors are now active.");

    Serial.println("\n✅ System Ready!");
    Serial.println("Press 'L' via Serial or press the load button to load cars.");
}

void loop() {
    bool loadButtonState = digitalRead(LOAD_BUTTON_PIN);
    bool startButtonState = digitalRead(START_BUTTON_PIN);

    if (loadButtonState == LOW && loadButtonLastState == HIGH) {
        loadButtonPressed = true;
        if (!carsLoaded) {
            carsLoaded = true;
            setLEDState("ready");
            Serial.println("🚦 Cars loaded. Press 'S' to start the race.");
        }
    }
    loadButtonLastState = loadButtonState;

    if (startButtonState == LOW && startButtonLastState == HIGH) {
        startButtonPressed = true;
        if (carsLoaded && !raceStarted) {
            setLEDState("racing");
            startRace();
        } else if (!carsLoaded) {
            Serial.println("⚠ Please load the cars first by pressing 'L' or pressing the load button.");
        } else {
            Serial.println("⚠ Race already in progress!");
        }
    }
    startButtonLastState = startButtonState;

    if (Serial.available() > 0) {
        char command = Serial.read();
        Serial.print("📩 Received Serial Command: ");
        Serial.println(command);

        if (command == 'L') {
            if (!carsLoaded) {
                carsLoaded = true;
                setLEDState("ready");
                Serial.println("🚦 Cars loaded. Press 'S' to start the race.");
            } else {
                Serial.println("⚠ Cars are already loaded.");
            }
        }

        if (command == 'S' && carsLoaded) {
            if (!raceStarted) {
                setLEDState("racing");
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

    tone(BUZZER_PIN, 1000, 200);

    digitalWrite(RELAY_PIN, LOW);  // LOW = Relay ON
    startTime = millis();
    raceStarted = true;
    car1Finished = false;
    car2Finished = false;
    delay(250);  // Increased delay to 250ms to ensure relay has time to actuate
    digitalWrite(RELAY_PIN, HIGH);  // HIGH = Relay OFF
    Serial.println("✔ Relay deactivated");

    carsLoaded = false;
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

    if (dist1 < 150 && !car1Finished) {
        car1Time = millis() - startTime;
        car1Finished = true;
        Serial.print("🏁 Car 1 Finished! Time: ");
        Serial.print(car1Time);
        Serial.println(" ms");
    }

    if (dist2 < 150 && !car2Finished) {
        car2Time = millis() - startTime;
       // car2Time = (car2Time > 17) ? car2Time - 17 : 0;  // Subtract 17ms to account for sensor timing offset
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

    tone(BUZZER_PIN, 2000, 500);

    if (car1Time < car2Time) {
      Serial.println("🏆 Car 1 Wins!");
  } else if (car2Time < car1Time) {
      Serial.println("🏆 Car 2 Wins!");
  } else {
      Serial.println("🤝 It's a tie!");
  }

    Serial.print("📊 RESULT: C1=");
    Serial.print(car1Time);
    Serial.print("ms, C2=");
    Serial.print(car2Time);
    Serial.println("ms");

    Serial.println("\n🔄 Getting ready for next race...");
    delay(2000);
    setLEDState("finished");
    raceStarted = false;
    Serial.println("\nPress 'L' via Serial or press the load button to load cars.");
}

void setLEDState(String state) {
    if (state == "waiting") {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
    } else if (state == "ready") {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
    } else if (state == "racing") {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, HIGH);
    } else if (state == "finished") {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
    } else {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
    }
}
