#include <Arduino.h>
#include <cactus_io_BME280_I2C.h>
#include "pins.h"
#include "zone.h"

// Constructors

Zone::Zone(uint8_t address, int pin) {
    setupSensor(address);
    setupPin(pin);
}


// Private

/**
 * Initiates BME280 sensor using I2C address
 * Sets proper temp offset based on sensor address
 * Sets online state if successful
**/ 
void Zone::setupSensor(uint8_t address) {
    sensor = BME280_I2C(address);
    if (address == 0x76)
    {
        sensor.setTempCal(-.8);
    } else
    {
        sensor.setTempCal(-1);
    }
    if (sensor.begin()) {online = true;}
}

/**
 * Initiates output PWM pin
 * Sets pwmPin to appropriate pin number
**/ 
void Zone::setupPin(int pin) {
    pwmPin = pin;
    pinMode(pin, OUTPUT);
}


// Public

/**
 * Set fan speed for a particular zone
**/ 
void Zone::setFanSpeed(int speed) {
    fanSpeed = speed;
    speed = map(speed, 0, 100, 0, 1024);
    analogWrite(pwmPin, speed);

    if (speed <= 0) {            // Turn off fans if either speed 0
        digitalWrite(FANPOWER, LOW);
        digitalWrite(LEDPIN, LOW);   // Turn on LED
    } else {                        // Turns fans on elsewise
        digitalWrite(FANPOWER, HIGH);
        digitalWrite(LEDPIN, HIGH);    // Turn off LED
    }
}