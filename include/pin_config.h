#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// I2C Pins
#define PIN_I2C_SDA           8
#define PIN_I2C_SCL           9

// VL53L0X Sensor Pins
#define PIN_SENSOR1_XSHUT     16
#define PIN_SENSOR2_XSHUT     15

// Button Pins
#define PIN_BUTTON_LOAD       4
#define PIN_BUTTON_START      5

// Control Pins
#define PIN_RELAY            14
#define PIN_LED_BLUE         48
#define PIN_LED_RED          47
#define PIN_LED_GREEN        21
#define PIN_BUZZER           45

// SD Card Pins (SPI0)
#define PIN_SD_SCK           12  // GPIO12 - SPI0_SCK
#define PIN_SD_MISO          13  // GPIO13 - SPI0_MISO
#define PIN_SD_MOSI          11  // GPIO11 - SPI0_MOSI
#define PIN_SD_CS            10  // GPIO10 - SPI_CS0

#endif // PIN_CONFIG_H
