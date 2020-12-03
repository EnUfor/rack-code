#include <cactus_io_BME280_I2C.h>
#include <CircularBuffer.h>
#include "pins.h"

#include "zone.h"

class Rack
{
private:
    /**
     * Read sensor and set respective values
     * Must be passed by reference (3 hrs wasted)
    **/    
    void reader(Zone& zone) {
        if (zone.online) {
            zone.sensor.readSensor();
            zone.temp = zone.sensor.getTemperature_F();
            zone.humidity = zone.sensor.getHumidity();
        }
    }

    /**
     * Print sensor values to serial
    **/ 
    void printer(String name, Zone zone) {
        Serial.print(name); Serial.print(":\t");
        if (zone.online) {
            Serial.print(zone.temp); Serial.print(" *F\t\t");
            Serial.print(zone.humidity); Serial.println(" %");
        } else
        {
            Serial.println("Not Connected");
        }
    }

    void setup() {
        pinMode(FANPOWER, OUTPUT);
        pinMode(LEDPIN, OUTPUT);
        setFans(100);       // Set fans to 100% on class declaration
    }
public:
    Rack() {
        setup();
    };

    // NodeMCU ESP8266 pinout: SCL = 5 (D1) SDA = 4 (D2)
    Zone inlet = {0x76, INLETFANPIN};
    Zone outlet = {0x77, OUTLETFANPIN};

    CircularBuffer<double, 20> inletHistory;
    CircularBuffer<double, 20> outletHistory;
    
    bool manualFans = false;

    /**
     * Read sensor values
    **/
    void readSensors() {
        reader(inlet);
        reader(outlet);
        inletHistory.push(inlet.temp);
        outletHistory.push(outlet.temp);
    }

    /**
     * Print sensor values
    **/
    void printSensors() {
        printer("Inlet", inlet);
        printer("Outlet", outlet);
        Serial.println("\n");
    }

    /**
     * Print Buffer Values
    **/
    void printBuff(CircularBuffer<double, 20> & buff) {
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
    void setFans(int speed) {
        // Serial.println((String)"setFans Ran: " + speed);
        inlet.setFanSpeed(speed);
        outlet.setFanSpeed(speed);
    }
};