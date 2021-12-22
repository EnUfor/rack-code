#include "rack.h"

// Constructors
Rack::Rack() {
    setup();
}


// Private
void Rack::setup() {
    pinMode(FANPOWER, OUTPUT);
    pinMode(LEDPIN, OUTPUT);
    setFans(100);       // Set fans to 100% on class declaration
}

/**
 * Read sensor and set respective values
 * Must be passed by reference (3 hrs wasted)
**/    
void Rack::reader(Zone& zone) {
    if (zone.online) {
        zone.sensor.readSensor();
        zone.temp = zone.sensor.getTemperature_F();
        zone.humidity = zone.sensor.getHumidity();
    }
}

/**
 * Print sensor values to serial
**/ 
void Rack::printer(String name, Zone zone) {
    Serial.print(name); Serial.print(":\t");
    if (zone.online) {
        Serial.print(zone.temp); Serial.print(" *F\t\t");
        Serial.print(zone.humidity); Serial.println(" %");
    } else
    {
        Serial.println("Not Connected");
    }
}


// Public

/**
 * Read sensor values
**/
void Rack::readSensors() {
    reader(inlet);
    reader(outlet);
    inletHistory.push(inlet.temp);
    outletHistory.push(outlet.temp);
}

/**
 * Print sensor values
**/
void Rack::printSensors() {
    printer("Inlet", inlet);
    printer("Outlet", outlet);
    Serial.println("\n");
}

/**
 * Print Buffer Values
**/
void Rack::printBuff(CircularBuffer<double, 20> & buff) {
    for (int i = buff.size() - 1; i >= 0; i--)
    {
        Serial.print(buff.operator[](i));
        if (i > 0)
        {
            Serial.print(", ");
        }    
    }
    Serial.print("\n");
}


/**
 * Sets both fan speeds "simultaneously"
**/ 
void Rack::setFans(int speed) {
    // Serial.println((String)"setFans Ran: " + speed);
    inlet.setFanSpeed(speed);
    outlet.setFanSpeed(speed);
}