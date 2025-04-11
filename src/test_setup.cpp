#include <Arduino.h>

// Pin Definitions
#define PIN_BUTTON_LOAD       4
#define PIN_BUTTON_START      5
#define PIN_LED_BLUE         48
#define PIN_LED_RED          47
#define PIN_LED_GREEN        21

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    int timeout = 0;
    while (!Serial && timeout < 50) {
        delay(100);
        timeout++;
    }
    Serial.println("Serial initialized");
    
    Serial.println("\nESP32-S3 Basic LED Test");
    Serial.println("=====================");
    
    // Initialize GPIO pins
    pinMode(PIN_BUTTON_LOAD, INPUT_PULLUP);
    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    
    // Initial test sequence
    Serial.println("Starting LED test sequence...");
    
    // Test RGB LED
    Serial.println("Testing RED LED");
    digitalWrite(PIN_LED_RED, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_RED, LOW);
    
    Serial.println("Testing GREEN LED");
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_GREEN, LOW);
    
    Serial.println("Testing BLUE LED");
    digitalWrite(PIN_LED_BLUE, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_BLUE, LOW);
    
    Serial.println("\nLED test complete!");
    Serial.println("Now monitoring buttons and showing status on RGB LED");
    Serial.println("- Press LOAD button (RED LED)");
    Serial.println("- Press START button (GREEN LED)");
    Serial.println("- Press both buttons (BLUE LED)");
}

void loop() {
    bool loadButton = !digitalRead(PIN_BUTTON_LOAD);   // Inverted due to INPUT_PULLUP
    bool startButton = !digitalRead(PIN_BUTTON_START); // Inverted due to INPUT_PULLUP
    
    // Update LED based on button states
    digitalWrite(PIN_LED_RED, loadButton && !startButton);
    digitalWrite(PIN_LED_GREEN, !loadButton && startButton);
    digitalWrite(PIN_LED_BLUE, loadButton && startButton);
    
    // Print button states on change
    static bool lastLoadState = false;
    static bool lastStartState = false;
    
    if (loadButton != lastLoadState || startButton != lastStartState) {
        Serial.printf("Buttons: LOAD=%s START=%s\n", 
            loadButton ? "PRESSED" : "RELEASED",
            startButton ? "PRESSED" : "RELEASED");
            
        lastLoadState = loadButton;
        lastStartState = startButton;
    }
    
    delay(10);  // Debounce delay
}
